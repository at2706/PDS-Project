#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include "json.hpp"

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

using namespace std;
using json = nlohmann::json;

#define	LISTENQ 1024
#define BUFFER_SIZE 4096
#define DEFAULT_PORT 13000
#define USER_FILE "db/users"
#define MSG_FILE "db/messages"
#define FLW_FILE "db/follows"

// User data size: 256 bytes
// Max Users: 9999 (2.56MB)
// Character limits also enforced in javascript
#define ID_LEN 4
#define EMAIL_CHAR_LIMIT 60
#define NAME_CHAR_LIMIT 60

// Message size: 178 bytes
#define MSG_LEN 100

int setUpServer(int server_port);
json processRequest(json request);

json createUser(string email, string first_name, string last_name, string hashed_password);
json authUser(string email, string hashed_password);
json getUser(int user_id);
json postMessage(int user_id, string username, string message);
json getMessagesBy(int user_id);
json getMessagesFeed(int user_id);
json getUsers(int user_id);
json userFollowUser(int follower, int followee);
json userUnfollowUser(int follower, int followee);
json getFollowees(int user_id);
json getFollowers(int user_id);

void flash(json &r,  const string &message, const string &type);
void sFlash(json &r, const string &message);
void iFlash(json &r, const string &message);
void wFlash(json &r, const string &message);
void dFlash(json &r, const string &message);
void abort(int errcode);

void safe_open(fstream &fs, string path, ios_base::openmode mode);
string format_string(string &str, uint width);
string format_int(uint i, uint width);

int main(int argc, char const *argv[]) {
	//create socket
	int server_fd = setUpServer(DEFAULT_PORT);
	
	while(1) {
		//accept
		printf("Waiting for client connection..\n");
		int sock_fd = accept(server_fd, (struct sockaddr *) NULL, NULL);\
		if (sock_fd == -1) {
			perror("accept failed");
			exit(1);
		}
		printf("Connection established with a client.\n");

		//read request
		char msg_buf[BUFFER_SIZE];
		memset(&msg_buf, 0, sizeof(msg_buf));
		if (read(sock_fd, msg_buf, BUFFER_SIZE) == -1) {
			perror("read failed");
			exit(1);
		}

		json request = json::parse(msg_buf);

		json response = processRequest(request);

		string response_encoded = response.dump();

		//write out message
		if (write(sock_fd, response_encoded.c_str(), response_encoded.size()) == -1) {
			perror("write failed");
			exit(1);
		}	

		close(sock_fd);
	}

	return 0;
}

//Set up server using the given port
//Returns the server file descriptor
int setUpServer(int server_port) {
	//create socket
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		perror("socket failed");
		exit(1);
	}
	
	//set up server info
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(server_port);
	
	//bind to establish socket's interface and port
	if (bind(server_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
		perror("bind failed");
		exit(1);
	}

	//listen
	if (listen(server_fd, LISTENQ) == -1) {
		perror("listen failed");
		exit(1);
	}

	return server_fd;
}

//route request based on 'type' field
json processRequest(json request) {
	json response;

	try{

		if(request["type"] == "createUser"){
			response = createUser(request["data"]["email"], request["data"]["first_name"],
				request["data"]["last_name"], request["data"]["hashed_password"]);
		}

		else if(request["type"] == "authUser"){
			response = authUser(request["data"]["email"], request["data"]["hashed_password"]);
		}

		else if(request["type"] == "getUser"){
			int user_id = request["data"]["user_id"];
			response = getUser(user_id);
		}

		else if(request["type"] == "postMessage"){
			int user_id = request["data"]["user_id"];
			response = postMessage(user_id, request["data"]["username"], request["data"]["message"]);
		}
		else if(request["type"] == "getMessagesBy"){
			response = getMessagesBy(request["data"]["user_id"]);
		}


		
		else if(request["type"] == "userFollowUser"){
			response = userFollowUser(request["data"]["follower"], request["data"]["followee"]);
		}
		else if(request["type"] == "userUnfollowUser"){
			response = userUnfollowUser(request["data"]["follower"], request["data"]["followee"]);
		}

	}
	catch(int errcode){
		response.clear();
		response["success"] = false;
		response["errcode"] = errcode;
	}
	catch(char const* e){
		response.clear();
		string err_msg = "An exception has been thrown: ";
		err_msg += e;
		dFlash(response, err_msg);
	}
	catch(exception &e){
		response.clear();
		string err_msg = "An exception has been thrown: ";
		err_msg += e.what();
		cout << err_msg << endl;
		dFlash(response, err_msg);
	}
	return response;
}

json createUser(string email, string first_name, string last_name, string hashed_password) {
	json response;

	fstream fs;
	safe_open(fs, USER_FILE, fstream::in);

	int id = 0;
	int l_id;
	string l_email, l_hashed_password, l_first_name, l_last_name;

	while(fs >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name){
		if(email.compare(l_email) == 0){
			dFlash(response, "That email is already taken.");
			return response;
		}
		id++;
	}

	fs.close();
	safe_open(fs, USER_FILE, fstream::out | fstream::app);

	string name = first_name + " " + last_name;
	fs  << format_int(id, ID_LEN) << "\t" 
		<< format_string(email, EMAIL_CHAR_LIMIT) << "\t" 
		<< hashed_password << "\t"
		<< format_string(name, NAME_CHAR_LIMIT)
		<< endl;
	iFlash(response, "Successfully created user.");
	fs.close();
	return response;
}

