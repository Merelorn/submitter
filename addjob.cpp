/*
 * frontend.cpp
 *
 *  Created on: Aug 15, 2017
 *      Author: wondrash
 */
#include "core.h"
#include <props.h>
#include <stdlib.h>     /* srand, rand */
#include <unistd.h>		// usleep
#include <string>


bool addjob(std::string home_path, std::string submit_script, int cpus, int minutes, std::string cleanup_success, std::string cleanup_fail){

	if ( ! initialize(file_paths)){ return false; }

	std::vector<Job> submitted_jobs;
	std::vector< std::vector< std::string > > dummy_vvs;

	std::vector<Job> new_job;
	std::vector<std::string> dummy_entry;
	dummy_entry.push_back(home_path);
	dummy_entry.push_back(submit_script);
	dummy_entry.push_back(std::to_string(cpus));
	dummy_entry.push_back(std::to_string(minutes));
	dummy_entry.push_back(cleanup_success);
	dummy_entry.push_back(cleanup_fail);

	dummy_vvs.clear();
	dummy_vvs.push_back(dummy_entry);

	if ( ! vvs_to_vj(dummy_vvs, new_job, JT_TODO) ){
		return false;
	}
	if ( new_job.size() == 0 ){ return false; }

	bool success = true;

	if ( ! vj_to_textfile(new_job, file_paths.find("newjobs")->second, JT_TODO, true, false) ){
		std::cout << "Unable to submit job!" << std::endl;
		success = false;
	}
	return success;
}

int main(int argc, char * argv[]){
  SUBMITTER_ID = std::getenv("USER");
	  if ( argc < 5 || argc > 7 ) {
		  std::cout << "Adds a job to be processed by submitter." << std::endl;
		  std::cout << "Usage: " << std::endl;
		  std::cout << "Four arguments are expected, two are optional." << std::endl;
		  std::cout << "  1 - home path of a job to be submitted " << std::endl;
		  std::cout << "  2 - submit script" << std::endl;
		  std::cout << "  3 - number of cpu slots" << std::endl;
		  std::cout << "  4 - expected duration in minutes " << std::endl;
		  std::cout << " (5) - script to run after succesful completion " << std::endl;
		  std::cout << " (6) - script to run after unsuccesful completion " << std::endl;
	  } else {
		  std::string cleanup_success = "NA";
		  std::string cleanup_fail = "NA";
		  if ( argc > 5 ){ cleanup_success = argv[5]; }
		  if ( argc > 6 ){ cleanup_fail = argv[6]; }
		  bool pass = addjob(argv[1], argv[2], std::stoi(argv[3]), std::stoi(argv[4]), cleanup_success, cleanup_fail);
		  if ( ! pass ){
			  std::cout << "The job has NOT been submitted!" << std::endl;
			  return 1;
		  } else {
			  std::cout << "The job has been submitted!" << std::endl;
			  return 0;
		  }
	  }
}


