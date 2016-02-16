import os
import hashlib
from flask import Flask, render_template, request, redirect, flash, url_for, abort, session
from jinja2 import TemplateNotFound
from werkzeug import secure_filename
import flask.ext.login as flask_login


ALLOWED_EXTENSIONS = set(['png', 'jpg', 'jpeg', 'gif'])
app = Flask(__name__)
app.secret_key = os.urandom(24)
app.config['UPLOAD_FOLDER'] = 'db/uploads'

login_manager = flask_login.LoginManager()
login_manager.init_app(app)


class User(flask_login.UserMixin):
    pass


@login_manager.user_loader
def user_loader(id):
    # if email not in users:
    #     return

    data = {}
    with open("db/users/" + str(id)) as user_file:
        for line in user_file:
            (key, val) = line.split(':')
            data[key] = val
    user = User()
    user.id = id
    user.name = data['name']
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
            return redirect(url_for('home'))

    return render_template(
        'register.html',
        form=request.form
    )


@app.route("/login", methods=['post', 'get'])
def login():
    if flask_login.current_user.is_authenticated:
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
    flask_login.logout_user()
    flash("Logged out.", "success")
    return home()


@app.route('/user/<int:user_id>')
def profile(user_id):
    return render_template(
        'user.html',
        user=User()
    )


def allowed_file(filename):
    return '.' in filename and \
           filename.rsplit('.', 1)[1] in ALLOWED_EXTENSIONS


@app.route('/user', methods=['POST'])
def upload_file():
    if request.method == 'POST':
        file = request.files['file']
        file_ext = file.filename.rsplit('.', 1)[1]
        if file and allowed_file(file.filename):
            filename = secure_filename(file.filename)
            file.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))
            return redirect(url_for('home'))
        else:
            abort(400)


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

    flash("Successfully Created User.", "info")
    return True


def login_user(email, password):
    with open("db/user_index", "r") as file:
        for line in file:
            attr = line.strip('\n').split("\t")
            if attr[1] == email and attr[2] == hashlib.sha512(password).hexdigest():
                user = User()
                user.id = attr[0]
                flask_login.login_user(user)

                flash("Logged in.", "success")
                return True

    flash("Bad Login.", "warning")
    return False


app.run("127.0.0.1", 5000, debug=True)
