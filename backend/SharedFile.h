#pragma once
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


class SharedFile{
public:
	fstream fs;

	SharedFile(string path, uint n);
	void open(ios_base::openmode mode);
	void close();

private:
	mutex mx;
	condition_variable cv;
	string path;
	uint line_len;
};