#!/bin/bash

output_folder="evaluation/out"
rm -rf ${output_folder}
mkdir -p ${output_folder}

duration=60

function init_file() {
  printf "num_pings" > ${1}.out
  for i in {1..${duration}..1}; do
    echo "value${i}, " >> ${1}.out
  done;
  echo "" >> ${1}.out
}

# -- Benchmark -------------------------------------------------------------

net_file_name="${output_folder}/streaming-net"
init_file ${net_file_name}
for num_nodes in {1..64..1}; do
  echo "netBench-${num_nodes}:"
  ./release/caf_streaming -mnetBench -n${num_nodes} -i${duration} >> ${net_file_name}.out 2> ${net_file_name}.err
  echo 
done;

echo ""

io_file_name="${output_folder}/streaming-io"
init_file ${io_file_name}
for num_nodes in {1..64..1}; do
  echo "ioBench-${num_nodes}:"
  ./release/caf_streaming -mioBench -n${num_nodes} -i${duration} >> ${io_file_name}.out 2> ${io_file_name}.err
  echo 
done;
