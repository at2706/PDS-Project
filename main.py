import os
import hashlib
from flask import Flask, render_template, request, redirect, flash, url_for, abort, session
from jinja2 import TemplateNotFound
from werkzeug import secure_filename
from user import User


ALLOWED_EXTENSIONS = set(['png', 'jpg', 'jpeg', 'gif'])
app = Flask(__name__)
app.secret_key = os.urandom(24)


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
            return redirect(url_for('home'))

    return render_template(
        'register.html',
        form=request.form
    )


@app.route("/login", methods=['post', 'get'])
def login():
    if 'userid' in session:
        flash("You're already logged in!", "warning")
        return redirect(url_for('home'))
    if request.method == 'POST':
        email = request.form["email"]
        password = request.form["password"]
        if login_user(email, password):
            return redirect(url_for('home'))

    return render_template(
        'login.html',
        form=request.form
    )


@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for('home'))


@app.route('/user/<int:user_id>')
def profile(user_id):
    try:
        return render_template(
            'user.html',
            user=User(user_id),
            current_user=(int(session['userid']) == user_id)
        )
    except Exception:
        abort(404)


@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def catch_all(path):
    try:
        return render_template(
            path + '.html'
        )
    except TemplateNotFound:
        abort(404)


@app.errorhandler(400)
@app.errorhandler(401)
@app.errorhandler(403)
@app.errorhandler(404)
def error(e):
    msg = str(e.code) + ' '
    if e.code == 400:
        msg += 'Bad Request!'
    elif e.code == 401:
        msg += 'Unauthorized!'
    elif e.code == 403:
        msg += 'Forbidden!'
    elif e.code == 404:
        msg += 'Page Not Found!'
    else:
        msg += 'Something Bad Happened'
    return render_template(
        'error.html',
        title='Error ' + str(e.code),
        message=msg), e.code


def create_user(email, first_name, last_name, pwd1, pwd2):
    if not email:
        flash("Invalid Email.", "warning")
        return False
    if pwd1 != pwd2:
        flash("Passwords do not match.", "warning")
        return False

    # open and write user to file
    with open("db/user_index", "a+") as file:
        id = 1
        for id, line in enumerate(file, 1):
            attr = line.strip('\n').split("\t")
            if attr[1] == email:
                flash("This email is already registered.", "warning")
                return False
            id += 1
        file.write(str(id) + "\t" + email + "\t" + hashlib.sha512(pwd1).hexdigest() + "\n")
        with open("db/users/" + str(id), "w+") as user_file:
            user_file.write("name:" + first_name + " " + last_name + "\n")
            user_file.write("email:" + email + "\n")

    flash("Successfully Created User.", "info")
    return True


def login_user(email, password):
    with open("db/user_index", "r") as file:
        for line in file:
            attr = line.strip('\n').split("\t")
            if attr[1] == email and attr[2] == hashlib.sha512(password).hexdigest():
                user = User(attr[0])
                session['userid'] = user.id
                session['username'] = user.name
                session['useremail'] = user.email
                flash("Logged in.", "success")
                return True

    flash("Bad Login.", "warning")
    return False


app.run("127.0.0.1", 5000, debug=True)
