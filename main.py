import os
from flask import Flask, render_template, request, redirect, flash

app = Flask(__name__)
app.secret_key = os.urandom(24)


@app.route("/")
@app.route("/home")
def home():
    return render_template(
        'index.html',
        title='Home'
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
        title="Register"
    )

# @app.route('/', defaults={'path': ''})
# @app.route('/<path:path>')
# def catch_all(path):
#     return 'You want path: %s' % path


@app.errorhandler(404)
def page_not_found(e):
    return render_template(
        '404.html',
        title="404 Error"), 404


def create_user(email, first_name, last_name, pwd1, pwd2):
    if not email:
        flash("Invalid Email.", "warning")
        return False
    if pwd1 != pwd2:
        flash("Passwords do not match.", "warning")
        return False
    else:
        flash("Successfully Created User.", "info")
        return True

app.run("127.0.0.1", 5000, debug=True)
