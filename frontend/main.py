import os
import hashlib
import socket
import json
from flask import Flask, render_template, request, redirect, flash, url_for, abort, session
from jinja2 import TemplateNotFound
from werkzeug import secure_filename

BACKEND_IP = "localhost"
BACKEND_PORT = 13000
DEBUGGING = True

app = Flask(__name__)
app.secret_key = os.urandom(24)


@app.route("/", methods=['post', 'get'])
@app.route("/home", methods=['post', 'get'])
def home():
    debug("Home Fuction")
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
        user=session
    )


@app.route("/register", methods=['post', 'get'])
def register():
    debug("register Fuction")
    if 'id' in session:
        flash("You're already logged in!", "warning")
        return redirect(url_for('home'))
    if request.method == 'POST':
        email = request.form['email']
        first_name = request.form['first_name']
        last_name = request.form['last_name']
        pwd1 = request.form['password1']
        if create_user(email, first_name, last_name, pwd1):
            return redirect(url_for('home'))

    return render_template(
        'register.html',
        form=request.form
    )


@app.route("/login", methods=['post', 'get'])
def login():
    debug("login Fuction")
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
    debug("logout Fuction")
    session.clear()
    return redirect(url_for('home'))


@app.route("/users")
def users():
    debug("users Fuction")
    if 'id' not in session:
        return redirect(url_for('login'))

    return render_template(
        'users.html',
        users=getUsers(session['id'])
    )


@app.route('/user/<int:user_id>', methods=['post', 'get'])
def profile(user_id):
    debug("profile Fuction")
    if 'id' not in session:
        return redirect(url_for('login'))

    if request.method == 'POST':
        if request.form['password']:
            password = request.form['password']
            if session['id'] != user_id:
                abort(401)

            if delete_user(user_id, password):
                return logout()

    return render_template(
        'user.html',
        user=get_profile(user_id),
        current_user=(int(session['id']) == user_id)
    )


@app.route('/edit/<int:user_id>', methods=['post', 'get'])
def edit(user_id):
    debug("edit Fuction")
    if 'id' not in session:
        return redirect(url_for('login'))
    if int(session['id']) != user_id:
        abort(403)

    if request.method == 'POST':
        email = request.form['email']
        first_name = request.form['first_name']
        last_name = request.form['last_name']
        password = request.form['password']
        pwd1 = request.form['password1']
        pwd2 = request.form['password2']
        if edit_user(email, first_name, last_name, password, pwd1, pwd2):
            return redirect(url_for('profile', user_id=user_id))

    return render_template(
        'user_edit.html',
        form=request.form)


@app.route('/user/<int:user_id>/follow')
def follow(user_id):
    debug("follow Fuction")
    if 'id' not in session:
        return redirect(url_for('login'))

    user_follow_user(session['id'], user_id)
    return redirect(url_for('home'))


@app.route('/user/<int:user_id>/unfollow')
def unfollow(user_id):
    debug("unfollow Fuction")
    if 'id' not in session:
        return redirect(url_for('login'))

    user_unfollow_user(session['id'], user_id)
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
    debug("sendRequest Fuction")

    debug("Request Sent:")
    for key in request:
        debug("\t" + str(key) + " : " + str(request[key]))
    sock = socket.socket()

    try:
        sock.connect((BACKEND_IP, BACKEND_PORT))
        sock.send(json.dumps(request))

        data = ""
        data_segment = None
        while data_segment != "":
            data_segment = sock.recv(4096)
            data += data_segment

        response = json.loads(data)

        debug("\nRequest Response:")
        for key in response:
            debug("\t" + str(key) + " : " + str(response[key]))
        debug("\n")

        if all(key in response for key in ("fMessage", "fType")):
            for msg, type in zip(response["fMessage"], response["fType"]):
                flash(msg, type)

        if "errcode" in response:
            abort(response["errcode"])
    except socket.error:
        flash("Could not connect to backend !!!!", "warning")
        response = {'success': False}
    finally:
        sock.close()

    return response


