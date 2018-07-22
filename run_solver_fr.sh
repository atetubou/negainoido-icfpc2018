#!/bin/bash

MDL_PATH=$(readlink -f $1)
OUT_DIR=$2
SOLVER=${3:-simple_solve}
echo $MDL_PATH $OUT_DIR $SOLVER

mkdir -p $OUT_DIR

P=${P:-1}
for mdl in $MDL_PATH/FR*_src.mdl; do
    if [ -f $mdl ]; then
        output_file=`basename $mdl | sed -e 's/_src\.mdl/\.nbt/'`
        tgt_file=`basename $mdl | sed -e 's/_src/_tgt/'`
        # echo "running for ${mdl}"
        echo "bazel run //solver:$SOLVER -- --src_filename=$MLD_PATH/$mdl --tgt_filename=$MLD_PATH/$mdl> $OUT_DIR/${output_file}"
    fi
done | shuf | parallel -j ${P} --dry-run

for nbt in $OUT_DIR/FR*.nbt; do
    problem_id=$(basename ${nbt} | sed s/.nbt//g)
    curl http://negainoido:icfpc_ojima@35.196.88.166/submit_solution \
         -F problem_id=${problem_id} \
         -F solver_id=${SOLVER} \
         -F comment=from-run_solver_fr \
         -F nbt=@${nbt} > /dev/null
done
