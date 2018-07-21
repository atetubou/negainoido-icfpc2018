
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
        if not row['filepath'] is None and base[-3:] == 'tgt':
            dest_path = os.path.join(destpath, fname)
            shutil.copy(os.path.join(path, fname), dest_path)
            curr.execute("update problems set filepath = %s where name = %s", (dest_path, name))

        curr.close()
        cnx.commit()

cnx.commit()
        

