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

# -- netBenchmark -------------------------------------------------------------

for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  net_file="${output_folder}/pingpong-net-${pings}-pings"
  init_file ${net_file}
  for num_nodes in {1..64..1}; do
    printf ${num_nodes} >> ${net_file}
    while : ; do
      echo "starting netBench-${num_nodes} nodes"
      ./release/pingpong_tcp -mnetBench -n${num_nodes} -p${pings} -i${duration} >> ${net_file}.out 2> ${net_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
done;

echo ""

for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  io_file="${output_folder}/pingpong-io-${pings}-pings"
  init_file ${io_file}
  for num_nodes in {1..64..1}; do
    printf ${num_nodes} >> ${io_file}
    while : ; do
      echo "starting ioBench-${num_nodes} nodes"
      ./release/pingpong_tcp -mioBench -n${num_nodes} -p${pings} -i${duration} >> ${io_file}.out 2> ${io_file}.err
      [[ $? != 0 ]] || break # if program exited with error rerun it.
    done;
  done;
done;