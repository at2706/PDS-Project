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
  - Messages between the web server and the application data server are formatted with JSON. The web server sends a request containing a type and other necessary data. The data server will send back the request data if possible. The data center's response will always have a success flag.

  - Data inserted into files now have a fixed size. This allows users to edit various data. Also, when deleting data, the entire file no longer needs to be rewritten. Instead, it will be overwritten with the * character. Inserting will fill the first occurance of blank data.
      File structures listed with the following format: data: type(size) .. type(size)
        users: id(4) email(60) hashed_password(128) full_name(60)
        follows: follower(4) followee(4)
        messages: time(10) id(4) full_name(60) message(100)
    Deleting users will remove user data. However, messages sent by that user will persist. User IDs will never be reused.

  - Form validation: character limits.
    User Delete: now requires password.
    Timestamps in messages

Written by:
  - Minhtri Tran
  - Andy Tang


TODO: delete user's following data.