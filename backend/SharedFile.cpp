#include "SharedFile.h"

//////////////////////////////////
// Constructor. Adds existing blank 
// lines to list.
//////////////////////////////////
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

//////////////////////////////////
// Read line by line and skips over
// blank lines.
//////////////////////////////////
bool SharedFile::read(string &line){
	while(getline(fs, line)){
		if(line == empty_line)
			continue;
		return true;
	}
	return false;
}

//////////////////////////////////
// Offset parameter used for the special
// case in user_file where the next ID is
// at the end of the file.
// Returns true if inserted into blank.
// False if at end of file.
//////////////////////////////////
bool SharedFile::insert(const string data, int offset){
	if(data.length() != line_len){
		close();
		throw "Error: insert data length incorrect. Data: " 
		+ to_string(data.length()) + ", Line Length: " + to_string(line_len);
	}
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
		fs << data << endl;
		return false;
	}
	
}

//////////////////////////////////
// Should be called inside a read loop. 
// The seek position is moved to the 
// previous line and overwrites data.
//////////////////////////////////
bool SharedFile::remove(){
	//Sanity check: is the seek position at the start of a line?
	if(fs.tellp() % (line_len + 1) != 0){
		close();
		throw "Error: remove failed. Seek position invalid.";
	}
	streampos pos;
	pos = fs.tellg();
	pos -= line_len + 1;
	fs.clear();
	fs.seekp(pos);
	fs << empty_line;
	empty_pos.push_back(pos);
	return true;
}