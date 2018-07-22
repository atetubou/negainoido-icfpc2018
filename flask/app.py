from flask import Flask, g, render_template, request, jsonify
import mysql.connector.pooling
from urllib.parse import urlparse
import os.path
import os
from result import Ok, Ng, Result
from typing import TypeVar,Generic,List,Any,Callable,cast
import math
from table import problem_size
import shutil
import subprocess

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


def score(r, default, team):
    return math.floor(math.floor(math.log(r, 2)) * 1000 * (default - team) / default)

def get_connection():
    return mysql.connector.connect(**dbconfig)

@api('/test_error', 'index.html')
def test_error():
    return Ng('test message')

@api('/gen_zip', 'gen_zip.html', methods=['GET','POST'])
def gen_zip() -> Result[Any]:
    if request.method != 'POST':
        return Ok(list(os.listdir('static/zip')))

    subprocess.call('python dump_best_solutions.py', shell =True)
    return Ok(list(os.listdir('static/zip')))


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

@api('/resubmit_solution/<int:id>',  'solution_id.html', methods=['POST'])
def resubmit_solution(id):
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    try:
        curr.execute('select * from solutions where id = %s', (id,))
        row = curr.fetchone()
        if not row:
            return Ng('invalid id')
        problem_id = row['problem_id']
        solver_id = row['solver_id']
        nbt_src_path = 'static/solutions/%d.nbt' % id
        curr.execute('insert into solutions(problem_id, solver_id) values (%s,%s)', (problem_id, solver_id))

        new_id = curr.lastrowid
        nbt_dst_path = 'static/solutions/%d.nbt' % new_id
        shutil.copy( nbt_src_path, nbt_dst_path )
        curr.execute('delete from solutions where id = %s', (id,))
        conn.commit()
        res = curr.execute("select id,problem_id,solver_id, score, comment, created_at from solutions where id = %s", (new_id,))
        row = curr.fetchone()
        assert(row)

        return Ok(row)
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

@api('/list_solution', 'list_solution.html')
def list_solution() -> Result[Any]:
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    pid = request.args.get("problem", None)
    try:
        if pid:
            curr.execute("select id,problem_id,solver_id, score, comment, created_at from solutions where problem_id = %s order by id desc", (pid,))
            return Ok({ "solutions" : list(curr), "problem_id" : pid })
        else:
            curr.execute("select id,problem_id,solver_id, score, comment, created_at from solutions order by id desc")
            return Ok({ "solutions" : list(curr) })
    finally:
        curr.close()
        conn.close()


@api('/list_problems', 'list_problems.html')
def list_problems() -> Result[List[Any]]:
    conn = get_connection()
    curr = conn.cursor(dictionary=True)
    try:
        curr.execute('select * from standing_scores')
        standing_tbl = { row['name']: row   for row in curr }
        
        curr.execute("""
select name, ifnull(t.created_at,problems.created_at) updated_at, t.solver_id, t.score, t.max_score, t.id 
  from problems 
  left join   
    (select solutions.problem_id, solutions.solver_id, solutions.id, best.score, best.max_score, created_at 
      from 
        (select best.problem_id, score, max_score from
          (select problem_id, MIN(score) as score 
            from solutions where score > 0 group by problem_id) as best
          left join
            (select problem_id, MAX(score) as max_score
              from solutions where score > 0 and solver_id = 'DEFALT' group by problem_id) as def
          on best.problem_id = def.problem_id) as best
      left join solutions 
      on solutions.problem_id = best.problem_id and solutions.score = best.score) as t   
  on t.problem_id = problems.name 
  where name not like "LA%"
  order by name asc
""")
        total_score = 0
        def estimated_score(row):
            if row['score'] and row['max_score']:
                return score(row['r'], row['max_score'], row['score'])
            else:
                return 0

        def opt_score(row):
            if row['name'] in standing_tbl:
                opt = standing_tbl[row['name']]['score']
            elif row['score']:
                opt = row['score'] // 100
            else:
                opt = 0
            if row['max_score']:
                return score(row['r'], row['max_score'], opt)
            else:
                return 0

        ret = list(curr)
        for row in ret:
            row['r'] = problem_size(row['name'])
            row['estimated_score'] = estimated_score(row)
            row['suboptimal_score'] = opt_score(row)
            if row['name'] in standing_tbl:
                row['opt'] = standing_tbl[row['name']]['score']
            row['live'] = row['name'] in standing_tbl
        
        ret.sort(key = (lambda row: row['estimated_score'] - row['suboptimal_score']))
        return Ok({ "problems" : ret, "total_score" : total_score })
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
