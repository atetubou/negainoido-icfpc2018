#!/bin/bash

MDL_PATH=$(readlink -f $1)
OUT_DIR=$2
SOLVER=${3:-simple_solve}
echo $MDL_PATH $OUT_DIR $SOLVER

mkdir -p $OUT_DIR

for mdl in $MDL_PATH/FR*_src.mdl; do
    if [ -f $mdl ]; then
        output_file=`basename $mdl | sed -e 's/_src\.mdl/\.nbt/'`
        tgt_file=`basename $mdl | sed -e 's/_src/_tgt/'`
        echo "running for ${mdl}"
        bazel run "//solver:$SOLVER" -- --src_filename=$MLD_PATH/$mdl --tgt_filename=$MLD_PATH/$mdl> $OUT_DIR/${output_file} || exit 1
    fi
done

