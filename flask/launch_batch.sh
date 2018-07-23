#!/bin/bash

set -x
if test -f batch.pid && kill -0 $(cat batch.pid) 2> /dev/null; then
    echo kill batch process
    kill $(cat batch.pid)
fi

source env/bin/activate
source secret.sh
#python batch.py
nohup python batch.py > batch.log 2>&1 &
echo $! > batch.pid

