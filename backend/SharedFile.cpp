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
	
	fs.clear();
	fs.open(path, mode);

	if (!fs.is_open()) {
		throw runtime_error("Error: failed to open file: \"" + path + "\"");
	}
}

//////////////////////////////////
// Close file.
//////////////////////////////////
void SharedFile::close(){
	fs.close();
}

//////////////////////////////////
// Read line by line and skips over blank
// lines.
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
// Barebone write function specific for
// creating user. It's the only abnormal
// file, having next id appended at EOF.
//////////////////////////////////
void SharedFile::write(string data, streamoff offset, fstream::seekdir way){
	fs.clear();
	fs.seekp(offset, way);
	fs << data;
}
//////////////////////////////////
// Offset parameter used for the special
// case in user_file where the next ID is
// at the EOF. Returns true if inserted 
// into blank. False if at end of file.
//////////////////////////////////
bool SharedFile::insert(const string data, streamoff offset){
	throw_assert(validData(data));
	fs.clear();
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
	throw_assert(validWrite() && validData(data));
	streampos pos = fs.tellg();
	pos -= line_len + 1;
	fs.clear();
	fs.seekp(pos);
	fs << data;
	return true;
}

////////////////////////////////// 
// The seek position is moved to the 
// previous line and overwrites data.
//////////////////////////////////
bool SharedFile::remove(){
	throw_assert(validWrite());
	streampos pos = fs.tellg();
	pos -= line_len + 1;
	fs.clear();
	fs.seekp(pos);
	fs << empty_line;
	empty_pos.push_back(pos);
	return true;
}

////////////////////////////////// 
// Removes a series of entries based
// on a vector of positions.
//////////////////////////////////
void SharedFile::remove(vector<streampos>& v){
	fs.clear();
	for(streampos pos : v){
		fs.seekp(pos);
		remove();
	}
}

////////////////////////////////// 
// Accepts a lambda function that determines
// what type of entries to find. Returns 
// the position of all found entries. The 
// limit parameter is for controlling return size.
//////////////////////////////////
vector<streampos> SharedFile::find(function<bool(string)> &cond, int limit){
	string line;
	std::vector<streampos> v;
	fs.clear();
	while(read(line) && (int)v.size() != limit){
		if(cond(line)){
			v.push_back(fs.tellg());
		}
	}
	return v;
}
