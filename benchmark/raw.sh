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


out_file="evaluation/out/pingpong-tcp-raw-message-size"
echo "pingpong-tcp-raw-message-size"
init_file message_size ${out_file}
message_size=1
while [ $message_size -le 4096 ]; do
  echo "-- message-size = ${message_size} -----------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in {0..50}; do
    while : ; do
      ./release/pingpong_raw_tcp -a10000 -m$message_size >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  message_size=$((message_size*2))
done;

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

