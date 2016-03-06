#include <iostream>
#include <string>

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

#define	LISTENQ 1024
#define BUFFER_SIZE 4096
#define DEFAULT_PORT 13000

int setUpServer(int server_port);

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

		char msg_buf[BUFFER_SIZE];
		memset(&msg_buf, 0, sizeof(msg_buf));
		if (read(sock_fd, msg_buf, BUFFER_SIZE) == -1) {
			perror("read failed");
			exit(1);
		}

		cout << msg_buf << endl;

		int msg_buf_n = snprintf(msg_buf, BUFFER_SIZE, "Hello from C++!");
		if (write(sock_fd, msg_buf, msg_buf_n) == -1) {
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