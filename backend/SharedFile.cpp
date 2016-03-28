#include "SharedFile.h"

SharedFile::SharedFile(string path, uint n) : path(path), line_len(n) {}

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