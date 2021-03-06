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
import  signal
from table import problem_size

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
            etype, value, tb = sys.exc_info()
            emsg = ''.join(format_exception(etype, value, tb, limit))
            print("exception on worker ", worker_id, emsg)
            sys.stdout.flush()
            comment = emsg
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
        if 'unsafe' in solution['solver_id']:
            cmd = '../bazel-bin/src/simulator_unsafe --src_filename %s --tgt_filename %s --nbt_filename %s' % (prob_srcpath,prob_path,dest)
        elif solution['solver_id'] == 'DEFALT' or problem_size(solution['problem_id']) >= 120:
            cmd = '../bazel-bin/src/simulator --src_filename %s --tgt_filename %s --nbt_filename %s' % (prob_srcpath,prob_path,dest)
        else:
            cmd = 'python ../soren/main.py %s %s %s' % (prob_srcpath,prob_path,dest)
        s = subprocess.check_output(cmd,
                                    shell=True, 
                                    universal_newlines =True)
        comment = "cmd: " + cmd + "\n"
        comment += "output: " + s
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

    def killchildren(a,b):
        exit(0)

    signal.signal(signal.SIGTERM, killchildren)
    
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
            print("kill!")
            sys.stdout.flush()
            th.terminate()

if __name__ == '__main__':
    main()
