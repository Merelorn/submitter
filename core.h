#ifndef CORE_H_
#define CORE_H_

#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <ctime>
#include <vector>
#include <dirent.h> // ls
#include <sys/types.h> //stat
#include <sys/stat.h> // stat

enum joblist_type { JT_TODO, JT_RUNNING, JT_DONE };

extern std::string SUBMITTER_ID;
extern std::map< std::string, std::string > file_paths;

class Job {
public:
  Job(){delete_me = false;};
  int cleanup_success_method(); // kill if running, copy from scratch_path to home_path
  int cleanup_fail_method();

  std::string scratch_path, home_path, submit_script;
  std::string cleanup_success, cleanup_fail;
  int expected_duration, cpus, PID;
  time_t start_time;
  std::string id, submitter;
  bool delete_me;

  bool dedicated_success, dedicated_fail; // indicates whether the user has provided a script for cleaning up after success/fail

};

int ls (std::string dir, std::vector<std::string> & files);

inline bool copy_file(std::string from, std::string to){
    std::ifstream  src(from, std::ios::binary);
    std::ofstream  dst(to,   std::ios::binary);

    if ( src.good() && dst.good() ){
    	dst << src.rdbuf();
	src.close(); dst.close();
    	return true;
    } else {
    	return false;
    }
}

inline bool concat_files(std::string from, std::string to){
    std::ifstream  src(from, std::ios::binary);
    std::ofstream  dst(to,   std::ios::app);

    if ( src.good() && dst.good() ){
    	dst << src.rdbuf();
	src.close(); dst.close();
    	return true;
    } else {
    	return false;
    }
}

inline bool dir_exists (const std::string & dir){
//  DIR *pdir = opendir(dir.c_str());
  struct stat info;
  stat(dir.c_str(), &info);

  if( info.st_mode & S_IFDIR ){ return true; }
  return false;
}

//inline bool file_exists (const std::string & name) {
//  struct stat buffer;
//  return (stat (name.c_str(), &buffer) == 0);
//}
inline std::string dirname(const std::string filename){
	return filename.substr(0, filename.find_last_of("/"));
}

inline bool file_exists (const std::string& name) {
//	DIR *pdir = opendir(dirname(name).c_str());
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

inline std::string str_time(std::time_t arg_time = 0){
    char output[30];

    std::time_t current_time = arg_time;
    if ( arg_time == 0 ){
      current_time = time(0);
    }
    strftime(output, 30, "%Y %m-%d %H-%M-%S %a", std::localtime(&current_time));
    return std::string(output);
}

bool initialize(std::map< std::string, std::string > & file_paths);
bool vvs_to_vj (std::vector< std::vector< std::string > > config_vvs, std::vector< Job > & parsed, joblist_type jt_type);
bool vvs_to_map_config (std::vector< std::vector< std::string > > config_vvs, std::map< std::string, std::string > & parsed);
bool textfile_to_vvs(const std::string config_path, std::vector< std::vector< std::string > > & config_vvs, bool longlock = false);
bool vj_to_textfile(std::vector< Job > jobs_vj, std::string target_file, joblist_type jt_type = JT_RUNNING, bool append = false, bool ignore_longlock = false);
bool claim_id(std::string id_file, long & id);

bool lock_and_access(const std::string target_file, const std::string & msg, bool create_longlock = false, bool ignore_longlock = false);

#endif /* CORE_H_ */
