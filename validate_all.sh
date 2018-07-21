set -eux

for i in $(seq -w 1 186)
do
    echo $i
    time go run validate.go -problem LA$i
done
