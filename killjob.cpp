/*
 * frontend.cpp
 *
 *  Created on: Aug 15, 2017
 *      Author: wondrash
 */
#include "core.h"
#include <props.h>
#include <string>

bool killjob(std::string home_path){
	if ( ! props::ends_with(home_path,"/") ) { home_path = home_path + "/"; } // add "/" to the end of the string

	std::map< std::string, std::string > file_paths;

	if ( ! initialize(file_paths)){ return false; }

	std::vector<Job> submitted_jobs;
	std::vector< std::vector< std::string > > dummy_vvs;

	if ( ! ( textfile_to_vvs(file_paths.find(std::string("submitted"))->second, dummy_vvs) && vvs_to_vj(dummy_vvs, submitted_jobs, JT_TODO) ) ){
	   std::cout << "\"submitted\" file has not been read properly" << std::endl;
	   return false;
	}

	for ( std::vector<Job>::iterator it = submitted_jobs.begin(); it != submitted_jobs.end(); it++ ){
		if ( it->home_path == home_path ){
			submitted_jobs.erase(it);
			return vj_to_textfile(submitted_jobs, file_paths.find("submitted")->second, JT_TODO);
		}
	}
	return false;
}

int main(int argc, char * argv[]){
 SUBMITTER_ID = std::getenv("USER");

	if ( argc > 2 ) {
	  std::cout << "Removes the first non-running job with given path from a list of submitted jobs." << std::endl;
	  std::cout << "Usage: " << std::endl;
	  std::cout << "A single optional argument is accepted" << std::endl;
	  std::cout << " (1) - home path of a job to be removed from list of submitted jobs " << std::endl;
	} else {
      std::string my_path = std::getenv("PWD");
	  if ( argc == 2 ){ my_path = argv[1]; }
	  bool pass = killjob(my_path);
	  if ( ! pass ){
		  std::cout << "The job has NOT been killed!" << std::endl;
		  return 1;
	  } else {
		  std::cout << "The job has been killed!" << std::endl;
		  return 0;
	  }
	}
}



