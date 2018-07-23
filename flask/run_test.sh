source env/bin/activate
source secret.sh

FLASK_APP=app.py FLASK_ENV=development python -m flask run --port 8080 > debug.log 2>&1 &
SERVER_PID=$!

sleep 1
if kill -0 $SERVER_PID 2>/dev/null; then
    echo server launched!
else
    echo server failed to launch!
    exit 1
fi

ERROR=0
# run tests
for test in tests/*.py; do
    if ! python -m unittest -v $test; then
        ERROR=1
        break
    fi
done

if [ $ERROR != 0 ]; then
    echo enter debug shell
    PS1="(debug) $ " bash --norc
fi

echo exiting debug server
kill $SERVER_PID
wait $SERVER_PID
exit $ERROR

