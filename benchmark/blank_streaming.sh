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
echo "blank-streaming-net-message-size"
out_file="evaluation/out/blank-streaming-net-message-size"
init_file message_size ${out_file}
for message_size in 1024 10240 102400 1024000; do
  echo "-- message-size = ${message_size} ---------------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/blank_streaming_tcp -mnetBench -s$message_size -a104857600 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  echo "-- message-size = ${message_size} DONE ----------------------------------"
done;


# out_file="evaluation/out/blank-streaming-net-amount-1k-chunks"
# echo "blank-streaming-net-amount-1k-chunks"
# init_file amount ${out_file}
# for amount in 1048576 10485760 104857600 1048576000; do
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
#   printf "${amount}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mnetBench -s$1024 -a$amount >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
#   echo "" >> ${out_file}.out
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
# done;


# out_file="evaluation/out/blank-streaming-net-amount-10k-chunks"
# echo "-- blank-streaming-net-amount-10k-chunks ---------------------------------"
# init_file amount ${out_file}
# for amount in 1048576 10485760 104857600 1048576000; do
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
#   printf "${amount}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mnetBench -s$10240 -a$amount >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
#   echo "" >> ${out_file}.out
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
# done;

# out_file="evaluation/out/blank-streaming-net-amount-100k-chunks"
# echo "-- blank-streaming-net-amount-100k-chunks ---------------------------------"
# init_file amount ${out_file}
# for amount in 1048576 10485760 104857600 1048576000; do
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
#   printf "${amount}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mnetBench -s$102400 -a$amount >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
#   echo "" >> ${out_file}.out
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
# done;

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
for message_size in 1024 10240 102400 1024000; do
  echo "-- message-size = ${message_size} ---------------------------------------"
  printf "${message_size}, " >> ${out_file}.out
  for i in 0 1 2 3 4 5 6 7 8 9; do
    while : ; do
      ./release/blank_streaming_tcp -mioBench -s$message_size -a104857600 >> ${out_file}.out 2> ${out_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
  echo "" >> ${out_file}.out
  echo "-- message-size = ${message_size} DONE ----------------------------------"
done;


# out_file="evaluation/out/blank-streaming-io-amount-1k-chunks"
# echo "blank-streaming-io-amount-1k-chunks"
# init_file amount ${out_file}
# for amount in 1048576 10485760 104857600 1048576000; do
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
#   printf "${amount}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mioBench -s$1024 -a$amount >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
#   echo "" >> ${out_file}.out
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
# done;

# out_file="evaluation/out/blank-streaming-io-amount-10k-chunks"
# echo "-- blank-streaming-io-amount-10k-chunks ---------------------------------"
# init_file amount ${out_file}
# for amount in 1048576 10485760 104857600 1048576000; do
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
#   printf "${amount}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mioBench -s$10240 -a$amount >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
#   echo "" >> ${out_file}.out
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
# done;

# out_file="evaluation/out/blank-streaming-io-amount-100k-chunks"
# echo "-- blank-streaming-io-amount-100k-chunks ---------------------------------"
# init_file amount ${out_file}
# for amount in 1048576 10485760 104857600 1048576000; do
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
#   printf "${amount}, " >> ${out_file}.out
#   for i in 0 1 2 3 4 5 6 7 8 9; do
#     while : ; do
#       ./release/blank_streaming_tcp -mioBench -s$102400 -a$amount >> ${out_file}.out 2> ${out_file}.err
#       [[ $? != 0 ]] || break # if program exited with error rerun it.
#     done;
#   done;
#   echo "" >> ${out_file}.out
#   echo "-- streaming-amount = ${amount} ----------------------------------------"
# done;

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