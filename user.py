import os


class User:

    def __init__(self, id):
        self.path = "db/users/" + str(id)
        self.data = {'id': id}
        try:
            with self.open_user() as user_file:
                for line in user_file:
                    (key, val) = line.strip('\n').split(":")
                    self.data[key] = val
        except IOError:
            raise IOError

    def open_user(self, mode='r'):
        return open(self.path, mode)

    def commit(self):
        password = User.delete(self.data['id'])
        with self.open_user('w+') as file:
            for key, value in self.data.items():
                file.write(str(key) + ":" + str(value) + "\n")
        with open("db/user_index", "a") as file:
            file.write(str(self.data['id']) + "\t" + self.data['email'] + "\t" + password + "\n")

    @staticmethod
    def delete(id):
        password = ""
        os.remove("db/users/" + str(id))
        with open("db/user_index", "r") as file:
            users = file.readlines()
        with open("db/user_index", "w") as file:
            for line in users:
                attr = line.strip('\n').split("\t")
                if attr[0] != str(id):
                    file.write(line)
                else:
                    password = attr[2]
        return password
