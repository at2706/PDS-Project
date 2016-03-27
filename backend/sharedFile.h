#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <condition_variable>

#include <string.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

#define USER_FILE "db/users"
#define MSG_FILE "db/messages"
#define FLW_FILE "db/follows"


struct shared_file{
	mutex mx;
	fstream fs;
	condition_variable cv;
	string path;

	shared_file(string path): path(path){}
};


//////////////////////////////////
// Global Shared File Variables
//////////////////////////////////
shared_file user_file(USER_FILE),
			msg_file(MSG_FILE),
			flw_file(FLW_FILE);


inline void safe_open(shared_file &shared, ios_base::openmode mode) {
	if (shared.fs.is_open()) {
		cerr << "Warning: Attempted to reopen file: \"" + shared.path + "\"";
		return;
	}

	shared.fs.open(shared.path, mode);

	if (!shared.fs.is_open()) {
		throw "Error: failed to open file: \"" + shared.path + "\"";
	}
}

inline void safe_close(shared_file &shared){
	shared.fs.close();
	shared.fs.clear();
}

inline void safe_remove(string file){
	if(remove(file.c_str()) != 0)
		throw "Error: failed to remove file: \"" + file + "\"";
}

inline void safe_rename(string oldname, string newname){
	if(rename(oldname.c_str(), newname.c_str()) != 0)
		throw "Error failed to rename file: \"" + oldname + "\"";
}