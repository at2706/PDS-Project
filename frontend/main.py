import os
import hashlib
import socket
import json
from flask import Flask, render_template, request, redirect, flash, url_for, abort, session
from jinja2 import TemplateNotFound
from werkzeug import secure_filename
from user import User

BACKEND_IP = "localhost"
BACKEND_PORT = 13000

app = Flask(__name__)
app.secret_key = os.urandom(24)


@app.route("/", methods=['post', 'get'])
@app.route("/home", methods=['post', 'get'])
def home():
    if 'id' not in session:
        return redirect(url_for('login'))

    if request.method == 'POST':
        message = request.form['message']
        if post_message(session['id'], session['username'], message):
            return redirect(url_for('home'))

    return render_template(
        'index.html',
        messages=getMessagesBy(session['id']),
        feed=getMessagesFeed(session['id']),
        followees=getFollowees(session['id']),
        followers=getFollowers(session['id']),
        user=User(session['id'])
    )


@app.route("/register", methods=['post', 'get'])
def register():
    if request.method == 'POST':
        email = request.form['email']
        first_name = request.form['first_name']
        last_name = request.form['last_name']
        pwd1 = request.form['password1']
        pwd2 = request.form['password2']
        if create_user(email, first_name, last_name, pwd1, pwd2):
            return redirect(url_for('home'))

    return render_template(
        'register.html',
        form=request.form
    )


@app.route("/login", methods=['post', 'get'])
def login():
    if 'id' in session:
        flash("You're already logged in!", "warning")
        return redirect(url_for('home'))
    if request.method == 'POST':
        email = request.form['email']
        password = request.form['password']
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


@app.route("/users")
def users():
    if 'id' not in session:
        return redirect(url_for('login'))

    return render_template(
        'users.html',
        users=getUsers(session['id'])
    )


@app.route('/user/<int:user_id>')
def profile(user_id):
    if 'id' not in session:
        return redirect(url_for('login'))
    try:
        following = False
        followees = getFollowees(session['id'])
        for followee in followees:
            if int(followee['id']) == user_id:
                following = True
                break
        return render_template(
            'user.html',
            user=User(user_id),
            following=following,
            current_user=(int(session['id']) == user_id)
        )
    except Exception:
        abort(404)


@app.route('/edit/<int:user_id>', methods=['post', 'get'])
def edit(user_id):
    if int(session['id']) != user_id:
        abort(403)

    if request.method == 'POST':
        email = request.form['email']
        first_name = request.form['first_name']
        last_name = request.form['last_name']
        user = User(user_id)
        if not (email or first_name or last_name):
            flash("There are empty fields.", "warning")
        user.data['email'] = email
        user.data['name'] = first_name + " " + last_name
        user.commit()
        session['username'] = first_name + " " + last_name

        return redirect(url_for('profile', user_id=user_id))

    return render_template(
        'user_edit.html',
        form=request.form)


@app.route('/delete/<int:user_id>')
def delete(user_id):
    if int(session['id']) == user_id:
        User.delete(user_id)
        flash("User account has been removed!", "success")
        return redirect(url_for('logout'))
    else:
        flash("An error has occured.", "danger")


@app.route('/user/<int:user_id>/follow')
def follow(user_id):
    if user_follow_user(session['id'], user_id):
        flash("Follow user successful!", "success")
    else:
        flash("Failed to follow user.", "danger")
    return redirect(url_for('home'))


@app.route('/user/<int:user_id>/unfollow')
def unfollow(user_id):
    if user_unfollow_user(session['id'], user_id):
        flash("You successfully unfollowed that user!", "success")
    else:
        flash("Failed to unfollow user.", "danger")
    return redirect(url_for('home'))


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


# Send request (type: dictionary) to backend server
# Returns reponse in json
def sendRequest(request):
    sock = socket.socket()
    sock.connect((BACKEND_IP, BACKEND_PORT))
    sock.send(json.dumps(request))

    data = ""
    data_segment = None
    while data_segment != "":
        data_segment = sock.recv(4096)
        data += data_segment

    response = json.loads(data)
    sock.close()
    return response


