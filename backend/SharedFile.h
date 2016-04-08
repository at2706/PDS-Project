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

class SharedFile{
public:
	fstream fs;
	mutex mx;

	SharedFile(const string path, uint n);
	void open(ios_base::openmode mode);
	void close();

	bool read(string &line);
	bool insert(const string data, int offset = 0);
	bool edit(string data);
	bool remove();

private:
	const string path;
	const uint line_len;
	string empty_line;
	// Seek positions of empty lines
	list<streampos> empty_pos;

	void validate(bool cond, string msg);
	bool validWrite();
	bool validData(string data);
};