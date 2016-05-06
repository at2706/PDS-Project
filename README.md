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
  - We implemented passive (primary) backup replication
  - Specify the replica manger's (RM) port number using the first command line argument
    - it is assumed RMs have the same IP address: 127.0.0.1
  - Initiate three RMs using these ports in order: 13000, 13001, 13002
    - The initial primary RM (13000) must be the first server to be started
    - The other RMs must be started witihn the grace period (or they will be assumed dead by the primary)
    - This is a static system (fixed set of RMs)
    - Number of FEs can be dynamic though
  - RMs track the state of other RMs: replica_manager(port, alive, heartbeat)
    - port: what port the RM is listening on
    - alive: is this RM alive?
    - heartbeat: did this RM recently send a heartbeat message to us?
  - Slave RMs sends heartbeat messages at fixed intervals to the primary RM to determine if the primary RM is still alive.
  - When a primary RM is dead, the slave RMs will determine who will be the next primary RM
    - The next RM in the vector will become the primary RM if it is alive
    - The list is ordered and everyone (both RMs and FEs) knows this list beforehand
  - The primary RM will consider a slave RM to be dead if the primary RM doesn't receive a heartbeat message from it within a period a time
  - The frontend (FE) will send requests to the first RM in list. If fail, send to the next RM in list
    - With our system, the first alive RM in the list is always the primary RM
  - Backup procedure:
    1. FE issues request to the primary RM
    2. Primary RM executes request
    3. If request does a write, the primary RM transmits the request all alive slave RMs
    4. Primary RM responds back to FE


Written by:
  - Minhtri Tran
  - Andy Tang