def create_user(email, first_name, last_name, pwd1):
    debug("create_user Fuction")
    request = {
        'type': 'createUser',
        'data': {
            'email': email,
            'first_name': first_name,
            'last_name': last_name,
            'hashed_password': hashlib.sha512(pwd1).hexdigest()
        }
    }

    response = sendRequest(request)

    return response['success']


def edit_user(email, first_name, last_name, password, pwd1, pwd2):
    debug("edit_user Fuction")
    request = {
        'type': 'editUser',
        'data': {
            'user_id': session['id'],
            'email': email,
            'first_name': first_name,
            'last_name': last_name,
            'hashed_password': hashlib.sha512(password).hexdigest(),
            'new_password': "" if pwd1 == "" else hashlib.sha512(pwd1).hexdigest()
        }
    }

    response = sendRequest(request)

    session['username'] = response['first_name'] + " " + response['last_name']
    session['email'] = response['email']

    return response['success']


def delete_user(user_id, pwd):
    debug("delete_user Fuction")
    request = {
        'type': 'deleteUser',
        'data': {
            'user_id': user_id,
            'hashed_password': hashlib.sha512(pwd).hexdigest()
        }
    }

    response = sendRequest(request)

    return response['success']


def login_user(email, password):
    debug("login_user Fuction")
    request = {
        'type': 'authUser',
        'data': {
            'email': email,
            'hashed_password': hashlib.sha512(password).hexdigest()
        }
    }

    response = sendRequest(request)

    if response['success']:
        session['id'] = response['user_id']
        session['username'] = response['first_name'] + " " + response['last_name']
        session['email'] = response['email']

    return response['success']


# No longer needed
# def get_user(user_id):
#     debug("get_user Fuction")
#     request = {
#         'type': 'getUser',
#         'data': {
#             'user_id': user_id
#         }
#     }

#     response = sendRequest(request)

#     return response


def get_profile(user_id):
    debug("get_profile Fuction")
    request = {
        'type': 'getProfile',
        'data': {
            'user_id': session['id'],
            'profile': user_id
        }
    }

    response = sendRequest(request)

    return response


def post_message(user_id, username, message):
    debug("post_message Fuction")
    request = {
        'type': 'postMessage',
        'data': {
            'user_id': user_id,
            'username': username,
            'message': message
        }
    }

    response = sendRequest(request)

    return response['success']


def getMessagesBy(user_id):
    debug("getMessagesBy Fuction")
    request = {
        'type': 'getMessagesBy',
        'data': {
            'user_id': user_id,
        }
    }

    response = sendRequest(request)

    if response['messages'] is None:
        return []

    return response['messages']


def getMessagesFeed(user_id):
    debug("getMessagesFeed Fuction")
    debug(session)
    request = {
        'type': 'getMessagesFeed',
        'data': {
            'user_id': user_id,
        }
    }

    response = sendRequest(request)

    if response['messages'] is None:
        return []

    return response['messages']


def getUsers(user_id):
    debug("getUsers Fuction")
    request = {
        'type': 'getUsers',
        'data': {
            'user_id': user_id
        }
    }

    response = sendRequest(request)

    if response['users'] is None:
        return []

    return response['users']


def user_follow_user(follower, followee):
    debug("user_follow_user Fuction")
    request = {
        'type': 'userFollowUser',
        'data': {
            'follower': follower,
            'followee': followee
        }
    }

    response = sendRequest(request)

    return response['success']


def user_unfollow_user(follower, followee):
    debug("user_unfollow_user Fuction")
    request = {
        'type': 'userUnfollowUser',
        'data': {
            'follower': follower,
            'followee': followee
        }
    }

    response = sendRequest(request)

    return response['success']


def getFollowees(user_id):
    debug("getFollowees Fuction")
    request = {
        'type': 'getFollowees',
        'data': {
            'user_id': user_id
        }
    }

    response = sendRequest(request)

    if response['followees'] is None:
        return []

    return response['followees']


def getFollowers(user_id):
    debug("getFollowers Fuction")
    request = {
        'type': 'getFollowers',
        'data': {
            'user_id': user_id
        }
    }

    response = sendRequest(request)

    if response['followers'] is None:
        return []

    return response['followers']


def debug(message):
    if DEBUGGING:
        print message


app.run("127.0.0.1", 5000, debug=DEBUGGING)
