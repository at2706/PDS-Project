#include "main.h"

int main(int argc, char const *argv[]) {
	int server_port = getPortInput(argc, argv);
	if (server_port == -1) {
		return 0;
	}

	user_file = new SharedFile(to_string(server_port) + "_" + USER_FILE, USER_LINE_LEN);
	msg_file = new SharedFile(to_string(server_port) + "_" + MSG_FILE, MSG_LINE_LEN);
	flw_file = new SharedFile(to_string(server_port) + "_" + FLW_FILE, FLW_LINE_LEN);

	//Initial setup if this is the primary RM
	if (server_port == FIRST_RM_PORT) {
		isPrimaryManager = true;
		thread t(slaveHandler, server_port);
		t.detach();
	}
	//Initial setup if this is a slave RM
	else {
		thread t(slaveHeartBeat, server_port);
		t.detach();
	}

	//create socket
	int server_fd = setUpServer(server_port);
	while(1) {
		//accept
		printf("Waiting for client connection..\n");
		int sock_fd = accept(server_fd, (struct sockaddr *) NULL, NULL);
		if (sock_fd == -1) {
			perror("accept failed");
			exit(1);
		}
		printf("Connection established with a client.\n");
		thread t(processConnection, sock_fd, server_port);
		t.detach();
	}

	return 0;
}

void slaveHandler(int server_port) {
	while(true) {
		sleep(HEARTBEAT_INTERVAL + HEARTBEAT_GRACE_PERIOD);
		try {
			rm_mx.lock();
			for (auto& rm : replica_managers) {
				int port = get<0>(rm);
				bool alive = get<1>(rm);
				bool heartbeat_received = get<2>(rm);
	
				//don't need to check ourselves since we are not a slave
				if (port == server_port) continue;
				//don't need to check dead slaves
				if (!alive) continue;
				//this slave didn't send a heartbeat - set him as dead
				if (!heartbeat_received) {
					get<1>(rm) = false;
					cout << "SLAVE WITH PORT " << port << " IS DEAD." << endl;
				}
				//reset
				get<2>(rm) = false;
			}
			rm_mx.unlock();
		}
		catch(exception &e){
			rm_mx.unlock();
		}
	}
}

//Sends periodic heartbeat messages to the primary RM
//If there's no response, then determine who is the next primary RM
void slaveHeartBeat(int server_port) {
	int primary_RM_port = FIRST_RM_PORT;
	while (true) {
		int sock_fd = connectToServer(DEFAULT_SERVER_IP, primary_RM_port);
		
		//can't connect to primary RM
		if (sock_fd == -1) {
			cout << "PRIMARY RM "<< primary_RM_port << " IS DOWN." << endl;
			//make the next RM in list be primary RM
			//could be itself
			try {
				rm_mx.lock();
				for (auto& rm : replica_managers) {
					int port = get<0>(rm);

					//set previous primary RM as dead
					if (port == primary_RM_port) {
						get<1>(rm) = false;
					}

					//skip over dead RMs
					bool alive = get<1>(rm);
					if (!alive) continue;

					//if next RM in list is itself
					if (port == server_port) {
						cout << "WE ARE NOW THE PRIMARY RM." << endl;
						isPrimaryManager = true;
						thread t(slaveHandler, server_port);
						t.detach();
						return;
					}
					//set next RM in list as primary
					else {
						cout << "PRIMARY RM IS NOW " << port << "." << endl;
						primary_RM_port = port;
						break;
					}
				}
			}
			catch(exception &e){
				rm_mx.unlock();
			}
		}
		else {
			//send heartbeat to primary RM
			json heartbeat;
			heartbeat["type"] = "heartbeatMessage";
			heartbeat["data"]["from_port"] = server_port;
			string heartbeat_encoded = heartbeat.dump();
			if (write(sock_fd, heartbeat_encoded.c_str(), heartbeat_encoded.size()) == -1) {
				perror("write failed");
				exit(1);
			}
			cout << "HEARTBEAT SENT" << endl;
			
			//wait for heartbeat response from primary RM
			char msg_buf[BUFFER_SIZE];
			memset(&msg_buf, 0, sizeof(msg_buf));
			int n = read(sock_fd, msg_buf, BUFFER_SIZE);
			if (n == -1) {
				perror("read failed");
				exit(1);
			}
			json request = json::parse(msg_buf);
			if (request["type"] == "heartbeatResponse") {
				cout << "RECEIVED HEARTBEAT RESPONSE" << endl;
			}
			close(sock_fd);
			sleep(HEARTBEAT_INTERVAL);
		}
	}
}

