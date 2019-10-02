#include "core.h"
#include <chrono>
#include <props.h>
#include <sstream> //stringstream
#include <stdio.h> //remove
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <map>

#include <sys/types.h> // opendir
#include <dirent.h> // opendir

#include <sys/stat.h> // time reads
#include <time.h> // time reads

std::string SUBMITTER_ID;
std::map< std::string, std::string > file_paths;

long wait_constant = 100000;
bool debug = true;

int Job::cleanup_success_method(){
	int errnum = 0;
	if ( dedicated_success ){
		errnum = system(cleanup_success.c_str());
	}
	return errnum;
}

int Job::cleanup_fail_method(){
	int errnum = 0;
//	int touch_errnum = 0;
	if ( dedicated_fail ){
		errnum = system(cleanup_fail.c_str());
	}

	if ( errnum != 0 ) { return errnum; }
//	if ( touch_errnum != 0 ) { return touch_errnum; }
	return 0;
}

bool lock_and_access(const std::string target_file, const std::string & msg, bool create_longlock, bool ignore_longlock){

	unsigned int max_wait = 200;

	unsigned int waiting_for = 0;
	std::string lock = target_file + "_lock";
	std::string longlock = target_file + "_longlock";
	std::fstream myofile;
	std::string line;

	struct stat lock_stat, longlock_stat;
	int lock_exists = stat(lock.c_str(), &lock_stat);
	int longlock_exists = stat(longlock.c_str(), &longlock_stat);
	bool b_lock_exists, b_longlock_exists;
	
	long reason_nonexist = 0, reason_locked = 0, reason_longlocked = 0;
	long waited_for = 0;

	while( waiting_for < max_wait ){
		b_lock_exists = file_exists(lock); // lock is not older than 60 seconds
		b_longlock_exists = file_exists(longlock); // longlock is not older than 60 seconds

		// if longlock is older than 5 minutes - remove it!
		stat(longlock.c_str(), &longlock_stat);
		if ( b_longlock_exists && longlock_stat.st_mtime + 300 < time(0) ){
			remove(longlock.c_str());
			b_longlock_exists = false;
		}

		if ( (! file_exists(target_file)) ){ reason_nonexist++; }
		if ( reason_nonexist > 30 ){ break; }
		if ( b_lock_exists ){ reason_locked++; }
		if ( b_longlock_exists ){ reason_longlocked++; }
		if ( (! file_exists(target_file)) || b_lock_exists || ( b_longlock_exists && !ignore_longlock ) ){
			//usleep(5000000 + waiting_for * wait_constant); waiting_for++;
			waited_for += 10000000;
			usleep(10000000);
		} else {
			// lock
			//long waited_for = (waiting_for * waiting_for + waiting_for) * wait_constant / 2 + 100000 * (waiting_for + 1);
			myofile.open(lock.c_str());
			myofile << "lock" << std::endl;
			myofile.close();
			// longlock
			if ( create_longlock ){
				myofile.open(longlock.c_str(), std::fstream::out);
				myofile << SUBMITTER_ID << std::endl;
				myofile.close();

				file_exists(longlock);
				usleep(3000000);
				file_exists(longlock);

				myofile.open(longlock.c_str(), std::fstream::in);
				line = "";
				getline(myofile, line);
				myofile.close();

				if ( line != SUBMITTER_ID ){
					waiting_for++;
					std::cout << "SUBMITTER: I keep waiting, because longlock is being held by " << line << "" << std::endl;
					continue;
				}
			}
			
			if ( debug ){ std::cerr << "SUBMITTER: We waited for " << waited_for / 1000000 << " seconds to access " << target_file << std::endl; }
			return true;
		}
	}
	std::cout << "SUBMITTER: File cannot be accessed or is locked: \" " + target_file + " \"" << std::endl;
	std::cout << "SUBMITTER: File was nonexistent " << reason_nonexist << " times, locked " << reason_locked << " times, and longlocked " << reason_longlocked << " times. Total of " << max_wait << " waits." << std::endl;

	return false;
}

int ls (std::string dir, std::vector<std::string> & files){
  DIR *dp;
  struct dirent *dirp;
  if((dp = opendir(dir.c_str())) == NULL) {
    std::cout << "SUBMITTER: Error opening " << dir << std::endl;
    return 1;
  }

  files.clear();
  while ((dirp = readdir(dp)) != NULL) {
    if ( int(dirp->d_type) == 4 ){ continue; } // ignore if this is a directory
    files.push_back(std::string(dirp->d_name));
  }
  closedir(dp);
  return 0;
}

