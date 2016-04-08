#include "main.h"

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

		thread t(processConnection, sock_fd);
		t.detach();
	}

	return 0;
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

void processConnection(int sock_fd){
	// read request
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

// route request based on 'type' field
json processRequest(json request) {
	json response;
	response["success"] = true;

	try{

		if(request["type"] == "createUser"){
			lock_guard<mutex> lock(user_file.mx);
			response = createUser(request["data"]["email"], request["data"]["first_name"],
				request["data"]["last_name"], request["data"]["hashed_password"]);
		}
		else if(request["type"] == "deleteUser"){
			lock_guard<mutex> lock(user_file.mx);
			response = deleteUser(request["data"]["user_id"], request["data"]["hashed_password"]);
		}
		else if(request["type"] == "editUser"){
			lock_guard<mutex> user_lock(user_file.mx);
			response = editUser(request["data"]["user_id"],
								request["data"]["email"],
								request["data"]["first_name"],
								request["data"]["last_name"],
								request["data"]["hashed_password"],
								request["data"]["new_password"]);
		}
		else if(request["type"] == "authUser"){
			lock_guard<mutex> user_lock(user_file.mx);
			response = authUser(request["data"]["email"], request["data"]["hashed_password"]);
		}
		else if(request["type"] == "getUser"){
			lock_guard<mutex> user_lock(user_file.mx);
			response = getUser(request["data"]["user_id"]);
		}
		else if(request["type"] == "getProfile"){
			lock_guard<mutex> flw_lock(flw_file.mx);
			lock_guard<mutex> user_lock(user_file.mx);
			response = getProfile(request["data"]["user_id"], request["data"]["profile"]);
		}
		else if(request["type"] == "postMessage"){
			lock_guard<mutex> msg_lock(msg_file.mx);
			int user_id = request["data"]["user_id"];
			response = postMessage(user_id, request["data"]["username"], request["data"]["message"]);
		}
		else if(request["type"] == "getMessagesBy"){
			lock_guard<mutex> msg_lock(msg_file.mx);
			response = getMessagesBy(request["data"]["user_id"]);
		}
		else if(request["type"] == "getMessagesFeed"){
			lock_guard<mutex> flw_lock(flw_file.mx);
			lock_guard<mutex> user_lock(user_file.mx);
			lock_guard<mutex> msg_lock(msg_file.mx);
			response = getMessagesFeed(request["data"]["user_id"]);
		}
		else if(request["type"] == "getUsers"){
			lock_guard<mutex> flw_lock(flw_file.mx);
			lock_guard<mutex> user_lock(user_file.mx);
			int user_id = request["data"]["user_id"];
			response = getUsers(user_id);
		}
		else if(request["type"] == "userFollowUser"){
			lock_guard<mutex> flw_lock(flw_file.mx);
			response = userFollowUser(request["data"]["follower"], request["data"]["followee"]);
		}
		else if(request["type"] == "userUnfollowUser"){
			lock_guard<mutex> flw_lock(flw_file.mx);
			response = userUnfollowUser(request["data"]["follower"], request["data"]["followee"]);
		}
		else if(request["type"] == "getFollowees"){
			lock_guard<mutex> flw_lock(flw_file.mx);
			lock_guard<mutex> user_lock(user_file.mx);
			response = getFollowees(request["data"]["user_id"]);
		}
		else if(request["type"] == "getFollowers"){
			lock_guard<mutex> flw_lock(flw_file.mx);
			lock_guard<mutex> user_lock(user_file.mx);
			response = getFollowers(request["data"]["user_id"]);
		}

		else{
			cout << "Request type could not be processed: " << request["type"] << endl;
		}

	}
	catch(int errcode){
		response.clear();
		response["success"] = false;
		response["errcode"] = errcode;
	}
	catch(char const* e){
		response.clear();
		response["success"] = false;
		string err_msg = "An exception has been thrown: ";
		err_msg += e;
		dFlash(response, err_msg);
	}
	catch(string &s){
		response.clear();
		response["success"] = false;
		string err_msg = "An exception has been thrown: ";
		err_msg += s;
		dFlash(response, err_msg);
	}
	catch(exception &e){
		response.clear();
		response["success"] = false;
		string err_msg = "An exception has been thrown: ";
		err_msg += e.what();
		cout << err_msg << endl;
		dFlash(response, err_msg);
	}
	return response;
}

//////////////////////////////////
// Files Accessed: users(read/write)
// Exceptions: format_string/int
json createUser(string email, string first_name, string last_name, string hashed_password) {
	json response;
	string line;
	int id = 0, l_id;
	string l_email, l_hashed_password, l_first_name, l_last_name;

	user_file.open(fstream::in | fstream::out);
	while(user_file.read(line)){
		stringstream ss(line);
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		if(email.compare(l_email) == 0){
			user_file.close();
			dFlash(response, "That email is already taken.");
			return response;
		}
	}
	// Retrieve ID located at EOF
	user_file.fs.clear();
	user_file.fs.seekg(-ID_LEN, fstream::end);
	user_file.fs >> id;
	user_file.fs.clear();

	string name = first_name + " " + last_name;
	bool filled = user_file.insert(format_int(id, ID_LEN) + "\t" 
		+ format_string(email, EMAIL_CHAR_LIMIT) + "\t" 
		+ hashed_password + "\t"
		+ format_string(name, NAME_CHAR_LIMIT), -ID_LEN);

	// If user data got inserted at EOF, 
	// then the next ID has been overwritten.
	// Otherwise, seek ID_LEN bytes from EOF
	// to overwrite the next ID.
	if(filled){
		user_file.fs.clear();
		user_file.fs.seekp(-ID_LEN, fstream::end);
	}

	user_file.fs << format_int(id + 1, ID_LEN);

	user_file.close();
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
	string l_email, l_hashed_password, l_first_name, l_last_name;

	user_file.open(fstream::in | fstream::out);

	while(user_file.read(line)){
		stringstream ss(line);
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		if(user_id == l_id){
			if(hashed_password.compare(l_hashed_password) == 0){
				removed = user_file.remove();
				break;
			}
			else{
				user_file.close();
				wFlash(response, "Incorrect password.");
				return response;
			}
		}
	}
	user_file.close();

	// If user not found for some reason
	if(!removed){
		wFlash(response, "Failed to delete user.");
		return response;
	}

	sFlash(response, "Successfully deleted User.");
	return response;
}

//////////////////////////////////
// Files Accessed: users(read/write)
json editUser(int user_id, string email, string first_name, string last_name, string hashed_password, string new_password){
	json response;
	string line;

	int l_id;
	string l_email, l_hashed_password, l_first_name, l_last_name;

	user_file.open(fstream::in | fstream::out);
	while(user_file.read(line)){
		stringstream ss(line);
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		if(user_id == l_id){
			if(hashed_password.compare(l_hashed_password) != 0){
				user_file.close();
				dFlash(response, "Invaid Password.");
				return response;
			}
			response["first_name"] = l_first_name;
			response["last_name"] = l_last_name;
			response["email"] = l_email;
			break;
		}
	}

	string new_data = format_int(user_id, ID_LEN) + '\t';

	string new_email = (email.empty()) ? l_email : email;
	new_data += format_string(new_email, EMAIL_CHAR_LIMIT) + '\t';

	new_password = (new_password.empty()) ? l_hashed_password : new_password;
	new_data += new_password + '\t';

	if(!first_name.empty() || !last_name.empty()){
		string current_first_name = response["first_name"];
		string current_last_name = response["last_name"];

		string new_first_name = (!first_name.empty()) ? first_name : current_first_name;
		response["first_name"] = new_first_name;

		string new_last_name = (!last_name.empty()) ? last_name : current_last_name;
		response["last_name"] = new_last_name;

		string new_full_name = new_first_name + " " + new_last_name;
		new_data += format_string(new_full_name, NAME_CHAR_LIMIT);
	}
	cout << "Length: " << new_data.length() << endl;
	user_file.edit(new_data);
	user_file.close();
	sFlash(response, "Successfully updated user information.");
	return response;
}

//////////////////////////////////
// Files Accessed: users(read)
json authUser(string email, string hashed_password) {
	json response;

	string line;

	user_file.open(fstream::in | fstream::out);
	while(user_file.read(line)){
		stringstream ss(line);
		int l_id;
		string l_email, l_hashed_password, l_first_name, l_last_name;
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		if(email.compare(l_email) == 0 && hashed_password.compare(l_hashed_password) == 0){
			response["user_id"] = l_id;
			response["email"] = l_email;
			response["first_name"] = l_first_name;
			response["last_name"] = l_last_name;
			sFlash(response, "Successfully Logged in.");
			user_file.close();
			return response;
		}
	}

	wFlash(response, "Bad log in.");
	user_file.close();
	return response;
}

//////////////////////////////////
// Files Accessed: users(read)
// Exceptions: abort(400) Bad Request!
json getUser(int user_id){
	json user;
	string line;

	user_file.open(fstream::in);
	while(user_file.read(line)){
		stringstream ss(line);
		int l_id;
		string l_email, l_hashed_password, l_first_name, l_last_name;
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;
		if(user_id == l_id && !l_email.empty()){
			user["user_id"] = l_id;
			user["email"] = l_email;
			user["first_name"] = l_first_name;
			user["last_name"] = l_last_name;

			user_file.close();
			user["success"] = true;
			return user;
		}
	}

	user_file.close();
	abort(400);
	return user;
}

//////////////////////////////////
// Files Accessed: users(read/write), follows(read/write)
// Exceptions: abort(400) Bad Request!
json getProfile(int user_id, int profile){
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

	int length = message.length();
	if(length > MSG_LEN){
		wFlash(response, "Your message was too long. " + to_string(length) + " characters.");
		return response;
	}

	time_t current_time = time(NULL);
	replace(message.begin(), message.end(), '\n', ' ');

	msg_file.open(fstream::out | fstream::app);

	msg_file.insert(format_int(user_id, ID_LEN) + "\t" 
		+ format_int(current_time, TIME_LEN) + "\t" 
		+ format_string(username, NAME_CHAR_LIMIT) + "\t"
		+ format_string(message, MSG_LEN));

	msg_file.close();
	iFlash(response, "Message posted.");
	return response;
}

//////////////////////////////////
// Files Accessed: messages(read)
json getMessagesBy(int user_id){
	json response;
	json messages;

	string line;
	time_t timestamp, 
	current_time = time(NULL);
	int l_id;
	string l_first_name, l_last_name, l_message;

	msg_file.open(fstream::in);
	while(msg_file.read(line)){
		stringstream ss(line);
		ss  >> l_id >> timestamp >> l_first_name >> l_last_name;
		getline(ss, l_message);

		if(user_id == l_id){
			json message;
			message["user_id"] = l_id;
			message["time"] = format_timestamp(current_time, timestamp);
			message["first_name"] = l_first_name;
			message["last_name"] = l_last_name;
			message["message"] = l_message;
			messages.push_back(message);
		}
	}

	msg_file.close();

	reverse(messages.begin(), messages.end());
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
	string l_first_name, l_last_name, l_message;

	msg_file.open(fstream::in);
	while(msg_file.read(line)){
		stringstream ss(line);
		ss >> l_id >> timestamp >> l_first_name >> l_last_name;
		getline(ss, l_message);

		for(json followee : followees){
			if(l_id == followee["user_id"]){
				json message;
				message["user_id"] = l_id;
				message["time"] = format_timestamp(current_time, timestamp);
				message["first_name"] = l_first_name;
				message["last_name"] = l_last_name;
				message["message"] = l_message;
				messages.push_back(message);
			}
		}
	}
	msg_file.close();

	reverse(messages.begin(), messages.end());
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

	user_file.open(fstream::in);
	while(user_file.read(line)){
		stringstream ss(line);
		int l_id;
		string l_email, l_hashed_password, l_first_name, l_last_name;
		ss >> l_id >> l_email >> l_hashed_password >> l_first_name >> l_last_name;

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
	user_file.close();

	response["users"] = users;

	return response;
}

//////////////////////////////////
// Files Accessed: follows(read/write)
json userFollowUser(int follower, int followee){
	json response;

	string line;

	int l_follower, l_followee;

	flw_file.open(fstream::in | fstream::out);
	while(flw_file.read(line)){
		stringstream ss(line);
		ss >> l_follower >> l_followee;
		if(follower == l_follower && followee == l_followee){
			flw_file.close();
			wFlash(response, "You're alreadying following that user.");
			return response;
		}
	}
	flw_file.fs.clear();

	flw_file.insert(format_int(follower, ID_LEN) + "\t" 
					+ format_int(followee, ID_LEN));
	flw_file.close();

	sFlash(response, "Follow user successful!");
	return response;
}

//////////////////////////////////
// Files Accessed: follows(read/write)
json userUnfollowUser(int follower, int followee){
	json response;
	string line;
	int l_follower, l_followee;

	flw_file.open(fstream::in | fstream::out);
	while(flw_file.read(line)){
		stringstream ss(line);
		ss >> l_follower >> l_followee;
		if(follower == l_follower && followee == l_followee){
			flw_file.remove();
			flw_file.close();
			sFlash(response, "Unfollow user successful!");
			return response;
		}
	}
	flw_file.close();

	wFlash(response, "You're not following that user.");
	return response;
}

//////////////////////////////////
// Files Accessed: users(read), follows(read)
json getFollowees(int user_id){
	json response;
	json followees;

	string line;
	int l_follower, l_followee;

	flw_file.open(fstream::in);
	while(flw_file.read(line)){
		stringstream ss(line);
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
	flw_file.close();

	response["followees"] = followees;
	return response;
}

//////////////////////////////////
// Files Accessed: users(read), follows(read)
json getFollowers(int user_id){
	json response;
	json followers;

	string line;
	int l_follower, l_followee;

	flw_file.open(fstream::in);
	while(flw_file.read(line)){
		stringstream ss(line);
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
	flw_file.close();
	response["followers"] = followers;
	return response;
}