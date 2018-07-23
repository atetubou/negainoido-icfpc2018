
import mysql.connector
import os
import shutil
import json

import mysql.connector.pooling
dbconfig = {
    "database" : os.environ['DBNAME'],
    "user" : os.environ['DBUSER'],
    "password" : os.environ['DBPASS'],
    "host" : 'localhost',
}
cnx = mysql.connector.connect(**dbconfig)
paths=["shared/problemsL/", "shared/problemsF/"]
destpath="static/problems"


for path in paths:
    for fname in os.listdir(path):
        base, ext = os.path.splitext(os.path.basename(fname))
        if ext != ".mdl":
            continue
        name = base[:-4]
        curr = cnx.cursor(dictionary=True)
        curr.execute("select name, src_filepath, filepath from problems where name = %s", (name,))
        row = curr.fetchone()
        if not row:
            curr.execute("insert into problems(name) values (%s)", (name,))
        
        curr.execute("select name, src_filepath, filepath from problems where name = %s", (name,))
        row = curr.fetchone()
        print(row,base,base[-3:])
        if row['src_filepath'] is None and base[-3:] == 'src':
            src_path = os.path.join(destpath, fname)
            shutil.copy(os.path.join(path, fname), src_path)
            curr.execute("update problems set src_filepath = %s where name = %s", (src_path, name))
        if row['filepath'] is None and base[-3:] == 'tgt':
            dest_path = os.path.join(destpath, fname)
            shutil.copy(os.path.join(path, fname), dest_path)
            curr.execute("update problems set filepath = %s where name = %s", (dest_path, name))

        curr.close()
        cnx.commit()

curr= cnx.cursor(dictionary=True)
curr.execute('select * from problems where name like "FR%"')
problems = list(curr)
curr.close()
for problem in problems:
    pd_name = 'PD' + problem['name'][2:]
    curr= cnx.cursor(dictionary=True)
    curr.execute('select * from problems where name = %s', (pd_name,))
    if not curr.fetchone():
        curr.execute('insert into problems(name) values (%s)', (pd_name,))
        continue
    src_path = 'static/problems/'+pd_name+'_src.mdl'
    shutil.copy( problem['src_filepath'], src_path)

    curr.execute('update problems set src_filepath = %s where name = %s' , (src_path, pd_name))
    curr.close()
    cnx.commit()


best_scores = json.load(open('live.json'))
for key in best_scores.keys():
    if key[0] != 'F':
        continue
    v = best_scores[key]
    energy = v['energy']
    team = v['team']
    curr = cnx.cursor()
    curr.execute('insert into standing_scores(name,team,score) values (%s,%s,%s)'
                 ' on duplicate key update team = %s, score = %s', (key, team, energy, team, energy))
    cnx.commit()

cnx.commit()
        

