import os
import hashlib

from flask import Flask, render_template, request, redirect, flash
import flask.ext.login as flask_login


app = Flask(__name__)
app.secret_key = "allyourbasesarebelongtous"

login_manager = flask_login.LoginManager()
login_manager.init_app(app)

class User(flask_login.UserMixin):
    pass


@login_manager.user_loader
def user_loader(email):
    # if email not in users:
    #     return

    user = User()
    user.id = email
    user.first = "hello lol"
    return user


@app.route("/")
@app.route("/home")
def home():
    return render_template(
        'index.html'
    )


@app.route("/register", methods=['post', 'get'])
def register():
    if request.method == 'POST':
        email = request.form["email"]
        first_name = request.form["first_name"]
        last_name = request.form["last_name"]
        pwd1 = request.form["password1"]
        pwd2 = request.form["password2"]
        if create_user(email, first_name, last_name, pwd1, pwd2):
            return home()

    return render_template(
        'register.html',
        form=request.form
    )


@app.route("/login", methods=['post', 'get'])
def login():
    if request.method == 'POST':
        email = request.form["email"]
        password = request.form["password"]
        if login_user(email, password):
            return home()
    return render_template(
        'login.html',
        form=request.form
    )


@app.route("/logout")
def logout(): 
    flask_login.logout_user()
    flash("Logged out.", "success")
    return home()

@app.errorhandler(404)
def page_not_found(e):
    return render_template(
        '404.html'), 404


def create_user(email, first_name, last_name, pwd1, pwd2):
    if not email:
        flash("Invalid Email.", "warning")
        return False
    if pwd1 != pwd2:
        flash("Passwords do not match.", "warning")
        return False
    


    # open and write user to file
    with open("db/users", "a") as file:
        file.write(email + "\t" + first_name + "\t" + last_name + "\t" + hashlib.sha512(pwd1).hexdigest() + "\n")
        file.close()

    flash("Successfully Created User.", "info")
    return True


def login_user(email, password):
    with open("db/users", "r") as file:
        for line in file:
            attr = line.strip('\n').split("\t")
            if attr[0] == email and attr[3] == hashlib.sha512(password).hexdigest():
                user = User()
                user.id = email
                flask_login.login_user(user)

                flash("Logged in.", "success")
                return True
    
    flash("Bad Login.", "warning")
    return False


app.run("127.0.0.1", 5000, debug=True)