// read config/submitted and put it into vector of vector of strings
bool textfile_to_vvs(const std::string & target_file, std::vector< std::vector< std::string > > & text_vvs, bool longlock){

	if ( debug ){ std::cerr << "SUBMITTER: locking " << target_file << "!" << std::endl; }
	if ( ! lock_and_access(target_file, " textfile_to_vvs " + target_file, longlock, false) ){ return false; }
	if ( debug ){ std::cerr << "SUBMITTER: done locking " << std::endl; }

	std::string lock = target_file + "_lock";
	std::ifstream myifile(target_file);
	std::string line;
	text_vvs.clear();

	while ( myifile.good() ){
	  std::getline(myifile, line);
	  if ( line == "" ){ continue; }

	  std::vector< std::string > text_entry;
	  for ( int i = 1; i <= props::ReadColumn(line, 0).size(); i++ ){
		text_entry.push_back(props::ReadColumn(line, i));
	  }
	  text_vvs.push_back( text_entry );
	}
	myifile.close();
	if ( longlock & debug ){ props::logfile("DEBUG_TEXTFILE_TO_VVS " + SUBMITTER_ID + " : " + std::to_string(text_vvs.size()) + " lines read from " + target_file + " at " + str_time(), file_paths.find("LOG")->second); }

	remove(lock.c_str()); // unlock
	if ( debug ){ std::cerr << "SUBMITTER: unlocked " << target_file << "!" << std::endl; }

	return true;
}

// read paths to files

//  submitted running finished_sub finished unfinished
bool vvs_to_map_config (std::vector< std::vector< std::string > > config_vvs, std::map< std::string, std::string > & parsed){
  for ( auto it : config_vvs ){
    if ( it[0] == "submitted"  || it[0] == "finished" || it[0] == "unfinished" || it[0] == "LOG" || it[0] == "STOP" || it[0] == "base_scratch_path" || it[0] == "newjobs" ){
      if ( it[0] != "base_scratch_path" && it[0] != "STOP" && ! file_exists(it[1]) ) { continue; } // ignore this entry if the file does not exist
      if ( it[0] == "base_scratch_path" ){ // ignore base_scratch_path if it's not a dir
        if ( ! dir_exists(it[1]) ) { continue; }
        if ( ! props::ends_with(it[1],"/") ) { it[1] = it[1] + "/"; } // add "/" to the end of the string
      }
      parsed.insert(std::pair<std::string, std::string> (it[0], it[1]));
    }
  }
  if ( parsed.size() != 7 ){
	  std::cout << "SUBMITTER: " << parsed.size() << std::endl;
	  std::cout << "SUBMITTER: Not all config-files entries exist." << std::endl;
	  for ( auto it : parsed ){
		  std::cout << "SUBMITTER: " << it.second << std::endl;
	  }
	  return false;
  }
  return true;
}

bool vvs_to_vj ( const std::vector< std::vector< std::string > > & jobs_vvs, std::vector< Job > & parsed, joblist_type jt_type){
	  parsed.clear();
	  Job entry;

	  for ( auto it : jobs_vvs ){

		// If reading "to-do" jobs, the expected format is
		// home_path submit_script_path no_of_cpus expected_duration_in_hours cleanup_script_success cleanup_script_fail

		// If reading "running" jobs, the expected format is
		// home_path submit_script_path no_of_cpus expected_duration_in_hours cleanup_script_success cleanup_script_fail start_time scratch_path job_id
		if ( jt_type == JT_RUNNING && it.size() != 9 ) { continue; }

		if ( jt_type == JT_TODO && it.size() != 6 ) { continue; }

		// home path exists
	    if ( ! dir_exists(it[0]) ){ continue; }
	    if ( ! props::ends_with(it[0],"/") ) { it[0] = it[0] + "/"; } // add "/" to the end of the string
	    entry.home_path = it[0];

	    // submit script exists
	    if ( ! file_exists(it[1]) ){ continue; }
	    entry.submit_script = it[1];

	    // read number of cpus
	    if ( ! ( props::first_num(it[2]) > 0 && props::first_num(it[2]) <= 24 ) ){ continue; }
	    entry.cpus = int(props::first_num(it[2]));

	    // read number of expected duration in hours
	    entry.expected_duration = int(props::first_num(it[3]));

	    // clean success script exists
	    if ( ! file_exists(it[4]) ){
	    	entry.dedicated_success = false;
	    	entry.cleanup_success = "NA";
	    } else {
	    	entry.dedicated_success = true;
	    	entry.cleanup_success = it[4];
	    }

	    // clean fail script exists
	    if ( ! file_exists(it[5]) ){
	    	entry.dedicated_fail = false;
	    	entry.cleanup_fail = "NA";
	    } else {
	    	entry.dedicated_fail = true;
	    	entry.cleanup_fail = it[5];
	    }

	    if ( jt_type == JT_RUNNING ) {
	    	// read start time
	    	entry.start_time = time_t(props::first_num(it[6]));

	    	// scratch path
		    if ( ! dir_exists(it[7]) ){ continue; }
		    entry.scratch_path = it[7];

	    	// id
	    	entry.id = int(props::first_num(it[8]));
	    }
	    parsed.push_back(entry);
	  }
	  if ( debug ){ props::logfile("DEBUG_VVS_TO_VJ " + SUBMITTER_ID + " : " + std::to_string(parsed.size()) + " jobs processed at " + str_time(), file_paths.find("LOG")->second); }
	  return true;
}

