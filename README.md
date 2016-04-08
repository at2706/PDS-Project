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


Written by:
  - Minhtri Tran
  - Andy Tang

TODO: EditUser function.
TODO: Finish SharedFile class so fstream fs can be private member.
TODO: SharedFile remove(): insert new blank position in ascending order. 
TODO: Finish commenting every function.