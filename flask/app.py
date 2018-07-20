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
