import mysql.connector
import os

import mysql.connector.pooling
import shutil
import subprocess
import json
dbconfig = {
    "database" : os.environ['DBNAME'],
    "user" : os.environ['DBUSER'],
    "password" : os.environ['DBPASS'],
    "host" : 'localhost',
}
cnx = mysql.connector.connect(**dbconfig)
path="shared/dfltTracesL/"
destpath="static/solutions"



for fname in os.listdir(path):
    base, ext = os.path.splitext(os.path.basename(fname))
    if ext != ".nbt":
        continue
    curr = cnx.cursor()
    solver = "DEFALT"
    curr.execute("select problem_id from solutions where problem_id = %s and solver_id = %s", (base,solver))
    if curr.fetchone():
        curr.close()
        continue
    curr.execute("insert into solutions(problem_id, solver_id) values (%s,%s)", (base, solver)) 
    id = curr.lastrowid
    curr.close()
    cnx.commit()

scores=json.load(open('solutions-dump.json'))
for solution in scores:
    pid = solution['problem_id']
    score = solution['score']
    sid = solution['solver_id']
    
    curr = cnx.cursor()
    curr.execute("update solutions set score = %s where problem_id = %s AND solver_id = %s", (score,pid, sid))
    cnx.commit()


