#!/bin/bash

output_folder="evaluation/out"
mkdir -p ${output_folder}

function init_file() {
  printf "${1}, " > ${2}.out
  for i in {1..50}; do
    printf "value${i}, " >> ${2}.out
  done;
  echo "" >> ${2}.out
}


echo "-- rawBenchmark ---------------------------------------------------------"
echo "blank-streaming-raw-message-size"
out_file="evaluation/out/blank-streaming-raw-message-size"
init_file message_size ${out_file}
message_size=512
while [ $message_size -le 140000 ]; do
  echo "-- message-size = ${message_size} ---------------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in {1..50}; do
    while : ; do
      ./release/streaming_raw_tcp -m$message_size -a104857600 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  echo "-- message-size = ${message_size} DONE ----------------------------------"
  message_size=$((message_size*2))
done;

echo "-- netBenchmark ---------------------------------------------------------"
echo "blank-streaming-net-message-size"
out_file="evaluation/out/blank-streaming-net-message-size"
init_file message_size ${out_file}
message_size=512
while [ $message_size -le 140000 ]; do
  echo "-- message-size = ${message_size} ---------------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in {1..50}; do
    while : ; do
      ./release/blank_streaming_tcp -mnetBench -s$message_size -a104857600 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  echo "-- message-size = ${message_size} DONE ----------------------------------"
  message_size=$((message_size*2))
done;

# out_file="evaluation/out/blank-streaming-net-remote-nodes"
# echo "-- blank-streaming-net-remote-nodes --------------------------------------"
# init_file remote_nodes ${out_file}
# for remote_nodes in {1..32}; do
#   echo "-- ${remote_nodes} nodes -----------------------------------------------"
#   printf "${remote_nodes}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mnetBench -n$remote_nodes -s10240 -a1073741824 >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
# done;


echo "-- ioBenchmark ---------------------------------------------------------"

echo "blank-streaming-io-message-size"
out_file="evaluation/out/blank-streaming-io-message-size"
init_file message_size ${out_file}
message_size=512
while [ $message_size -le 140000 ]; do
  echo "-- message-size = ${message_size} ---------------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in {1..50}; do
    while : ; do
      ./release/blank_streaming_tcp -mioBench -s$message_size -a104857600 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  echo "-- message-size = ${message_size} DONE ----------------------------------"
  message_size=$((message_size*2))
done;

# out_file="evaluation/out/blank-streaming-io-remote-nodes"
# echo "-- blank-streaming-io-remote-nodes --------------------------------------"
# init_file remote_nodes ${out_file}
# for remote_nodes in {1..32}; do
#   echo "-- ${remote_nodes} nodes -----------------------------------------------"
#   printf "${remote_nodes}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mioBench -n$remote_nodes -s10240 -a1073741824 >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
# done;