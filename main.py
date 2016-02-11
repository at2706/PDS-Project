from flask import Flask, render_template, request, redirect

app = Flask(__name__)


@app.route("/")
@app.route("/home")
def home():
    return render_template(
        'index.html',
        title='Contact',
        # year=datetime.now().year,
        message='Your contact page.'
    )


app.run("127.0.0.1", 5000, debug=True)
