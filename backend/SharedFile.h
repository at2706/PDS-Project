#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <mutex>

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

	SharedFile(const string path, uint n);
	void open(ios_base::openmode mode);
	void close();

	bool read(string &line);
	bool insert(const string data, int offset = 0);

	bool remove();

private:
	mutex mx;
	string path;

	uint line_len;
	string empty_line;
	// Seek positions of empty lines
	list<streampos> empty_pos;
};