//Connect to the given server ip and port
//Returns a socket file descriptor
int connectToServer(string server_ip, int server_port) {
	//create socket
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		perror("socket failed");
		exit(1);
	}
	
	//set up server info
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);
	int pton_return_val = inet_pton(AF_INET, server_ip.c_str(), &servaddr.sin_addr);
	if (pton_return_val == 0) {
		fprintf(stderr, "%s is an invalid ip address.\n", server_ip.c_str());
		exit(1);
	}
	else if (pton_return_val == -1) {
		perror("inet_pton failed");
		exit(1);
	}

	//connect to server
	if (connect(sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
		return -1;
	}

	return sock_fd;
}

//Get user port input
//Can only be one of the static RM ports
int getPortInput(int argc, char const *argv[]) {
	int server_port = 0;
	if (argc == 2) {
		char *t;
		server_port = strtol(argv[1], &t, 0);
		if (argv[1] == t) {
			cout << "Error: server_port provided is not a valid number." << endl;
			return -1;
		}
	}
	if (argc <= 1 || (server_port != FIRST_RM_PORT && server_port != SECOND_RM_PORT && server_port != THIRD_RM_PORT)) {
		cout << "Usage: main server_port" << endl;
		cout << "server_port must be either " << FIRST_RM_PORT << ", " << SECOND_RM_PORT << ", or ";
		cout << THIRD_RM_PORT << ". The first server ran should always be " << FIRST_RM_PORT << "." << endl;
		return -1;
	}
	return server_port;
}

// Set up server using the given port
// Returns the server file descriptor
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

// Threaded Function
// Parses the request for processing
void processConnection(int sock_fd, int server_port){
	// read request
	char msg_buf[BUFFER_SIZE];
	memset(&msg_buf, 0, sizeof(msg_buf));
	if (read(sock_fd, msg_buf, BUFFER_SIZE) == -1) {
		perror("read failed");
		exit(1);
	}

	json request = json::parse(msg_buf);
	json response = processRequest(request, server_port);
	string response_encoded = response.dump();

	//write out message
	if (write(sock_fd, response_encoded.c_str(), response_encoded.size()) == -1) {
		perror("write failed");
		exit(1);
	}	

	close(sock_fd);
}

