#!/bin/bash

curl -s https://icfpcontest2018.github.io/full/live-standings.html > /tmp/data

DATETIME=$(
    cat /tmp/data |
    grep -o '<h1 id="live-standings-full-at-2018-07-[0-9]*-[0-9]*-utc">' |
    grep -o '".*"'
)

# problem IDs
cat /tmp/data |
grep '<h3 id="problem-' | sed 's/^.* //g; s,</h3>,,g' >/tmp/data.prob

# score, energy
cat /tmp/data |
grep -A 17 '<h3 id="problem-' |
grep '<td style="text-align:right"><pre>[0-9][0-9]' |
grep -o '[0-9]*' | awk 'NR%2==0{print A,$0} {A=$0}' >/tmp/data.score

# teams
cat /tmp/data |
grep -A 17 '<h3 id="problem-' |
grep 'text-align:left' | sed 's/^ *//g; s/<[^>]*>//g' >/tmp/data.team

# output
(
echo "{"
paste /tmp/data.prob /tmp/data.score /tmp/data.team |
sed 's/\(.*\)\t\(.*\) \(.*\)\t\(.*\)/"\1": {"energy": \2, "score": \3, "team": "\4"},/g'
echo "\"datetime\": $DATETIME }"
) | jq . > /tmp/out.json
jq . /tmp/out.json

# backup
cp /tmp/out.json /tmp/out.json.bk

# submit
gsutil cp /tmp/out.json gs://negainoido-icfpc2018-shared-bucket/live.json
