#!/bin/bash

output_folder="evaluation/out"
mkdir -p ${output_folder}

function init_file() {
  printf "${1}, " > ${2}.out
  for i in 1 2 3 4 5 6 7 8 9 10; do
    printf "value${i}, " >> ${2}.out
  done;
  echo "" >> ${2}.out
}

echo "-- netBenchmark ---------------------------------------------------------"

out_file="evaluation/out/pingpong-tcp-net-num-pings"
echo "-- pingpong-tcp-net-num-pings --------------------------------------"
init_file num_pings ${out_file}
num_pings=1024
while [ $num_pings -le 600000 ]; do
  echo "-- ${num_pings} pings -----------------------------------------------"
  printf "${num_pings}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/pingpong_tcp -mnetBench -p$num_pings >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  num_pings=$((num_pings*2))
done;



echo "-- ioBenchmark ----------------------------------------------------------"

out_file="evaluation/out/pingpong-tcp-io-num-pings"s
echo "-- pingpong-tcp-io-num-pings --------------------------------------"
init_file num_pings ${out_file}
num_pings=1024
while [ $num_pings -le 600000 ]; do
  echo "-- ${num_pings} pings -----------------------------------------------"
  printf "${num_pings}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/pingpong_tcp -mioBench -p$num_pings >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  num_pings=$((num_pings*2))
done;
