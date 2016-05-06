# PDS-Project: Tweezer

Functionalities we've implemented:
  - User registration
  - User login
  - Post message
  - Show messages you've wrote
  - Show messages written by people you followed
  - Follow user
  - Unfollow user
  - Edit user profile
  - Delete user account

Special notes:
  - We store hashed passwords instead of in plaintext!
  - We used files to store persistent data
  - We used bootstrap

Part 2
  - Messages between the web server and the application data server are formatted with JSON. The web server sends a request containing a type and other necessary data. The data server will send back the request data if possible. The data server's response will always have a 'success' flag.

  - Data inserted into files now have a fixed size. This allows users to edit various data. Also, when deleting data, the entire file no longer needs to be rewritten. Instead, it will be overwritten with the * character. Inserting will fill the first occurance of blank data.
      File structures listed with the following format: data: type(size) .. type(size)
        users:    id(4) email(60) hashed_password(128)  full_name(60)
        follows:  follower(4)     followee(4)
        messages: id(4) time(10)  full_name(60)         message(100)

  - Form validation: character limits.
    User Delete: now requires password.
    Timestamps in messages

Part 3
  - If multiple locks are needed, they are taken in this order: flw_file, user_file, msg_file; this ensures that we don't have a deadlock situation

Part 4
  - Specify the replica manger's (RM) port number using the first command line argument
    - it is assumed RMs have the same IP address: 127.0.0.1
  - Initiate three Ms using these ports in order: 13000, 13001, 13002
  - RMs track the state of other RMs: replica_manager(port, alive, heartbeat)
    - port: what port the RM is listening on
    - alive: is this RM alive?
    - heartbeat: did the slave RM respond to heartbeat message?
  - The Primary RM sends heartbeat messages at fixed intervals to determine if the slave RM is still alive.
  - Slave RMs are considered dead when they do not respond to the heartbeat message after a grace period.
  - When a RM dies the next RM in the vector will become the primary RM.
    - The vector of RMs are syncronized
  - The frontend (FE) will follow a similar process when a RM dies
  - The FE will only send requests to the known primary RM
  - Only write requests are transmitted to the slave RMs


Written by:
  - Minhtri Tran
  - Andy Tang