import mysql.connector
import os

import mysql.connector.pooling
import shutil
import subprocess
import json
import datetime
dbconfig = {
    "database" : os.environ['DBNAME'],
    "user" : os.environ['DBUSER'],
    "password" : os.environ['DBPASS'],
    "host" : 'localhost',
}
cnx = mysql.connector.connect(**dbconfig)

curr = cnx.cursor(dictionary=True)
curr.execute(
    "select id, best.problem_id, solver_id, best.score, best.max_score, created_at"
    " from solutions inner join"
    " (select problem_id, MIN(score) as score, MAX(score) as max_score from solutions group by problem_id) as best"
    " on solutions.problem_id = best.problem_id and solutions.score = best.score"
    " order by best.problem_id asc, solutions.score asc"
)
destpath="/tmp/solution/"
subprocess.call('rm -rf /tmp/solution', shell=True)
subprocess.call('mkdir /tmp/solution', shell=True)

for row in curr:
    problem_id = row['problem_id']
    solution_id = row['id']
    from_path = "static/solutions/%d.nbt" % solution_id
    to_path = os.path.join(destpath, "%s.nbt" % problem_id)
    shutil.copy(from_path, to_path)

subprocess.call('cd /tmp/solution; zip result.zip *', shell=True)
dest = 'static/zip/' + datetime.datetime.now().strftime('%Y-%m-%d-%H%M%S') +'.zip'
shutil.copy('/tmp/solution/result.zip', dest)

curr.execute('insert into test(message) values (%s)', (dest,))
cnx.commit()