//corresponds with login_user
json authUser(string email, string hashed_password) {
	json response;

	fstream fs;
	safe_open(fs, USER_FILE, fstream::in);

	int l_id;
	string l_email, l_hashed_password, l_first_name, l_last_name;

	while(fs >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name){
		if(email.compare(l_email) == 0 && hashed_password.compare(l_hashed_password) == 0){
			response["user_id"] = l_id;
			response["email"] = l_email;
			response["first_name"] = l_first_name;
			response["last_name"] = l_last_name;
			sFlash(response, "Successfully Logged in.");
			fs.close();
			return response;
		}
	}

	wFlash(response, "Bad log in.");
	fs.close();
	return response;
}

json getUser(int user_id){
	json response;

	fstream fs;
	safe_open(fs, USER_FILE, fstream::in);

	int l_id;
	string l_email, l_hashed_password, l_first_name, l_last_name;

	while(fs >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name){
		if(user_id == l_id){
			response["user_id"] = l_id;
			response["email"] = l_email;
			response["first_name"] = l_first_name;
			response["last_name"] = l_last_name;

			fs.close();
			response["success"] = true;
			return response;
		}
	}

	fs.close();
	abort(401);
	return response;
}

json postMessage(int user_id, string username, string message) {
	json response;

	int length = message.length();
	if(length > MSG_LEN){
		wFlash(response, "Your message was too long. " + to_string(length) + " characters.");
		return response;
	}

	time_t current_time = time(NULL);

	fstream fs;
	safe_open(fs, MSG_FILE, fstream::out | fstream::app);

	// Will have to format time by 11/20/2286
	fs  << current_time << "\t" 
		<< format_int(user_id, ID_LEN) << "\t" 
		<< format_string(username, NAME_CHAR_LIMIT) << "\t"
		<< format_string(message, MSG_LEN)
		<< endl;

	fs.close();
	iFlash(response, "Message posted.");
	return response;
}

json getMessagesBy(int user_id){
	json response;
	json messages;

	fstream fs;
	safe_open(fs, MSG_FILE, fstream::in);

	int timestamp, l_id;
	string l_first_name, l_last_name, l_message;

	while(fs >> timestamp >> l_id >> l_first_name >> l_last_name >> l_message){
		if(user_id == l_id){
			json message;
			message["time"] = timestamp;
			message["user_id"] = l_id;
			message["first_name"] = l_first_name;
			message["last_name"] = l_last_name;
			message["message"] = l_message;
			messages.push_back(message);
		}
	}

	reverse(messages.begin(), messages.end());
	response["messages"] = messages;

	fs.close();
	return response;
}

json userFollowUser(int follower, int followee){
	json response;
	const uint line_len = ID_LEN + ID_LEN + 2;

	string line, empty_line;
	empty_line.resize(line_len - 1, '*');

	int e_pos = -1;
	int l_follower, l_followee;

	fstream fs;
	safe_open(fs, FLW_FILE, fstream::in | fstream::out);

	while(getline(fs, line)){
		if(line == empty_line){
			if(e_pos == -1){
				e_pos = fs.tellg();
				e_pos -= line_len;
			}
			continue;
		}

		stringstream ss(line);
		ss >> l_follower >> l_followee;
		if(follower == l_follower && followee == l_followee){
			wFlash(response, "You're alreadying following that user.");
			return response;
		}
	}
	fs.clear();

	if(e_pos == -1)
		fs.seekp(0, fstream::end);
	else{
		fs.seekp(e_pos);
	}


	cout << fs.tellp() << endl;
	fs  << format_int(follower, ID_LEN) << "\t" 
		<< format_int(followee, ID_LEN) << endl;
	fs.close();

	sFlash(response, "Follow user successful!");
	return response;
}

json userUnfollowUser(int follower, int followee){
	json response;
	const uint line_len = ID_LEN + ID_LEN + 2;

	string line, empty_line;
	empty_line.resize(line_len - 1, '*');

	uint e_pos = -1;
	int l_follower, l_followee;

	fstream fs;
	safe_open(fs, FLW_FILE, fstream::in | fstream::out);

	while(getline(fs, line)){
		stringstream ss(line);
		ss >> l_follower >> l_followee;
		if(follower == l_follower && followee == l_followee){
			e_pos = fs.tellg();
			e_pos -= line_len; // Go back to beginning of line.
			fs.clear();
			fs.seekp(e_pos);
			fs << empty_line;
			fs.close();
			sFlash(response, "Unfollow user successful!");
			return response;
		}
	}
	fs.close();

	wFlash(response, "You're not following that user.");
	return response;
}

void flash(json &r,  const string &message, const string &type) {
	r["fMessage"].push_back(message);
	r["fType"].push_back(type);
}

void sFlash(json &r, const string &message) {
	flash(r, message, "success");
	r["success"] = true;
}

void iFlash(json &r, const string &message) {
	flash(r, message, "info");
	r["success"] = true;
}

void wFlash(json &r, const string &message) {
	flash(r, message, "warning");
	r["success"] = false;
}

void dFlash(json &r, const string &message) {
	flash(r, message, "danger");
	r["success"] = false;
}

void abort(int errcode){
	throw errcode;
}

void safe_open(fstream &fs, string path, ios_base::openmode mode) {
	if (fs.is_open()) {
		cerr << "Warning: Attempted to reopen file: \"" + path + "\"";
		return;
	}

	fs.open(path, mode);

	if (!fs.is_open()) {
		throw "Error: failed to open file: \"" + path + "\"";
	}
}

string format_string(string &str, uint width){
	if(str.length() > width)
		throw "Error: Format string too long.";

	str.resize(width, ' ');
	return str;
}

string format_int(uint i, uint width){
	if(i > pow(10, width) - 1)
		throw "Error: Format int too big.";
	stringstream ss;
	ss.width(width);
	ss.fill('0');
	//int Align Left
	ss << right << i;

	return ss.str();
}