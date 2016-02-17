import os


class User:

    def __init__(self, id):
        self.id = id
        self.path = "db/users/" + str(id)
        self.data = {}
        try:
            with self.open_user() as user_file:
                for line in user_file:
                    (key, val) = line.strip('\n').split(":")
                    self.data[key] = val
        except IOError:
            raise IOError

        self.name = self.data['name']
        self.email = self.data['email']

    def open_user(self, mode='r'):
        return open(self.path, mode)

    def commit(self):
        with self.open_user('w') as file:
            for key, value in self.data:
                file.write(str(key) + str(value) + "\n")

    @staticmethod
    def delete(id):
        # Keep user info alive ?
        # os.remove("db/users/" + str(id))
        with open("db/user_index", "r") as file:
            users = file.readlines()
        with open("db/user_index", "w") as file:
            for line in users:
                attr = line.strip('\n').split("\t")
                if int(attr[0]) != id:
                    file.write(line)
