#!/bin/bash

MDL_PATH=$(readlink -f $1)
OUT_DIR=$2
SOLVER=${3:-simple_solve}
echo $MDL_PATH $OUT_DIR $SOLVER

mkdir -p $OUT_DIR

for mdl in $MDL_PATH/FA*.mdl; do
    if [ -f $mdl ]; then
        output_file=`basename $mdl | sed -e 's/_tgt\.mdl/\.nbt/'`
        echo "running for ${mdl}"
        bazel run "//solver:$SOLVER" -- --tgt_filename=$MLD_PATH/$mdl > $OUT_DIR/${output_file} || exit 1
    fi
done

