#!/bin/bash

output_folder="evaluation/out"
rm -rf ${output_folder}
mkdir -p ${output_folder}

function init_file() {
  echo "num_pings, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10," > ${1}.out
}

# -- netBenchmark -------------------------------------------------------------
:
for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  net_file="${output_folder}/pingpong-net-${pings}-pings"
  init_file ${net_file}
  for num_nodes in {1..64..1}; do
    echo "starting netBench-${num_nodes} nodes"
    ./release/pingpong_tcp -mnetBench -n${num_nodes} -p${pings} >> ${net_file}.out 2> ${net_file}.err
  done;
done;

echo ""

for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  io_file="${output_folder}/pingpong-io-${pings}-pings"
  init_file ${io_file}
  for num_nodes in {1..64..1}; do
    echo "starting ioBench-${num_nodes} nodes"
    ./release/pingpong_tcp -mioBench -n${num_nodes} -p${pings} >> ${io_file}.out 2> ${io_file}.err
  done;
done;
