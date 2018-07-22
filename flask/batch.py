import mysql.connector
import os

import mysql.connector.pooling
import shutil
import subprocess
import json
import traceback
import time
from multiprocessing import Process, Queue, Value
import sys
from queue import Full

dbconfig = {
    "database" : os.environ['DBNAME'],
    "user" : os.environ['DBUSER'],
    "password" : os.environ['DBPASS'],
    "host" : 'localhost',
}
destpath="static/solutions"

task_queue = Queue(30)
NUM_WORKERS = 4


def worker(worker_id):
    while True:
        solution_id = task_queue.get()
        print("worker %d is evaluating %d" % (worker_id, solution_id))
        sys.stdout.flush()
        cnx = mysql.connector.connect(**dbconfig)
        try:
            eval_solution(cnx, solution_id)
        except:
            _,_, tb = sys.exc_info()
            print("exception on worker ", worker_id, tb)
            sys.stdout.flush()
            comment = str(tb)
            curr=cnx.cursor()
            curr.execute("update solutions set score = -1, comment = %s where id = %s ", (comment, solution_id))
            curr.close()
            cnx.commit()

def eval_solution(cnx, solution_id):
    print("solution", solution_id)
    sys.stdout.flush()
    curr = cnx.cursor(dictionary=True)
    curr.execute("select * from solutions where id = %s", (solution_id,))
    solution = curr.fetchone()
    assert(solution)
    name = solution["problem_id"]
    curr.execute("select filepath, src_filepath from problems where name = %s", (name,))
    prob = curr.fetchone()
    assert(prob)
    prob_path = prob['filepath']
    prob_srcpath = prob['src_filepath']
    
    dest = os.path.join(destpath, '%d.nbt' % solution_id)
    dest_json = os.path.join(destpath, '%d.nbt.json' % solution_id)

    dest = os.path.abspath(dest)
    if prob_path:
        prob_path = os.path.abspath(prob_path)
    else:
        prob_path = '-'

    if prob_srcpath:
        prob_srcpath = os.path.abspath(prob_srcpath)
    else:
        prob_srcpath = '-'

    try:
        s = subprocess.check_output('python ../soren/main.py %s %s %s' % (prob_srcpath,prob_path,dest), 
                                    shell=True, 
                                    universal_newlines =True)
        comment = "output: " + s
        curr.execute("update solutions set comment = %s where id = %s", (comment,solution_id))
        score = int(s)
        curr.execute("update solutions set score = %s where id = %s", (score,solution_id))
        cnx.commit()
    
    except subprocess.CalledProcessError as exc:
        print("Status : FAIL", exc.returncode, exc.output)
        sys.stdout.flush()
        comment = "Failed:" + exc.output
        curr.execute("update solutions set comment = %s, score = -1 where id = %s", (comment, solution_id))
        cnx.commit()

def main():
    # launch workers
    workers = [ Process(target=worker,args=(i,)) for i in range(NUM_WORKERS) ] 
    last_id = None
    
    for th in workers:
        th.start()
    print("%d workers started" % NUM_WORKERS)
    sys.stdout.flush()
    
    try:
        while True:
            cnx = mysql.connector.connect(**dbconfig)
            curr = cnx.cursor(dictionary=True)
            if last_id:
                curr.execute("select id from solutions where score is NULL and id > %s order by id asc limit 1", (last_id,))
            else:
                curr.execute("select id from solutions where score is NULL order by id asc limit 1")
            row = curr.fetchone()
            curr.close()
            cnx.close()
            if not row:
                time.sleep(1)
                continue
            try: 
                task_queue.put(row['id'])
                last_id = row['id']
            except Full:
                print("queue is full")
                sys.stdout.flush()
                time.sleep(1)
    finally:
        for th in workers:
            th.terminate()

if __name__ == '__main__':
    main()
