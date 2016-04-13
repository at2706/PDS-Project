#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include "json.hpp"
#include "SharedFile.h"

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

using namespace std;
using json = nlohmann::json;

#define	LISTENQ 1024
#define BUFFER_SIZE 4096
#define DEFAULT_PORT 13000

// User data size: 256 bytes (including newline char)
// Max Users: 9999 (2.56MB)
// Character limits also enforced in javascript
#define ID_LEN 4
#define EMAIL_CHAR_LIMIT 60
#define NAME_CHAR_LIMIT 60

// Message size: 178 bytes (including newline char)
#define MSG_LEN 100

// Misc
#define HASH_LEN 128
#define TIME_LEN 10

// File names
#define USER_FILE "db/users"
#define MSG_FILE "db/messages"
#define FLW_FILE "db/follows"

// Line lengths in each file: including tabs, excluding the last \n
#define USER_LINE_LEN ID_LEN + EMAIL_CHAR_LIMIT + HASH_LEN + NAME_CHAR_LIMIT + 3
#define MSG_LINE_LEN ID_LEN + TIME_LEN + NAME_CHAR_LIMIT + MSG_LEN + 3
#define FLW_LINE_LEN ID_LEN + ID_LEN + 1

//////////////////////////////////
// Global Shared File Variables
//////////////////////////////////
SharedFile user_file(USER_FILE, USER_LINE_LEN),
			msg_file(MSG_FILE, MSG_LINE_LEN),
			flw_file(FLW_FILE, FLW_LINE_LEN);

int setUpServer(int server_port);
void processConnection(int sock_fd);
json processRequest(json request);

json createUser(string email, string first_name, string last_name, string hashed_password);
json deleteUser(int user_id, string hashed_password);
json editUser(	int user_id, string email, string first_name, string last_name,
				string hashed_password, string new_password);
json authUser(string email, string hashed_password);
json getUser(int user_id);
json getProfile(int user_id, int profile);
json postMessage(int user_id, string username, string message);
json getMessagesBy(int user_id);
json getMessagesFeed(int user_id);
json getUsers(int user_id);
json userFollowUser(int follower, int followee);
json userUnfollowUser(int follower, int followee);
json getFollowees(int user_id);
json getFollowers(int user_id);

//////////////////////////////////
// Various functions to display messages
// to the user. Feels like flask, easy
// to use.
//////////////////////////////////
inline void flash(json &r,  const string &message, const string &type) {
	r["fMessage"].push_back(message);
	r["fType"].push_back(type);
}
inline void sFlash(json &r, const string &message) {
	flash(r, message, "success");
	r["success"] = true;
}
inline void iFlash(json &r, const string &message) {
	flash(r, message, "info");
	r["success"] = true;
}
inline void wFlash(json &r, const string &message) {
	flash(r, message, "warning");
	r["success"] = false;
}
inline void dFlash(json &r, const string &message) {
	flash(r, message, "danger");
	r["success"] = false;
}
//////////////////////////////////
// Error handling. Same as flask. The
// exception is caught in processRequest().
//////////////////////////////////
inline void abort(int errcode){
	throw errcode;
}

//////////////////////////////////
// Formats data to be inserted into files.
// Formatted data have a fixed size to allow
// easier file updates. If the fixed size 
// changes, need to update entire file.
// Data in
//////////////////////////////////
inline string format_string(string &str, uint width){
	if(str.length() > width)
		throw overflow_error("Error: Format string too long.");

	str.resize(width, ' ');
	return str;
}

inline string format_int(uint i, uint width){
	if(i > pow(10, width) - 1)
		throw overflow_error("Error: Format int too big.");
	stringstream ss;
	ss.width(width);
	ss.fill('0');
	//int Align Left
	ss << right << i;

	return ss.str();
}

//////////////////////////////////
// Formats time to display on the web page.
// Data out
//////////////////////////////////
inline string format_timestamp(time_t now, time_t t){
	double diff = difftime(now, t);

	string str;

	if(diff < 10)
		str = "Just now";
	else if(diff < 60)
		str = to_string((int)diff) + " seconds ago.";
	else if (diff< 3600){
		int min = diff / 60;
		str = to_string(min);
		str += (min == 1) ? " minute ago." : " minutes ago.";
	}
	else if (diff< 86400){
		int hour = diff / 3600;
		str = to_string(hour);
		str += (hour == 1) ? " hour ago." : " hours ago.";
	}
	else{
		int day = diff / 86400;
		str = to_string(day);
		str += (day == 1) ? " day ago." : " days ago.";
	}
	return str;
}