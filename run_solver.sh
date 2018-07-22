#!/bin/bash

which shuf >/dev/null || exit 1
which parallel >/dev/null || exit 1

TYPE=
MDL_DIR=shared
OUT_DIR=
SOLVER=
SOLVER_OPTS=
SUBMIT=0
RANGE=1-1000
J=1

usage() {
    cat <<EOM
$0
    --mdl_dir               --- default: shared
    --type {FA | FD | FR}
    --solver             --- "//src:{solver}" (e.g. oscar_ai)
    --solver-opts        --- (e.g. --flip=false)
    -o out/
    --range              --- (default: 1-1000; inclusive)
    [-j 1]               --- parallel
    [--submit]           --- submit the result
EOM
    exit 1
}

range() {
    echo $1 | sed 's/\([0-9]*\)-\([0-9]*\)/seq \1 \2/g' | sh
}

while [ $# -gt 0 ]; do
    case "$1" in
        --solver )
            SOLVER=$2
            shift 2
            ;;
        --solver_opts | --solver-opts )
            SOLVER_OPTS=$2
            shift 2
            ;;
        --type )
            TYPE=$2
            shift 2
            ;;
        -o | --output )
            OUT_DIR=$(readlink -f $2)
            shift 2
            ;;
        --mdl_dir | --mdl-dir )
            MDL_DIR=$(readlink -f $2)
            shift 2
            ;;
        -j | --jobs )
            J=$2
            shift 2
            ;;
        --range )
            RANGE=$2
            shift 2
            ;;
        --submit )
            SUBMIT=1
            shift
            ;;
        * )
            usage
            exit 1;
            ;;
    esac
done

if [ -z "$TYPE" -o -z "SOLVER" -o -z "$OUT_DIR" ]; then
    echo "Error: wrong options"
    usage
fi

REPORT_DIR=$OUT_DIR-report

echo ---
echo RANGE $RANGE
echo TYPE $TYPE
echo SOLVER $SOLVER
echo MDL_DIR $MDL_DIR
echo OUT_DIR $OUT_DIR
echo REPORT_DIR $REPORT_DIR
echo J $J
echo SUBMIT $SUBMIT
echo ---

mkdir -p $OUT_DIR
mkdir -p $REPORT_DIR

shuffle() {
    if [ $J -eq 1 ]; then
        cat
    else
        shuf
    fi
}

for i in $(range "$RANGE"); do

    SRC=$(printf "$MDL_DIR/$TYPE%03d_src.mdl" $i)
    TGT=$(printf "$MDL_DIR/$TYPE%03d_tgt.mdl" $i)
    OUT=$(printf "$OUT_DIR/$TYPE%03d.nbt" $i)
    OUT_REPORT=$(printf "$REPORT_DIR/$TYPE%03d.log" $i)

    if [ -f "$SRC" ]; then
        SRC=$(readlink -f $SRC)
        if [ -f "$TGT" ]; then
            TGT=$(readlink -f $TGT)
        else
            TGT=-
        fi
    else
        if [ -f "$TGT" ]; then
            SRC=-
            TGT=$(readlink -f $TGT)
        else
            continue
        fi
    fi

    echo "bazel run //solver:$SOLVER -- --src_filename=$SRC --tgt_filename=$TGT $SOLVER_OPTS > $OUT 2>$OUT_REPORT && rm $OUT_REPORT"
done |
shuffle |
parallel -v -j ${J}


if [ $SUBMIT -eq 0 ]; then
    exit
fi

# submitting
for i in $(range "$RANGE"); do
    PROBLEM_ID=$(printf "$TYPE%03d" $i)
    NBT_FILE=$(printf "$OUT_DIR/$TYPE%03d.nbt" $i)
    LOG_FILE=$(printf "$REPORT_DIR/$TYPE%03d.log" $i)
    if [ -f "$NBT_FILE" -a ! -f "$LOGFILE" ]; then
        echo "Submitting $NBT_FILE"

    curl http://negainoido:icfpc_ojima@35.196.88.166/submit_solution \
         -F problem_id=${PROBLEM_ID} \
         -F solver_id=${SOLVER} \
         -F comment=from-run_solver \
         -F nbt=@${NBT_FILE} > /dev/null
    fi

done
