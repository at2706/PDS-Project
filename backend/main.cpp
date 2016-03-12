#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

#define	LISTENQ 1024
#define BUFFER_SIZE 4096
#define DEFAULT_PORT 13000

int setUpServer(int server_port);
json processRequest(json request);

json createUser(string email, string first_name, string last_name, string hashed_password);
json authUser(string email, string hashed_password);
json postMessage(int user_id, string username, string message);
json getMessagesBy(int user_id);
json getMessagesFeed(int user_id);
json getUsers(int user_id);
json userFollowUser(int follower, int followee);
json userUnfollowUser(int follower, int followee);
json getFollowees(int user_id);
json getFollowers(int user_id);

void flash(json &r, string message, string type);
void sFlash(json &r, string message);
void iFlash(json &r, string message);
void wFlash(json &r, string message);
void dFlash(json &r, string message);

void safe_open(fstream &fs, string path, ios_base::openmode mode);

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
		if (request["type"] == "createUser") {
			response = createUser(request["data"]["email"], request["data"]["first_name"],
				request["data"]["last_name"], request["data"]["hashed_password"]);
		}
		else if (request["type"] == "authUser") {
			response = authUser(request["data"]["email"], request["data"]["hashed_password"]);
		}
		else if (request["type"] == "postMessage") {
			int user_id = stoi(request["data"]["user_id"].get<string>());
			response = postMessage(user_id, request["data"]["username"], request["data"]["message"]);
		}
	}

	catch(string e){
		dFlash(response, "An exception has been thrown: " + e);
	}
	return response;
}

json createUser(string email, string first_name, string last_name, string hashed_password) {
	json response;

	fstream fs;
	safe_open(fs, "db/users", fstream::in);

	int id = 0;
	string l_email, l_hashed_password, l_first_name, l_last_name;

	while(fs >> l_email >> l_hashed_password >> l_first_name >> l_last_name){
		if(email.compare(l_email) == 0){
			dFlash(response, "That email is already taken.");
			return response;
		}
		id++;
	}

	fs.close();
	safe_open(fs, "db/users", fstream::out | fstream::app);

	fs << id << "\t" << email << "\t" << hashed_password << "\t" << first_name << "\t" << last_name << endl;
	iFlash(response, "Successfully created user.");
	fs.close();
	return response;
}

//corresponds with login_user
json authUser(string email, string hashed_password) {
	json response;

	fstream fs;
	safe_open(fs, "db/users", fstream::in);

	string line;
	fs >> line;

	return response;
}

json postMessage(int user_id, string username, string message) {
	cout << "posterID: " << user_id << endl;
	cout << "poster: " << username << endl;
	cout << "message: " << message << endl;

	json response;
	response["success"] = true;
	return response;
}

void flash(json &r, string message, string type) {
	r["fMessage"] = message;
	r["fType"] = type;
}

void sFlash(json &r, string message) {
	flash(r, message, "success");
	r["success"] = true;
}

void iFlash(json &r, string message) {
	flash(r, message, "info");
	r["success"] = true;
}

void wFlash(json &r, string message) {
	flash(r, message, "warning");
	r["success"] = false;
}

void dFlash(json &r, string message) {
	flash(r, message, "danger");
	r["success"] = false;
}

void safe_open(fstream &fs, string path, ios_base::openmode mode){
	fs.open(path, mode);

	if (!fs.is_open()) {
		throw "Error: failed to open file: \"" + path + "\"";
	}
}