//////////////////////////////////
// Route request based on 'type' field.
// Required locks are all aquired simultaneously
// in a specified order before function calls. 
// Locks are released automatically on completion.
//////////////////////////////////
json processRequest(json request, int server_port) {
	json response;
	response["success"] = true;

	try{
		bool is_write_request = false;
		if(request["type"] == "heartbeatMessage" && isPrimaryManager){
			//Keep this slave alive by setting the heartbeat_received flag
			try {		
				rm_mx.lock();
				for (auto& rm : replica_managers) {
					int port = get<0>(rm);
					if (request["data"]["from_port"] == port) {
						get<2>(rm) = true; //heartbeat_received
					}
				}
				rm_mx.unlock();
			}
			catch(exception &e){
				rm_mx.unlock();
			}
			cout << "RECEIVED HEARTBEAT FROM " << request["data"]["from_port"] << endl;
			response["type"] = "heartbeatResponse";
		}
		else if(request["type"] == "createUser"){
			lock_guard<mutex> lock(user_file->mx);
			response = createUser(request["data"]["email"], request["data"]["first_name"],
				request["data"]["last_name"], request["data"]["hashed_password"]);
			is_write_request = true;
		}
		else if(request["type"] == "deleteUser"){
			lock_guard<mutex> lock(user_file->mx);
			response = deleteUser(request["data"]["user_id"], request["data"]["hashed_password"]);
			is_write_request = true;
		}
		else if(request["type"] == "editUser"){
			lock_guard<mutex> user_lock(user_file->mx);
			response = editUser(request["data"]["user_id"],
								request["data"]["email"],
								request["data"]["first_name"],
								request["data"]["last_name"],
								request["data"]["hashed_password"],
								request["data"]["new_password"]);
			is_write_request = true;
		}
		else if(request["type"] == "authUser"){
			lock_guard<mutex> user_lock(user_file->mx);
			response = authUser(request["data"]["email"], request["data"]["hashed_password"]);
		}
		else if(request["type"] == "getUser"){
			lock_guard<mutex> user_lock(user_file->mx);
			response = getUser(request["data"]["user_id"]);
		}
		else if(request["type"] == "getProfile"){
			lock_guard<mutex> flw_lock(flw_file->mx);
			lock_guard<mutex> user_lock(user_file->mx);
			response = getProfile(request["data"]["user_id"], request["data"]["profile"]);
		}
		else if(request["type"] == "postMessage"){
			lock_guard<mutex> msg_lock(msg_file->mx);
			int user_id = request["data"]["user_id"];
			response = postMessage(user_id, request["data"]["username"], request["data"]["message"]);
			is_write_request = true;
		}
		else if(request["type"] == "getMessagesBy"){
			lock_guard<mutex> msg_lock(msg_file->mx);
			response = getMessagesBy(request["data"]["user_id"]);
		}
		else if(request["type"] == "getMessagesFeed"){
			lock_guard<mutex> flw_lock(flw_file->mx);
			lock_guard<mutex> user_lock(user_file->mx);
			lock_guard<mutex> msg_lock(msg_file->mx);
			response = getMessagesFeed(request["data"]["user_id"]);
		}
		else if(request["type"] == "getUsers"){
			lock_guard<mutex> flw_lock(flw_file->mx);
			lock_guard<mutex> user_lock(user_file->mx);
			int user_id = request["data"]["user_id"];
			response = getUsers(user_id);
		}
		else if(request["type"] == "userFollowUser"){
			lock_guard<mutex> flw_lock(flw_file->mx);
			response = userFollowUser(request["data"]["follower"], request["data"]["followee"]);
			is_write_request = true;
		}
		else if(request["type"] == "userUnfollowUser"){
			lock_guard<mutex> flw_lock(flw_file->mx);
			response = userUnfollowUser(request["data"]["follower"], request["data"]["followee"]);
			is_write_request = true;
		}
		else if(request["type"] == "getFollowees"){
			lock_guard<mutex> flw_lock(flw_file->mx);
			lock_guard<mutex> user_lock(user_file->mx);
			response = getFollowees(request["data"]["user_id"]);
		}
		else if(request["type"] == "getFollowers"){
			lock_guard<mutex> flw_lock(flw_file->mx);
			lock_guard<mutex> user_lock(user_file->mx);
			response = getFollowers(request["data"]["user_id"]);
		}

		else{
			response["success"] = false;
			cerr << "Request type could not be processed: " << request["type"] << endl;
			dFlash(response, "Something Bad Happened!");
		}

		//tell slaves to do this request if its an update
		if (isPrimaryManager && is_write_request) {
			try {
				rm_mx.lock();
				for (auto& rm : replica_managers) {
					int port = get<0>(rm);
					bool alive = get<1>(rm);

					//don't tell yourself since you are not a slave
					if (port == server_port) continue;
					//don't tell dead slaves
					if (!alive) continue;

					//send the same request to this slave
					int sock_fd = connectToServer(DEFAULT_SERVER_IP, port);
					string request_encoded = request.dump();
					if (write(sock_fd, request_encoded.c_str(), request_encoded.size()) == -1) {
						perror("write failed");
						exit(1);
					}
					close(sock_fd);
				}
				rm_mx.unlock();
			}
			catch(exception &e){
				rm_mx.unlock();
			}
		}

	}
	// Sends HTTP error codes from abort() to be handled by Web Server
	catch(int errcode){
		response.clear();
		response["success"] = false;
		response["errcode"] = errcode;
	}
	// Catches intentionally thrown exceptions from bad user data
	// Logs error onto the console and notifies user with a general flash
	catch(runtime_error e){
		response.clear();
		response["success"] = false;
		cerr << "An exception has been thrown: " << e.what() << endl;
		dFlash(response, "Something Bad Happened!");
	}
	// Catches everything else.
	// Should never be executed.
	catch(exception &e){
		response.clear();
		response["success"] = false;
		cerr << "Unknown Exception: " << e.what() << endl;
		dFlash(response, "Something Bad Happened!");
	}
	return response;
}

