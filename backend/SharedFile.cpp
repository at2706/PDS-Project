#include "SharedFile.h"

SharedFile::SharedFile(const string path, uint n) : path(path), line_len(n) {
	string line;
	empty_line.resize(line_len, '*');

	streampos pos;

	open(fstream::in);
	while(getline(fs, line)){
		if(line == empty_line){
			pos = fs.tellg();
			pos -= line_len + 1;
			empty_pos.push_back(pos);
		}
	}
	close();
}

void SharedFile::open(ios_base::openmode mode){
	if (fs.is_open()) {
		cerr << "Warning: Attempted to reopen file: \"" + path + "\"";
		return;
	}

	mx.lock();
	fs.open(path, mode);

	if (!fs.is_open()) {
		mx.unlock();
		throw "Error: failed to open file: \"" + path + "\"";
	}
}

void SharedFile::close(){
	fs.close();
	fs.clear();
	mx.unlock();
}

bool SharedFile::read(string &line){
	while(getline(fs, line)){
		if(line == empty_line)
			continue;
		return true;
	}
	return false;
}

bool SharedFile::insert(const string data, int offset){
	if(data.length() != line_len)
		throw "Error: insert data length incorrect. Data: " 
		+ to_string(data.length()) + ", Line Length: " + to_string(line_len);
	streampos pos;
	if(!empty_pos.empty()){
		pos = empty_pos.front();
		empty_pos.pop_front();

		fs.seekp(pos);
		fs << data << endl;
		return true;
	}
	else{
		fs.seekp(offset, fstream::end);
		return false;
	}
}

bool SharedFile::remove(){
	streampos pos;
	pos = fs.tellg();
	pos -= line_len + 1;
	fs.clear();
	fs.seekp(pos);
	fs << empty_line;
	empty_pos.push_back(pos);
	return true;
}