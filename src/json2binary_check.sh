set -eux

for i in ~/LA*.nbt
do
    echo $i
    bazel run //src:nbt_viewer -- --json --nbt_filename=$i > $i.json
    bazel run //src:json2binary -- --json_filename=$i.json > $i.json.nbt
    diff $i $i.json.nbt
done
