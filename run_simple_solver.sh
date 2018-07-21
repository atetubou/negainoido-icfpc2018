#!/bin/bash


for mdl in `ls $1/*.mdl`; do
    if [ -f $mdl ]; then
        output_file=`basename $mdl | sed -e 's/_tgt\.mdl/\.nbt/'`
        echo "running for ${mdl}"

        bazel run //solver:simple_solve -- --mdl_filename=`pwd`/$mdl > $2/${output_file} || exit 1
    fi
done


