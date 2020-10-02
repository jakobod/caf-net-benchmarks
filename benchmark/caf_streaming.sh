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

# out_file="evaluation/out/caf-streaming-net-variable-size"
# init_file message_size ${out_file}
# message_size=1
# while [ $message_size -lt 40000 ]; do
#   echo "------------------------ ${message_size} Bytes ------------------------"
#   printf "${message_size}, " >> ${out_file}.out
#   while : ; do
#     ./release/caf_streaming_tcp -mnetBench -s$message_size >> ${out_file}.out 2> ${out_file}.err
#     [[ $? != 0 ]] || break # if program exited with error rerun it.
#   done;
#   message_size=$((message_size*2))
# done;

out_file="evaluation/out/caf-streaming-net-variable-remote-nodes"
init_file remote_nodes ${out_file}
remote_nodes=1
while [ $remote_nodes -le 64 ]; do
  echo "---------------------- ${remote_nodes} nodes -----------------------"
  printf "${remote_nodes}, " >> ${out_file}.out
  while : ; do
    ./release/caf_streaming_tcp -mnetBench -n$remote_nodes >> ${out_file}.out 2> ${out_file}.err
    [[ $? != 0 ]] || break # if program exited with error rerun it.
  done;
  remote_nodes=$((remote_nodes+1))
done;


echo "-- ioBenchmark ---------------------------------------------------------"

# out_file="evaluation/out/caf-streaming-io-variable-size"
# init_file message_size ${out_file}
# message_size=1
# while [ $message_size -lt 40000 ]; do
#   echo "------------------------ ${message_size} Bytes ------------------------"
#   printf "${message_size}, " >> ${out_file}.out
#   while : ; do
#     ./release/caf_streaming_tcp -mioBench -s$message_size >> ${out_file}.out 2> ${out_file}.err
#     [[ $? != 0 ]] || break # if program exited with error rerun it.
#   done;
#   message_size=$((message_size*2))
# done;

out_file="evaluation/out/caf-streaming-io-variable-remote-nodes"
init_file remote_nodes ${out_file}
remote_nodes=1
while [ $remote_nodes -le 64 ]; do
  echo "---------------------- ${remote_nodes} nodes -----------------------"
  printf "${remote_nodes}, " >> ${out_file}.out
  while : ; do
    ./release/caf_streaming_tcp -mioBench -n$remote_nodes >> ${out_file}.out 2> ${out_file}.err
    [[ $? != 0 ]] || break # if program exited with error rerun it.
  done;
  remote_nodes=$((remote_nodes+1))
done;