bool claim_id(std::string id_file, long & id){

	if ( debug ){ std::cerr << "SUBMITTER: locking " << id_file << "!" << std::endl; }
	if ( ! lock_and_access(id_file, " claiming id ") ){ return false; } // lock
	if ( debug ){ std::cerr << "SUBMITTER: done locking " << id_file << "!" << std::endl; }

	std::string lock = id_file + "_lock";
	std::fstream idfile_fs;
	std::string line;

	// read id
	idfile_fs.open(id_file);
	std::getline(idfile_fs, line);
	id = std::stol(line);
	idfile_fs.close();

	// increment id
	idfile_fs.open(id_file);
	idfile_fs << id + 1 << std::endl;
	idfile_fs.close();

	remove(lock.c_str()); //  unlock
	if ( debug ){ std::cerr << "SUBMITTER: unlocked " << id_file << "!" << std::endl; }
	return true;
}

bool vj_to_textfile(const std::vector< Job > & jobs_vj, std::string target_file, joblist_type jt_type, bool append, bool ignore_lock){

	if ( debug ){ std::cerr << "SUBMITTER: locking " << target_file << "!" << std::endl; }
	if ( ! lock_and_access(target_file, " vj_to_textfile ", false, ignore_lock) ){ return false; } // lock
	if ( debug ){ std::cerr << "SUBMITTER: done locking " << target_file << "!" << std::endl; }
	std::string lock = target_file + "_lock";

	// flush
	std::stringstream ss;
	std::ofstream myofile;

	if ( append ){
		myofile.open(target_file.c_str(), std::fstream::app);
	} else {
		myofile.open(target_file.c_str());
	}

	for ( auto it : jobs_vj ){
		ss.str(std::string());
		ss << it.home_path << " " << it.submit_script << " " << it.cpus << " " << it.expected_duration << " " <<
		it.cleanup_success << " " << it.cleanup_fail;
		if ( jt_type == JT_RUNNING ){
			ss << " " << str_time(it.start_time) << " " << it.scratch_path << " " << it.id;
		} else if ( jt_type == JT_DONE ){
			ss << " " << str_time(it.start_time) << " " << it.scratch_path << " " << it.id << " " << it.submitter;
		}

		myofile << ss.str() << std::endl;
	}
	std::cout << " jobs flushed to " + target_file + " at " + str_time(); 
	if ( debug ){ props::logfile("DEBUG_VJ_TO_TEXTFILE " + SUBMITTER_ID + " : " + std::to_string(jobs_vj.size()) + " jobs flushed to " + target_file + " at " + str_time(), file_paths.find("LOG")->second); } 
	myofile.close();

	// unlock
	remove(lock.c_str());
	if ( debug ){ std::cerr << "SUBMITTER: unlocked " << target_file << "!" << std::endl; }
	if ( ignore_lock ){ remove((target_file + "_longlock").c_str()); }
	return true;
}

// read in config file = prerequisite
bool initialize(std::map< std::string, std::string > & file_paths){

	  std::string myhome = std::getenv("HOME");
	  std::string config_path;	 // address of config file

	  // look for config address - this should be created upon installation
	  std::string where_to_look_file = myhome + "/.submitter_config_location";
	  if ( ! file_exists(where_to_look_file) ) {
		  std::cout << "SUBMITTER: Where is a config file?" << std::endl;
		  return false;
	  } else {
		  std::ifstream myifile;
		  myifile.open(where_to_look_file);
		  std::getline(myifile, config_path);
		  myifile.close();
	  }

	  std::vector< std::vector < std::string > > config_vvs;

	  if ( ! ( textfile_to_vvs(config_path, config_vvs ) && vvs_to_map_config(config_vvs, file_paths) ) ){ // read config or crash
		  std::cout << "SUBMITTER: Could not read config file properly!" << std::endl;
		  return false;
	  }
	  return true;
}
