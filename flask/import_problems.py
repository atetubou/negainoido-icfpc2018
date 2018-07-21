
import mysql.connector
import os
import shutil

import mysql.connector.pooling
dbconfig = {
    "database" : os.environ['DBNAME'],
    "user" : os.environ['DBUSER'],
    "password" : os.environ['DBPASS'],
    "host" : 'localhost',
}
cnx = mysql.connector.connect(**dbconfig)
path="shared/problemsL/"
destpath="static/problems"

for fname in os.listdir(path):
    base, ext = os.path.splitext(os.path.basename(fname))
    if ext != ".mdl":
        continue
    name = base[:-4]
    curr = cnx.cursor()
    curr.execute("select name from problems where name = %s", (name,))
    if curr.fetchone():
        curr.close()
        continue
    shutil.copy(os.path.join(path, fname), os.path.join(destpath, fname))


    curr.execute("insert into problems(name, filepath) values (%s,%s)", (name, os.path.join(destpath,fname))) 
    curr.close()

cnx.commit()
        

