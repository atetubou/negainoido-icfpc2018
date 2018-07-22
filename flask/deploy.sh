#!/bin/bash
if [ ! -e env ]; then
    virtualenv env
fi

(cd ../; bazel build //src:simulator)

gsutil copy gs://negainoido-icfpc2018-shared-bucket/live.json live.json

source env/bin/activate
set -eu
source secret.sh
pip install -r requirements.txt
mysql -u $DBUSER -p"$DBPASS" $DBNAME < schema.sql

python import_problems.py
python import_default_solution.py
./launch_batch.sh

./run_test.sh 
if [ $FLASK_ENV = production ]; then
    if [ ! -f uwsgi.pid ] || ! kill -0 $(cat uwsgi.pid) 2>/dev/null; then
        nohup uwsgi --ini myapp.ini &
    fi
    touch reload.trigger
fi
