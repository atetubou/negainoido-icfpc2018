import mysql.connector
import os

import mysql.connector.pooling
import shutil
import subprocess
import json
import traceback
import time

dbconfig = {
    "database" : os.environ['DBNAME'],
    "user" : os.environ['DBUSER'],
    "password" : os.environ['DBPASS'],
    "host" : 'localhost',
}
destpath="static/solutions"

def eval_solution(cnx, solution_id):
    print("solution", solution_id)
    curr = cnx.cursor(dictionary=True)
    curr.execute("select * from solutions where id = %s", (solution_id,))
    solution = curr.fetchone()
    assert(solution)
    name = solution["problem_id"]
    curr.execute("select filepath from problems where name = %s", (name,))
    prob = curr.fetchone()
    assert(prob)
    prob_path = prob['filepath']
    
    dest = os.path.join(destpath, '%d.nbt' % solution_id)
    dest_json = os.path.join(destpath, '%d.nbt.json' % solution_id)

    s = subprocess.check_output('../bazel-bin/src/simulator --mdl_filename %s --nbt_filename %s' % (prob_path,dest), shell=True, universal_newlines =True)
    print(s)
    score = int(s)
    curr.execute("update solutions set score = %s where id = %s", (score,solution_id))
    cnx.commit()

def main():
    while True:
        cnx = mysql.connector.connect(**dbconfig)
        curr = cnx.cursor(dictionary=True)
        curr.execute("select id from solutions where score is NULL order by created_at asc limit 1")
        row = curr.fetchone()
        if not row:
            time.sleep(1)
            continue
        try: 
            eval_solution(cnx, row['id'])
        except:
            traceback.print_exc()
            curr.execute("update solutions set score = %s where id = %s", (-1,row['id']))
            cnx.commit()
            time.sleep(1)

if __name__ == '__main__':
    main()