def create_user(email, first_name, last_name, pwd1, pwd2):
    if not email:
        flash("Invalid Email.", "danger")
        return False
    if pwd1 != pwd2:
        flash("Passwords do not match.", "danger")
        return False

    request =  {
        'type': 'createUser',
        'data': {
            'email': email,
            'first_name': first_name,
            'last_name': last_name,
            'hashed_password': hashlib.sha512(pwd1).hexdigest()
        }
    }

    response = sendRequest(request)

    if response['success'] == True:
        flash("Successfully created user.", "info")
        return True
    else:
        flash("Unable to create user. Error: " + response['error_message'], "danger")
        return False


def login_user(email, password):
    with open("db/user_index", "r") as file:
        for line in file:
            attr = line.strip('\n').split("\t")
            if attr[1] == email and attr[2] == hashlib.sha512(password).hexdigest():
                user = User(attr[0])
                session['id'] = user.data['id']
                session['username'] = user.data['name']
                flash("Logged in.", "success")
                return True

    flash("Bad Login.", "warning")
    return False


def post_message(user_id, username, message):
    request =  {
        'type': 'postMessage',
        'data': {
            'user_id': user_id,
            'username': username,
            'message': message
        }
    }

    response = sendRequest(request)

    print response

    return True

    # if len(message) > 100:
    #     flash("Your message was too long. " + str(len(message)) + " Characters.", "warning")
    #     return False
    # with open("db/messages", "a") as file:
    #     file.write(user_id + "\t" + username + "\t" + message.strip().replace('\n', ' ').replace('\t', ' ') + "\n")
    #     file.close

    # flash("Successfully posted message.", "info")
    # return True


def getMessagesBy(user_id):
    messages = []
    try:
        with open("db/messages", "r") as file:
            for line in file:
                attr = line.strip('\n').split("\t")
                if attr[0] == user_id:
                    message = {}
                    message['writeid'] = attr[0]
                    message['writer'] = attr[1]
                    message['content'] = attr[2]
                    messages.append(message.copy())
        messages.reverse()
    except:
        return []

    return messages


def getMessagesFeed(user_id):
    messages = []
    try:
        followees = getFollowees(user_id)
        with open("db/messages", "r") as file:
            for line in file:
                attr = line.strip('\n').split("\t")
                if any(followee['id'] == attr[0] for followee in followees):
                    message = {}
                    message['writerid'] = attr[0]
                    message['writer'] = attr[1]
                    message['content'] = attr[2]
                    messages.append(message.copy())
        messages.reverse()
    except:
        return []

    return messages


def getUsers(user_id):
    users = []
    try:
        followees = getFollowees(user_id)
        with open("db/user_index", "r") as file:
            for line in file:
                attr = line.strip('\n').split("\t")
                if not attr[0] == user_id:
                    user = {}
                    user['id'] = attr[0]
                    user['email'] = attr[1]
                    if any(followee['id'] == attr[0] for followee in followees):
                        user['already_followed'] = True
                    else:
                        user['already_followed'] = False
                    users.append(user.copy())
    except:
        return []

    return users


def user_follow_user(follower, followee):
    if not os.path.exists("db/follows"):
        file = open("db/follows", "w")
    else:
        file = open("db/follows", "r+")
    for line in file:
        attr = line.strip('\n').split("\t")
        if attr[0] == str(follower) and attr[1] == str(followee):
            file.close()
            return True
    file.write(str(follower) + "\t" + str(followee) + "\n")
    file.close()

    return True


def user_unfollow_user(follower, followee):
    if not os.path.exists("db/follows"):
        return False

    file = open("db/follows", "r")
    lines = file.readlines()
    file.close()

    file = open("db/follows", "w")
    for line in lines:
        attr = line.strip('\n').split("\t")
        if not (attr[0] == str(follower) and attr[1] == str(followee)):
            file.write(line)
    file.close()

    return True


def getFollowees(user_id):
    followees = []
    try:
        with open("db/follows", "r") as file:
            for line in file:
                attr = line.strip('\n').split("\t")
                if attr[0] == user_id:
                    followee = {}
                    followee['id'] = attr[1]
                    followee['name'] = User(attr[1]).data['name']
                    followees.append(followee.copy())
    except:
        return []

    return followees


def getFollowers(user_id):
    followers = []
    try:
        with open("db/follows", "r") as file:
            for line in file:
                attr = line.strip('\n').split("\t")
                if attr[1] == user_id:
                    follower = {}
                    follower['id'] = attr[0]
                    follower['name'] = User(attr[0]).data['name']
                    followers.append(follower.copy())
    except:
        return []

    return followers

app.run("127.0.0.1", 5000, debug=True)
