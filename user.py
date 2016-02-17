import os


class User:

    def __init__(self, id):
        try:
            self.id = id
            self.data = {}
            with open("db/users/" + str(id), "r") as user_file:
                for line in user_file:
                    (key, val) = line.strip('\n').split(":")
                    self.data[key] = val

            self.name = self.data['name']
            self.email = self.data['email']
        except Exception:
            raise Exception

    @staticmethod
    def delete(id):
        os.remove("db/users/" + str(id))
        with open("db/user_index", "r") as file:
            users = file.readlines()
        with open("db/user_index", "w") as file:
            for line in users:
                attr = line.strip('\n').split("\t")
                if attr[0] != id:
                    file.write(line)
