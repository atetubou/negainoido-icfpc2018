from flask import Flask, g, render_template, request, jsonify
import mysql.connector.pooling
from urllib.parse import urlparse
import os.path
import os
from result import Ok, Ng, Result
from typing import TypeVar,Generic,List,Any,Callable,cast

app = Flask(__name__)
dbconfig = {
    "database" : os.environ['DBNAME'],
    "user" : os.environ['DBUSER'],
    "password" : os.environ['DBPASS'],
    "host" : 'localhost',
    "pool_name" : "mysql_pool",
    "pool_size" : 20,
    "autocommit" : True
}

# apiデコレータ
# rule: apiのポイント(html: <rule>, json: <rule>/jsonが自動生成される)
# template: 表示に用いるテンプレートファイルの名前
def api(rule, template, **options):
    def _api(func):
        def wrapper_html(*args, **kwargs):
            res = func(*args, **kwargs)
            return res.render_template(template)
        
        def wrapper_json(*args, **kwargs):
            res = func(*args, **kwargs)
            return res.jsonify()
        wrapper_html.__name__ = func.__name__ + '_as_html'
        app.add_url_rule(rule, view_func=wrapper_html, **options)
        wrapper_json.__name__ = func.__name__ + '_as_json'
        app.add_url_rule(os.path.join(rule,'json'), 
                         view_func=wrapper_json, 
                         **options)
        return func
    return _api


def get_connection():
    return mysql.connector.connect(**dbconfig)

@api('/test_error', 'index.html')
def test_error():
    return Ng('test message')

@api('/test_insert', 'test_insert.html', methods=['GET','POST'])
def test_insert() -> Result[None]:
    if request.method != 'POST':
        return Ok(None)
    if 'message' in request.form:
        message = request.form['message'].strip()
    else:
        return Ng('no message')
    if message == '':
        return Ng('empty message')
    conn = get_connection()
    curr = conn.cursor()
    try:
        curr.execute("insert into test(message) values (%s)", (message,))
        conn.commit()
        return Ok(None)
    finally:
        curr.close()
        conn.close()

@api('/submit_solution', 'submit_solution.html', methods=['GET','POST'])
def submit_solution() -> Result[None]:
    if request.method != 'POST':
        return Ok(None)

    problem_id = request.form.get('problem_id', None)
    solver_id = request.form.get('solver_id', None)
    comment = request.form.get('comment', None)
    nbt = request.files.get('nbt', None)

    if not problem_id:
        return Ng('problem_id is missing')
    if not solver_id:
        return Ng('solver_id is missing')
    if not nbt:
        return Ng('nbt file is missing')

    conn = get_connection()
    curr = conn.cursor()
    try:
        curr.execute('insert into solutions(problem_id,solver_id,comment) values(%s,%s,%s)',(problem_id, solver_id, comment))
        id = curr.lastrowid
        nbt.save('static/solutions/%d.nbt' % id)
        conn.commit()
    finally:
        curr.close()
        conn.close()
    return Ok(None)
       

@api('/test_select', 'test_select.html')
def test_select() -> Result[List[Any]]:
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    try:
        curr.execute("select id,message,created_at from test order by id desc limit 100")
        return Ok(list(curr))
    finally:
        curr.close()
        conn.close()

@api('/list_solution', 'list_solution.html')
def list_solution() -> Result[List[Any]]:
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    try:
        curr.execute("select id,problem_id,solver_id, score, comment, created_at from solutions order by id desc")
        return Ok(list(curr))
    finally:
        curr.close()
        conn.close()

@api('/list_problems', 'list_problems.html')
def list_problems() -> Result[List[Any]]:
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    try:
        curr.execute("select name, solutions.id, solutions.solver_id, solutions.score, solutions.created_at from problems"
                     " inner join solutions on solutions.problem_id = problems.name"
                     " order by name asc, solutions.score asc"
        )
        return Ok(list(curr))
    finally:
        curr.close()
        conn.close()


@api('/list_solution/<int:id>', 'solution_id.html')
def solution_id(id : int) -> Result[Any]:
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    try:
        res = curr.execute("select id,problem_id,solver_id, score, comment, created_at from solutions where id = %s", (id,))
        row = curr.fetchone()
        if row:
            return Ok(row)
        else:
            return Ng('no data for key ' + str(id))
    finally:
        curr.close()
        conn.close()
@api('/test_select/<int:id>', 'test_select_id.html')
def test_select_id(id : int) -> Result[Any]:
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    try:
        res = curr.execute("select id,message,created_at from test where id = %s", (id,))
        row = curr.fetchone()
        if row:
            return Ok(row)
        else:
            return Ng('no data for key ' + str(id))
    finally:
        curr.close()
        conn.close()


@app.route('/')
def index():
    return render_template('index.html')

if __name__ == "__main__":
    app.debug = True
    app.run()
