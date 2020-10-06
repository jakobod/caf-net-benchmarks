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

echo "-- netBenchmark -------------------------------------------------------"

out_file="evaluation/out/pingpong-tcp-net-message-size"
echo "pingpong-tcp-net-message-size"
init_file message_size ${out_file}
message_size=1
while [ $message_size -le 1024 ]; do
  echo "-- message-size = ${message_size} -----------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/pingpong_tcp -mnetBench -p10000 -s$message_size >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  message_size=$((message_size*2))
done;

echo "-- ioBenchmark --------------------------------------------------------"

out_file="evaluation/out/pingpong-tcp-io-message-size"
echo "pingpong-tcp-io-message-size"
init_file message_size ${out_file}
message_size=1
while [ $message_size -le 1024 ]; do
  echo "-- message-size = ${message_size} -----------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/pingpong_tcp -mioBench -p10000 -s$message_size >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  message_size=$((message_size*2))
done;
