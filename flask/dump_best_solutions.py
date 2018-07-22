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
curr.execute("select * from solutions where score > 0")
destpath="/tmp/solution/"
subprocess.call('rm -rf /tmp/solution', shell=True)
subprocess.call('mkdir /tmp/solution', shell=True)

best = {}

for row in curr:
    problem_id = row['problem_id']
    if problem_id[:2]=='LA':
        continue
    score = row['score']
    if problem_id not in best or best[problem_id]['score'] > score:
        best[problem_id] = row

for problem_id,row in best.items():
    solution_id = row['id']
    from_path = "static/solutions/%d.nbt" % solution_id
    to_path = os.path.join(destpath, "%s.nbt" % problem_id)
    shutil.copy(from_path, to_path)

subprocess.call('cd /tmp/solution; zip result.zip *', shell=True)
sha1 = subprocess.check_output('shasum -a 256 /tmp/solution/result.zip',shell=True, universal_newlines=True).split()[0]
dest = 'static/zip/' + datetime.datetime.now().strftime('%Y-%m-%d-%H%M%S-') + sha1 +'.zip'

shutil.copy('/tmp/solution/result.zip', dest)

curr.execute('insert into test(message) values (%s)', (dest,))
cnx.commit()


