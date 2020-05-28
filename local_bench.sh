#!/bin/bash

output_folder="evaluation/out"
rm -rf ${output_folder}
mkdir -p ${output_folder}

function init_file() {
  echo "num_pings, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${1}.out
}

# -- netBenchmark -------------------------------------------------------------

net_file_name="${output_folder}/pingpong-waiting-net"
init_file ${net_file_name}
for num_pings in 1 10 100 1000 10000 100000; do
  echo "netBench-${num_pings}:"
  ./build/pingpong -mnetBench -n${num_pings} >> ${net_file_name}.out 2> ${net_file_name}.err
  echo 
done;

io_file_name="${output_folder}/pingpong-waiting-io"
init_file ${io_file_name}
for num_pings in 1 10 100 1000 10000 100000; do
  echo "ioBench-${num_pings}:"
  ./build/pingpong -mioBench -n${num_pings} >> ${io_file_name}.out 2> ${io_file_name}.err
  echo 
done;
