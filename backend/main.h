#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>
#include "json.hpp"
#include "fileAssistant.h"

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

// User data size: 256 bytes
// Max Users: 9999 (2.56MB)
// Character limits also enforced in javascript
#define ID_LEN 4
#define EMAIL_CHAR_LIMIT 60
#define NAME_CHAR_LIMIT 60

// Message size: 178 bytes
#define MSG_LEN 100


int setUpServer(int server_port);
void processConnection(int sock_fd);
json processRequest(json request);

json createUser(string email, string first_name, string last_name, string hashed_password);
json deleteUser(int user_id, string hashed_password);
json editUser(int user_id, string email, string first_name, string last_name, string hashed_password, string new_password);
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

void flash(json &r,  const string &message, const string &type);
void sFlash(json &r, const string &message);
void iFlash(json &r, const string &message);
void wFlash(json &r, const string &message);
void dFlash(json &r, const string &message);
void abort(int errcode);

string format_string(string &str, uint width);
string format_int(uint i, uint width);
string format_timestamp(time_t now, uint t);