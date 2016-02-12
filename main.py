from flask import Flask, render_template, request, redirect

app = Flask(__name__)


@app.route("/")
@app.route("/home")
def home():
    return render_template(
        'index.html',
        title='Home'
    )


@app.route("/register")
def register():
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


app.run("127.0.0.1", 5000, debug=True)
