#!/bin/bash


TYPE=
MDL_DIR=shared
OUT_DIR=
RANGE=1-1000
OFFICIAL=0

usage() {
    cat <<EOM
$0
    --mdl_dir               --- default: shared
    --type {FA | FD | FR}
    -o out/
    --range              --- (default: 1-1000; inclusive)
    --official           --- use official sim
EOM
    exit 1
}

range() {
    echo $1 | sed 's/\([0-9]*\)-\([0-9]*\)/seq \1 \2/g' | sh
}

while [ $# -gt 0 ]; do
    case "$1" in
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
        --official )
            OFFICIAL=1
            shift
            ;;
        * )
            usage
            exit 1;
            ;;
    esac
done

if [ -z "$TYPE" -o -z "$OUT_DIR" ]; then
    echo "Error: wrong options"
    usage
fi

REPORT_DIR=$OUT_DIR-report

echo ---
echo RANGE $RANGE
echo TYPE $TYPE
echo MDL_DIR $MDL_DIR
echo OUT_DIR $OUT_DIR
echo REPORT_DIR $REPORT_DIR
echo ---

mkdir -p $OUT_DIR
mkdir -p $REPORT_DIR

rm -f $RES_REPORT
for i in $(range "$RANGE"); do

    PROBLEM_ID=$(printf "$TYPE%03d" $i)
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

    if [ $OFFICIAL -eq 0 ]; then
        bazel run //src:simulator -- -src_filename=$SRC -tgt_filename=$TGT $SOLVER_OPTS -nbt_filename $OUT 2>$OUT_REPORT && rm $OUT_REPORT || cat $OUT_REPORT
    else
        python soren/main.py  $SRC $TGT $SOLVER_OPTS $OUT 2>$OUT_REPORT && rm $OUT_REPORT || cat $OUT_REPORT
    fi
done


