/*
 * frontend.cpp
 *
 *  Created on: Aug 15, 2017
 *      Author: wondrash
 */
#include "core.h"
#include <props.h>
#include <string>

enum job_status { JS_SUBMITTED, JS_RUNNING, JS_FINISHED, JS_UNFINISHED, JS_NONE };

bool statusjob(std::string home_path, job_status & status){
	if ( ! props::ends_with(home_path,"/") ) { home_path = home_path + "/"; } // add "/" to the end of the string

	std::map< std::string, std::string > file_paths;
	if ( ! initialize(file_paths)){ return false; }
	std::vector<Job> dummy_vj;
	std::vector< std::vector< std::string > > dummy_vvs;

	// is it submitted?
	if ( ! ( textfile_to_vvs(file_paths.find(std::string("submitted"))->second, dummy_vvs) && vvs_to_vj(dummy_vvs, dummy_vj, JT_TODO) ) ){
	   std::cout << "\"submitted\" file has not been read properly" << std::endl;
	   return false;
	}
	for ( auto it : dummy_vj ){
		if ( it.home_path == home_path ){
			status = JS_SUBMITTED;
			return true;
		}
	}

	// is it running?
	if ( ! textfile_to_vvs(file_paths.find(std::string("LOG"))->second, dummy_vvs) ){
	   std::cout << "\"LOG\" file has not been read properly" << std::endl;
	   return false;
	}

	std::vector<std::string> entry;
	for ( auto it : dummy_vvs ){
		if ( it.size() > 2 ){
			if ( it[2] == home_path ){entry = it;}
		}
	}
	if ( entry.size() > 0 ){
		if ( entry[0] == "SUBMITTED" ){
			status = JS_RUNNING;
			return true;
		}
	}

	// is it finished?
	if ( ! ( textfile_to_vvs(file_paths.find(std::string("finished"))->second, dummy_vvs) && vvs_to_vj(dummy_vvs, dummy_vj, JT_TODO) ) ){
	   std::cout << "\"finished\" file has not been read properly" << std::endl;
	   return false;
	}
	for ( auto it : dummy_vj ){
		if ( it.home_path == home_path ){
			status = JS_FINISHED;
			return true;
		}
	}

	// is it unfinished?
	if ( ! ( textfile_to_vvs(file_paths.find(std::string("unfinished"))->second, dummy_vvs) && vvs_to_vj(dummy_vvs, dummy_vj, JT_TODO) ) ){
	   std::cout << "\"unfinished\" file has not been read properly" << std::endl;
	   return false;
	}
	for ( auto it : dummy_vj ){
		if ( it.home_path == home_path ){
			status = JS_UNFINISHED;
			return true;
		}
	}

	status = JS_NONE;
	return true;
}

int main(int argc, char * argv[]){
  SUBMITTER_ID = std::getenv("USER");

	  if ( argc > 2 ) {
		  std::cout << "Tells you the status of a job residing in a given home path." << std::endl;
		  std::cout << "Usage: " << std::endl;
		  std::cout << "A single argument is expected" << std::endl;
		  std::cout << "  1 - home path of a job to be inquired about status " << std::endl;
	  } else {
	      std::string my_path = std::getenv("PWD");
		  if ( argc == 2 ){ my_path = argv[1]; }
		  job_status status;
		  bool pass = statusjob(my_path, status);
		  if ( ! pass ){
			  std::cout << "Failed to acquire status!" << std::endl;
			  return 1;
		  } else {
			  switch(status){
				  case JS_SUBMITTED : std::cout << "There is a SUBMITTED job at " << my_path << std::endl; break;
				  case JS_RUNNING : std::cout << "There is a RUNNING job at " << my_path << std::endl; break;
				  case JS_FINISHED : std::cout << "There is a FINISHED job at " << my_path << std::endl; break;
				  case JS_UNFINISHED : std::cout << "There is an UNFINISHED job at " << my_path << std::endl; break;
				  case JS_NONE : std::cout << "I am not aware of any job at " << my_path << std::endl; break;
			  }
			  return 0;
		  }
	  }
}
