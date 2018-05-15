#include <unistd.h>
#include <iostream>
#include <sstream>
#include <props.h>
#include "core.h"
#include <sys/types.h>
#include <signal.h>

int main(int argc, char * argv[]){
	  if ( argc != 4) {
		  std::cout << "Submitter that will run your jobs at nodes" << std::endl;
		  std::cout << "Usage: " << std::endl;
		  std::cout << "Three arguments are expected." << std::endl;
		  std::cout << "  1 - number of cpus of a node " << std::endl;
		  std::cout << "  2 - lifetime of a submitter in minutes " << std::endl;
		  std::cout << "  3 - minimum number of jobs that need to be run - otherwise terminate" << std::endl;
		  return 1;
	  }

	  int no_of_cpus = std::stoi(argv[1]);
	  int lifetime = std::stoi(argv[2]); // lifetime of this submitter in hours (6min abort time)
		lifetime = (lifetime - 3) * 60;
	  int min_utilization = std::stoi(argv[3]); // minimum number of processor in use before autodestruct

 	  std::string whoami = std::getenv("USER");
	  std::string config_path;	 // address of config file

	  // look for config address - this should be created upon installation
	  std::string where_to_look_file = "/home/" + whoami + "/.submitter_config_location";

	  if ( ! initialize(file_paths)){ return false; }

	  // declarations
	  std::time_t start_time = time(0);
	  std::time_t abort_time = start_time + lifetime;

	  std::cout << "This submitter will terminate at " << str_time(abort_time) << std::endl; 

	  bool b_continue = true;
	  bool autodestruct = false;
	  bool copyfiles_b = true;
      int errnum; // error number storage
      std::stringstream ss;
      std::vector< std::vector < std::string > > config_vvs;
	  int used_cpus = 0;

	  SUBMITTER_ID = std::string(std::getenv("PBS_JOBID"));
	  std::string scratch_path;
	  if ( ! std::stoi(SUBMITTER_ID) > 0 ){ return 1; }
	  
	   //create scratch dir
	   scratch_path = file_paths.find("base_scratch_path")->second + SUBMITTER_ID + "/";
	   errnum = mkdir(scratch_path.c_str(), 0744);
	   if ( errnum != 0 && ! file_exists(scratch_path) ){ std::cout << "SUBMITTER: Unable to create dir " + scratch_path << std::endl; return 1;}

/*	   // create private running file
	   if ( file_exists(scratch_path + "running" + std::to_string(SUBMITTER_ID)) ){ system(("rm " + scratch_path + "running" + std::to_string(SUBMITTER_ID)).c_str()); }
	   errnum = system(("touch " + scratch_path + "running" + std::to_string(SUBMITTER_ID)).c_str());
	   if ( errnum != 0 ){
		   std::cout << "running file cannot be created ( exit code " << errnum << " )" << std::endl;
		   return 1;
	   } else {
		   std::cout << "running file created" << std::endl;
		   file_paths.insert(std::pair<std::string, std::string>("running", scratch_path + "running" + std::to_string(SUBMITTER_ID)));
	   }
*/
	   // containers
	   	   if ( file_exists(file_paths.find("STOP")->second) ){ std::cout << "SUBMITTER: Debug STOP file exists!" << std::endl; return 1; }

	   std::vector<Job> submitted_jobs, running_jobs, finished_jobs, jobs_to_append, dummy_vj;
	   std::vector< std::vector< std::string > > dummy_vvs;
	   std::vector<std::string> list_of_files;

	   bool freshly_submitted; // used to flush unsubmitted jobs only if we submitted something

	   long job_id = 0; // This will identify individual jobs

	   while ( b_continue ) {
		   std::cout << "SUBMITTER: The time is " << str_time() << std::endl;
		   dummy_vvs.clear();

		// check which ones are still actually running
		   for ( std::vector<Job>::iterator it = running_jobs.begin(); it != running_jobs.end(); it++ ){
			   if ( file_exists(it->scratch_path + "/THIS_JOB_HAS_FINISHED") ){
				   // run success script
				   remove((it->scratch_path + "/THIS_JOB_HAS_FINISHED").c_str());
				   errnum = chdir((it->scratch_path).c_str());
				   if ( errnum != 0 ){
					   props::logfile("Job " + it->id + " " + it->home_path +" cannot be cleaned up (normal finish)!" + " at time " + str_time() + " by ID:" + SUBMITTER_ID, file_paths.find("LOG")->second);
				   } else {
					   errnum = it->cleanup_success_method();
					   props::logfile("CLEANUP_SUCCESS " + it->id + " " + it->home_path +" exit_code: " + std::to_string(errnum) + " at time " + str_time() + " by ID:" + SUBMITTER_ID, file_paths.find("LOG")->second);
					   finished_jobs.push_back(*it);
				   }
				   it->delete_me = true;
			   }
		   }
	           errnum = chdir((scratch_path).c_str());
		   if ( errnum != 0 ) {
			std::cout << "Cannot cd to " << scratch_path << " !" << std::endl;
	           	errnum = chdir("/home/");
		   }

		   // copy home finished jobs
		   for ( auto it : finished_jobs){
			   ls(it.scratch_path, list_of_files);
			   copyfiles_b = true;
			   for (auto it2 : list_of_files){
				   copyfiles_b = copyfiles_b & copy_file(it.scratch_path + it2, it.home_path + it2);
			   }
			   props::logfile("FINISHED " + it.id + " " + it.home_path + " exit_code: " + std::to_string(!copyfiles_b)+ " at time " + str_time() + " by ID:" + SUBMITTER_ID, file_paths.find("LOG")->second);
			   system(("rm -rf " + it.scratch_path).c_str());
		   }

		   // flush finished jobs to finished file
		   if ( finished_jobs.size() != 0 ){
			   vj_to_textfile(finished_jobs, file_paths.find("finished")->second, JT_DONE, true);
			   finished_jobs.clear();
		   }

		   // remove all finished jobs from running_jobs
		   dummy_vj.clear();
		   for ( auto it : running_jobs){
			   if ( ! it.delete_me ){ dummy_vj.push_back(it); }
		   }
		   running_jobs = dummy_vj;

		// any free slots?
		// read-in submitted jobs

		   // create LONGLOCK - "submitted" file will remain locked until we are done deciding which jobs we run
		   // LONGLOCK is released in the "flush unsubmitted" section
		   dummy_vvs.clear();
		   submitted_jobs.clear();
		   jobs_to_append.clear();

		   // count load
		   used_cpus = 0;
		   for ( auto it : running_jobs ){
			   used_cpus += it.cpus;
		   }
		   std::cout << "SUBMITTER: I am using " << used_cpus << "CPUs and running " << running_jobs.size() << " jobs!" << std::endl;

		   // only check submitted file if we have free slots!
		   if (no_of_cpus != used_cpus ){
			   freshly_submitted = false;
			   if ( ! ( textfile_to_vvs(file_paths.find(std::string("submitted"))->second, dummy_vvs, true) && vvs_to_vj(dummy_vvs, submitted_jobs, JT_TODO) ) ){
				   std::cout << "SUBMITTER: \"submitted\" file has not been read properly" << std::endl;
				   b_continue = false;
				   break;
			   }

			// submit them
			   for ( std::vector<Job>::iterator it = submitted_jobs.begin(); it != submitted_jobs.end(); it++){
				   if ( it->cpus > no_of_cpus - used_cpus ){ continue; } // skip if we cannot fit it
				   if ( it->expected_duration * 60 + time(0) > abort_time ){ continue; } // skip if we cannot make it

			// get ID
//				   long newid;
//				   if ( ! claim_id(file_paths.find("idfile")->second, newid) ){std::cout << "Unable to claim new ID!" << std::endl;continue;}
				   ss.str("");
				   ss << ++job_id;
				   it->id = "S" + SUBMITTER_ID + "J" + ss.str();

			// create scratch path
				   errnum = mkdir((scratch_path + it->id + "/").c_str(), 0744);

				   if ( errnum != 0 ){ std::cout << "SUBMITTER: Unable to create dir " + (scratch_path + it->id + "/") << std::endl; continue;}
				   it->scratch_path = scratch_path + it->id + "/";

			// run submit_script
				   errnum = chdir(it->scratch_path.c_str());
				   if ( errnum != 0 ){
					   std::cout << "SUBMITTER: Job " << it->id << " " << it->home_path << " cannot be submitted!" << std::endl;
				   } else {
					   ls(it->home_path, list_of_files);
					   copyfiles_b = true;
					   for (auto it2 : list_of_files){
						   copyfiles_b = copyfiles_b & copy_file(it->home_path + it2, it->scratch_path + it2);
					   }
					   if ( copyfiles_b) {
						   errnum = system((it->submit_script + " && touch THIS_JOB_HAS_FINISHED &").c_str());
//						   it->PID = system("echo $!"); // get PID of a job (so that we can kill it)
//						std::cout << "PID is " << it->PID << " scratch is " << it->scratch_path << std::endl;
						   it->start_time = time(0); // get start_time
						   it->submitter = SUBMITTER_ID; // which submitter is running me?
						   props::logfile("SUBMITTED " + it->id + " " + it->home_path + " exit_code: " + std::to_string(!copyfiles_b) + " at time " + str_time() + " by " + SUBMITTER_ID, file_paths.find("LOG")->second);
						   std::cout << "SUBMITTER: Job " << it->id << " " << it->home_path << " submitted with exit code " << errnum << std::endl;
						   if ( errnum == 0){
							   running_jobs.push_back(*it);
							   it->delete_me = true; // remove from submitted vj
							   freshly_submitted = true;
							   jobs_to_append.push_back(*it);
							   used_cpus += it->cpus;
						   }
					   } else {
						   errnum = chdir(scratch_path.c_str());
						   std::cout << "SUBMITTER: Copying files failed for job from " << it->home_path << " - trying another one.." << std::endl;
						   system(("rm -rf " + it->scratch_path).c_str());
					   }
				   }
			   }

			   // remove all submitted jobs from submitted_jobs
			   dummy_vj.clear();
			   for ( auto it : submitted_jobs){
				   if ( ! it.delete_me ){
					dummy_vj.push_back(it);
				   } else {
			   		std::cout << "SUBMITTER: I have just submitted a new job!" << std::endl;
				   }
			   }
			   submitted_jobs = dummy_vj;

			   // flush unsubmitted jobs
			   // this is the only time where we disrespect longlock - the whole reason of the mechanism
			   if ( freshly_submitted && ! vj_to_textfile(submitted_jobs, file_paths.find("submitted")->second, JT_TODO, false, true) ){
					b_continue = false;
			   }
			   // remove longlock
			   remove(((file_paths.find(std::string("submitted"))->second)+"_longlock").c_str());
			   file_exists((file_paths.find(std::string("submitted"))->second)); // we do this in hope of removing longlock
			   file_exists((file_paths.find(std::string("submitted"))->second)+"_longlock"); // we do this in hope of removing longlock

		   }

		    // check for autodestruct
			if ( file_exists(file_paths.find("STOP")->second) ){
				b_continue = false;
			}

			// min utilization
			if ( used_cpus < min_utilization ) { b_continue = false; }
			
			// is it time to go already?
			if ( time(0) > abort_time ){
				std::cout << "We're almost done! Time to cleanup! " << str_time() << std::endl;
				b_continue = false;
			} // abort 6 minutes before running out of time - we still need to clean up!
			
			// don't be too eager
			std::cout << "DEBUG_lsof_current: " << props::num_exec("lsof 2>/dev/null | wc -l") << std::endl;
			if ( b_continue ){ usleep(120000000); }
	   }
// end of while loop

	   std::cout << "SUBMITTER: We are left with " << running_jobs.size() << " running jobs." << std::endl;

/*
	   // clean unfinished jobs
	   for ( auto it : running_jobs ){
		   std::cout << "SUBMITTER: Killing job " << it.PID << " !" << std::endl;
		   kill(it.PID, 1); // SIGHUP running jobs in the background
		   errnum = chdir(it.scratch_path.c_str());
		   if ( errnum != 0 ){
			   std::cout << "SUBMITTER: Job " << it.id << " " << it.home_path << " cannot be cleaned up!" << std::endl;
		   } else {
			   errnum = it.cleanup_fail_method();
			   props::logfile("CLEANUP_FAIL " + it.id + " " + it.home_path +" exit_code: " + std::to_string(errnum) + " at time " + str_time() + " by ID:" + SUBMITTER_ID, file_paths.find("LOG")->second);
		   }
	   }
*/

	   // copy home unfinished jobs
           errnum = chdir(file_paths.find("base_scratch_path")->second.c_str());
	   for ( auto it : running_jobs){
	   	   ls(it.scratch_path, list_of_files);
	   	   copyfiles_b = true;
		   for (auto it2 : list_of_files){
			   copyfiles_b = copyfiles_b & copy_file(it.scratch_path + it2, it.home_path + it2);
		   }
		   props::logfile("UNFINISHED " + it.id + " " + it.home_path + " exit_code: " + std::to_string(!copyfiles_b) + " at time " + str_time() + " by ID:" + SUBMITTER_ID, file_paths.find("LOG")->second);
		   system(("rm -rf " + it.scratch_path).c_str());
	   }
	   vj_to_textfile(running_jobs, file_paths.find("unfinished")->second, JT_DONE, true); 	// dump these to unfinished file

	   // clean private files
	   remove((file_paths.find("running")->second).c_str());
	   rmdir(scratch_path.c_str());

	   props::logfile("EXITING " + SUBMITTER_ID + " " + str_time(), file_paths.find("LOG")->second);

}
