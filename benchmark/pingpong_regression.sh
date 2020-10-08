#!/bin/bash

output_folder="evaluation/regression"
mkdir -p ${output_folder}

function init_file() {
  printf "${1}, " > ${2}.out
  for i in $(seq 1 $3); do
    printf "value${i}, " >> ${2}.out
  done;
  echo "" >> ${2}.out
}

out_file="evaluation/regression/pingpong-tcp-net-regression"
init_file runs ${out_file} ${runs}
for runs in 10 20 30 40 50 60 70 80 90 100; do
  echo "pingpong-tcp-net-regression-${runs}-runs"
  printf "${runs}, " >> ${out_file}.out
  for i in $(seq 1 $runs); do
    while : ; do
      ./release/pingpong_tcp -mnetBench -p10000 -s1024 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
done;