//////////////////////////////////
// Files Accessed: users(read/write)
// Exceptions: format_string/int
json createUser(string email, string first_name, string last_name, string hashed_password) {
	json response;
	string line;
	int l_id;

	//Scan user_file for already existing email addresses.
	user_file->open(fstream::in | fstream::out);
	while(user_file->read(line)){
		stringstream ss(line);
		string l_email;
		ss >> l_id >> l_email;
		if(email.compare(l_email) == 0){
			user_file->close();
			dFlash(response, "That email is already taken.");
			return response;
		}
	}
	// After reading the file, l_id would contain the next id, located on the last line.

	string name = first_name + " " + last_name;
	bool filled = user_file->insert(format_int(l_id, ID_LEN) + "\t" 
		+ format_string(email, EMAIL_CHAR_LIMIT) + "\t" 
		+ hashed_password + "\t"
		+ format_string(name, NAME_CHAR_LIMIT), -ID_LEN);

	// If user data got inserted at EOF, 
	// then the next ID has been overwritten.
	// Otherwise, seek ID_LEN bytes from EOF
	// to overwrite the next ID.
	if(filled){
		user_file->write(format_int(l_id + 1, ID_LEN), -ID_LEN, fstream::end);
	}
	else{
		user_file->write(format_int(l_id + 1, ID_LEN));
	}

	user_file->close();
	iFlash(response, "Successfully created user.");
	return response;
}

//////////////////////////////////
// Files Accessed: users(read/write)
json deleteUser(int user_id, string hashed_password){
	json response;
	string line;

	bool removed = false;
	int l_id;
	string l_email, l_hashed_password;

	user_file->open(fstream::in | fstream::out);

	while(user_file->read(line)){
		stringstream ss(line);
		ss >> l_id >> l_email >> l_hashed_password;
		if(user_id == l_id){
			if(hashed_password.compare(l_hashed_password) == 0){
				removed = user_file->remove();
				break;
			}
			else{
				user_file->close();
				wFlash(response, "Incorrect password.");
				return response;
			}
		}
	}
	user_file->close();

	// If user not found for some reason
	if(!removed){
		wFlash(response, "Failed to delete user.");
		return response;
	}

	line.clear();
	streampos pos;
	std::vector<streampos> v;

	// Opens follows file
	flw_file->open(fstream::in | fstream::out);
	// Lambda function that finds all entries that relate to user_id
	function<bool(string)> cond = [user_id](string line)->bool{
		stringstream ss(line);
		int l_follower, l_followee;
		ss >> l_follower >> l_followee;
		return user_id == l_follower;
	};
	// Get entry positions and remove
	v = flw_file->find(cond);
	flw_file->remove(v);

	flw_file->close();

	// Cleanup old variables just in case
	line.clear();
	v.clear();

	// Open messages file
	msg_file->open(fstream::in | fstream::out);
	// Lambda function that finds user_id messages
	cond = [user_id](string line)->bool{
		stringstream ss(line);
		int id, timestamp;
		ss >> id >> timestamp;
		return user_id == id;
	};
	// Get message positions and remove
	v = msg_file->find(cond);
	msg_file->remove(v);

	msg_file->close();

	sFlash(response, "Successfully deleted user.");
	return response;
}

//////////////////////////////////
// Files Accessed: users(read/write)
json editUser(int user_id, string email, string first_name, string last_name, string hashed_password, string new_password){
	json response;
	string line;

	int l_id;
	string l_email, l_hashed_password, l_first_name, l_last_name;

	user_file->open(fstream::in | fstream::out);
	while(user_file->read(line)){
		stringstream ss(line);
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		// Find user
		if(user_id == l_id){
			// Check if password is incorrect
			if(hashed_password.compare(l_hashed_password) != 0){
				user_file->close();
				dFlash(response, "Invaid Password.");
				return response;
			}
			break;
		}
	}

	// Builds new user data. Uses old data if the new data is blank.
	string new_data = format_int(user_id, ID_LEN) + '\t';

	string new_email = (email.empty()) ? l_email : email;
	new_data += format_string(new_email, EMAIL_CHAR_LIMIT) + '\t';

	new_password = (new_password.empty()) ? l_hashed_password : new_password;
	new_data += new_password + '\t';

	string new_first_name = (first_name.empty()) ? l_first_name : first_name;
	string new_last_name = (last_name.empty()) ? l_last_name : last_name;
	string new_full_name = new_first_name + " " + new_last_name;

	new_data += format_string(new_full_name, NAME_CHAR_LIMIT);

	// Finish building, and insert back into user file
	user_file->edit(new_data);
	user_file->close();

	// Update current session data
	response["first_name"] = new_first_name;
	response["last_name"] = new_last_name;
	response["email"] = new_email;
	sFlash(response, "Successfully updated user information.");
	return response;
}

