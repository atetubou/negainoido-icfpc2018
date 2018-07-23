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

curr = cnx.cursor(dictionary=True)
curr.execute("select * from solutions")

result = []
for row in curr:
    d = { "problem_id": row["problem_id"],
          "score": row["score"],
          "solver_id": row["solver_id"] }
    result.append(d)
json.dump(result, open("solutions-dump.json","w"))


