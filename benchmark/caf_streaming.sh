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

out_file="evaluation/out/caf-streaming-net-remote-nodes"
echo "-- caf-streaming-net-remote-nodes --------------------------------------"
init_file remote_nodes ${out_file}
for remote_nodes in {1..32}; do
  echo "-- ${remote_nodes} nodes -----------------------------------------------"
  printf "${remote_nodes}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/caf_streaming_tcp -mnetBench -n$remote_nodes -a1073741824 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
done;

echo "-- ioBenchmark ---------------------------------------------------------"

out_file="evaluation/out/caf-streaming-io-remote-nodes"
echo "-- caf-streaming-io-remote-nodes --------------------------------------"
init_file remote_nodes ${out_file}
for remote_nodes in {1..32}; do
  echo "-- ${remote_nodes} nodes -----------------------------------------------"
  printf "${remote_nodes}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/caf_streaming_tcp -mioBench -n$remote_nodes -a1073741824 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
done;

