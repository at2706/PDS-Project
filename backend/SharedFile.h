#pragma once
#include <iostream>
#include <fstream>
#include <functional>
#include <vector>
#include <string>
#include <list>
#include <mutex>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

// Macro for debugging
// Shows where exception was thrown
#define throw_assert(cond) {\
	if(!cond){\
		close();\
		throw runtime_error("Error processing file: " + path + \
							". Function: " + __PRETTY_FUNCTION__);\
	}}\

class SharedFile{
public:
	mutable mutex mx;

	SharedFile(const string path, uint n);
	void open(fstream::openmode mode);
	void close();

	bool read(string &line);
	void write(string data, streamoff offset = 0, fstream::seekdir way = fstream::cur);
	bool insert(const string data, streamoff offset = 0);
	bool edit(string data);
	bool remove();

	void remove(vector<streampos>& v);
	vector<streampos> find(function<bool(string)> &cond, int limit = -1);

private:
	fstream fs;
	const string path;
	const uint line_len;
	string empty_line;
	// Seek positions of empty lines
	list<streampos> empty_pos;

	// Write should be at start of line
	inline bool validWrite(){
		return fs.tellp() % (line_len + 1) == 0;
	}

	// Data should have the same lengths
	inline bool validData(string data){
		return data.length() == line_len;
	}
};