//////////////////////////////////
// Files Accessed: users(read)
json authUser(string email, string hashed_password) {
	json response;
	string line;

	user_file->open(fstream::in | fstream::out);
	while(user_file->read(line)){
		stringstream ss(line);
		int l_id;
		string l_email, l_hashed_password, l_first_name, l_last_name;
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		// Check email and password
		if(email.compare(l_email) == 0 && hashed_password.compare(l_hashed_password) == 0){
			// Send session data
			response["user_id"] = l_id;
			response["email"] = l_email;
			response["first_name"] = l_first_name;
			response["last_name"] = l_last_name;
			sFlash(response, "Successfully Logged in.");
			user_file->close();
			return response;
		}
	}

	wFlash(response, "Bad log in.");
	user_file->close();
	return response;
}

//////////////////////////////////
// Files Accessed: users(read)
// Exceptions: abort(400) Bad Request!
json getUser(int user_id){
	json user;
	string line;

	user_file->open(fstream::in);
	while(user_file->read(line)){
		stringstream ss(line);
		int l_id;
		string l_email, l_hashed_password, l_first_name, l_last_name;
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		if(user_id == l_id && !l_email.empty()){
			user["user_id"] = l_id;
			user["email"] = l_email;
			user["first_name"] = l_first_name;
			user["last_name"] = l_last_name;

			user_file->close();
			user["success"] = true;
			return user;
		}
	}

	user_file->close();
	// Returns the error page
	abort(400);
	return user;
}

//////////////////////////////////
// Files Accessed: users(read), follows(read)
// Exceptions: abort(400) Bad Request!
json getProfile(int user_id, int profile){
	// Checks whether or not the current user is following 
	//the current profile. Needed for displaying the right button.
	bool following = false;
	json followees = getFollowees(user_id)["followees"];
	for(json followee : followees){
		if(profile == followee["user_id"]){
			following = true;
			break;
		}
	}
	json user = getUser(profile);
	user["following"] = following;
	return user;
}

//////////////////////////////////
// Files Accessed: messages(write)
// Exceptions: format_string/int
json postMessage(int user_id, string username, string message) {
	json response;

	// Message length restraints
	int length = message.length();
	if(!length){
		wFlash(response, "Your message is empty.");
		return response;
	}
	if(length > MSG_LEN){
		wFlash(response, "Your message was too long. " + to_string(length) + " characters.");
		return response;
	}

	time_t current_time = time(NULL);
	// Removes \n characters so it won't mess with the file structure
	replace(message.begin(), message.end(), '\n', ' ');

	msg_file->open(fstream::in | fstream::out);
	msg_file->insert(format_int(user_id, ID_LEN) + "\t" 
		+ format_int(current_time, TIME_LEN) + "\t" 
		+ format_string(username, NAME_CHAR_LIMIT) + "\t"
		+ format_string(message, MSG_LEN));
	msg_file->close();
	iFlash(response, "Message posted.");
	return response;
}

//////////////////////////////////
// Files Accessed: messages(read)
json getMessagesBy(int user_id){
	json response;
	json messages;	// Vector of found messages

	string line;
	time_t timestamp, 
	current_time = time(NULL);
	int l_id;

	msg_file->open(fstream::in);
	while(msg_file->read(line)){
		stringstream ss(line);
		string l_first_name, l_last_name, l_message;
		ss  >> l_id >> timestamp >> l_first_name >> l_last_name;
		getline(ss, l_message);

		if(user_id == l_id){
			// Build message based on found line
			json message;
			message["user_id"] = l_id;
			message["t"] = timestamp; // Unformated time for sorting
			message["time"] = format_timestamp(current_time, timestamp);
			message["first_name"] = l_first_name;
			message["last_name"] = l_last_name;
			message["message"] = l_message;
			messages.push_back(message);
		}
	}
	msg_file->close();

	// Sort messages based on timestamp
	sort(messages.begin(), messages.end(), 
		[](const json m1, const json m2)->bool{
			return m1["t"] > m2["t"];
		});
	response["messages"] = messages;

	return response;
}

