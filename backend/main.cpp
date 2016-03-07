#include <iostream>
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
json post_message(int user_id, string username, string message);

int main(int argc, char const *argv[])
{
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
		auto msg = json::parse(msg_buf);

		json response;

		//route request based on 'request' field
		if (msg["request"] == "post_message") {
			int user_id = stoi(msg["data"]["user_id"].get<string>());
			response = post_message(user_id, msg["data"]["username"], msg["data"]["message"]);
		}

		string response_encoded = response.dump();

		memset(&msg_buf, 0, sizeof(msg_buf));
		strncpy(msg_buf, response_encoded.c_str(), sizeof(msg_buf));
		//write out message
		if (write(sock_fd, msg_buf, sizeof(msg_buf)) == -1) {
			perror("write failed");
			exit(1);
		}	

		close(sock_fd);
	}

	return 0;
}


json post_message(int user_id, string username, string message) {
	cout << "posterID: " << user_id << endl;
	cout << "poster: " << username << endl;
	cout << "message: " << message << endl;

	json msg;
	msg["success"] = true;

	return msg;
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