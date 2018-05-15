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

bool commit(int & newjobs){

	std::vector< std::vector<std::string> > dummy_vvs;
	std::vector<Job> newjobs_vj;

	if ( ! initialize(file_paths)){ return false; }

	std::string target_file = file_paths.find(std::string("newjobs"))->second;
	if ( ! ( textfile_to_vvs(target_file, dummy_vvs) && vvs_to_vj(dummy_vvs, newjobs_vj, JT_TODO) ) ){
	  return false;
	} else {
          newjobs = newjobs_vj.size();
	  std::ofstream of_newjobs; of_newjobs.open(target_file,std::ios::out);of_newjobs.close(); // empty newjobs
	  if ( newjobs_vj.size() == 0 ){ return true; }
	}

	bool success = true;

	if ( ! vj_to_textfile(newjobs_vj, file_paths.find(std::string("submitted"))->second, JT_TODO, true, false) ){
		std::cout << "Unable to commit jobs - submitted file cannot be accessed!" << std::endl;
		success = false;
	}
	return success;
}

int main(int argc, char * argv[]){

  int newjobs = 0;
  SUBMITTER_ID = std::getenv("USER");
  bool pass = commit(newjobs);

  if ( ! pass ){
    std::cout << "The jobs have NOT been committed!" << std::endl;
  } else {
    std::cout << "I have committed " << newjobs << " new jobs!" << std::endl;
  }
  return 0;
}


