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
	
	fs.open(path, mode);

	if (!fs.is_open()) {
		throw "Error: failed to open file: \"" + path + "\"";
	}
}

void SharedFile::close(){
	fs.close();
	fs.clear();
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
	validate(validData(data), "insert");
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
// Make changes to existing data
//////////////////////////////////
bool SharedFile::edit(string data){
	validate(validWrite() && validData(data), "edit");
	streampos pos = fs.tellg();
	pos -= line_len + 1;
	fs.clear();
	fs.seekp(pos);
	fs << data;
	return true;
}

//////////////////////////////////
// Should be called inside a read loop. 
// The seek position is moved to the 
// previous line and overwrites data.
//////////////////////////////////
bool SharedFile::remove(){
	validate(validWrite(), "remove");
	streampos pos = fs.tellg();
	pos -= line_len + 1;
	fs.clear();
	fs.seekp(pos);
	fs << empty_line;
	empty_pos.push_back(pos);
	return true;
}

// Validation checks before execution
void SharedFile::validate(bool cond, string msg){
	if(!cond){
		fs.close();
		throw "Error in file: " + path + " " + msg;
	}
}

// Write should be at start of line
bool SharedFile::validWrite(){
	return fs.tellp() % (line_len + 1) == 0;
}

// Data should have the same lengths
bool SharedFile::validData(string data){
	return data.length() == line_len;
}