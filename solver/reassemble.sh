#!/bin/bash

SRC_FILENAME=$1
TGT_FILENAME=$2
ASSEMBLE_SOLVER=${3:-simple_solve}
DEASSEMBLE_SOLVER=${4:-square_deleter}

src_temp_file=$(mktemp)
tgt_temp_file=$(mktemp)
(bazel run //solver:${DEASSEMBLE_SOLVER} -- --src_filename=${SRC_FILENAME}) > ${src_temp_file} || exit 1
(bazel run //solver:${ASSEMBLE_SOLVER} -- --tgt_filename=${TGT_FILENAME}) > ${tgt_temp_file} || exit 1
bazel run //src:trace_connector -- --assemble_nbt ${tgt_temp_file} --deassemble_nbt ${src_temp_file}