//////////////////////////////////
// Files Accessed: messages(read)
json getMessagesFeed(int user_id){
	json response;
	json messages;
	json followees = getFollowees(user_id)["followees"];

	string line;
	time_t timestamp, 
	current_time = time(NULL);
	int l_id;

	msg_file->open(fstream::in);
	while(msg_file->read(line)){
		stringstream ss(line);
		string l_first_name, l_last_name, l_message;
		ss >> l_id >> timestamp >> l_first_name >> l_last_name;
		getline(ss, l_message);

		for(json followee : followees){
			if(l_id == followee["user_id"]){
				json message;
				message["user_id"] = l_id;
				message["t"] = timestamp; // Unformated time for sorting
				message["time"] = format_timestamp(current_time, timestamp);
				message["first_name"] = l_first_name;
				message["last_name"] = l_last_name;
				message["message"] = l_message;
				messages.push_back(message);
			}
		}
	}
	msg_file->close();

	// Sort messages based on timestamp
	sort(messages.begin(), messages.end(), 
		[](const json m1, const json m2)->bool{
			return m1["t"] > m2["t"];
		});
	response["messages"] = messages;

	return response;
}

//////////////////////////////////
// Files Accessed: users(read)
json getUsers(int user_id){
	json response;
	json users;
	json followees = getFollowees(user_id)["followees"];

	string line;	

	user_file->open(fstream::in);
	while(user_file->read(line)){
		stringstream ss(line);
		int l_id;
		string l_email, l_hashed_password;
		ss >> l_id >> l_email >> l_hashed_password;

		if(user_id != l_id && !l_email.empty()){
			bool following = false;
			for(json followee : followees){
				if(l_id == followee["user_id"]){
					following = true;
					break;
				}
			}

			json user;
			user["user_id"] = l_id;
			user["email"] = l_email;
			user["following"] = following;
			users.push_back(user);
		}
	}
	user_file->close();

	response["users"] = users;

	return response;
}

//////////////////////////////////
// Files Accessed: follows(read/write)
json userFollowUser(int follower, int followee){
	json response;
	string line;

	// Check if the user has been followed already.
	flw_file->open(fstream::in | fstream::out);
	while(flw_file->read(line)){
		stringstream ss(line);
		int l_follower, l_followee;
		ss >> l_follower >> l_followee;
		if(follower == l_follower && followee == l_followee){
			flw_file->close();
			wFlash(response, "You're alreadying following that user.");
			return response;
		}
	}

	flw_file->insert(format_int(follower, ID_LEN) + "\t" 
					+ format_int(followee, ID_LEN));
	flw_file->close();

	sFlash(response, "Follow user successful!");
	return response;
}

//////////////////////////////////
// Files Accessed: follows(read/write)
json userUnfollowUser(int follower, int followee){
	json response;
	string line;

	flw_file->open(fstream::in | fstream::out);
	while(flw_file->read(line)){
		stringstream ss(line);
		int l_follower, l_followee;
		ss >> l_follower >> l_followee;
		if(follower == l_follower && followee == l_followee){
			flw_file->remove();
			flw_file->close();
			sFlash(response, "Unfollow user successful!");
			return response;
		}
	}
	flw_file->close();

	wFlash(response, "You're not following that user.");
	return response;
}

//////////////////////////////////
// Files Accessed: users(read), follows(read)
json getFollowees(int user_id){
	json response, followees;
	string line;

	flw_file->open(fstream::in);
	while(flw_file->read(line)){
		stringstream ss(line);
	int l_follower, l_followee;
		ss >> l_follower >> l_followee;
		if(user_id == l_follower){
			json followee;
			try{
				json user = getUser(l_followee);
				followee["user_id"] = l_followee;
				followee["first_name"] = user["first_name"];
				followee["last_name"] = user["last_name"];
				followees.push_back(followee);
			}
			catch(...){
				// If user does not exist
				continue;
			}
		}
	}
	flw_file->close();

	response["followees"] = followees;
	return response;
}

//////////////////////////////////
// Files Accessed: users(read), follows(read)
json getFollowers(int user_id){
	json response, followers;
	string line;

	flw_file->open(fstream::in);
	while(flw_file->read(line)){
		stringstream ss(line);
		int l_follower, l_followee;
		ss >> l_follower >> l_followee;
		if(user_id == l_followee){
			json follower;
			try{
				json user = getUser(l_follower);
				follower["user_id"] = l_follower;
				follower["first_name"] = user["first_name"];
				follower["last_name"] = user["last_name"];
				followers.push_back(follower);
			}
			catch(...){
				// If user does not exist
				continue;
			}
		}
	}
	flw_file->close();
	response["followers"] = followers;
	return response;
}