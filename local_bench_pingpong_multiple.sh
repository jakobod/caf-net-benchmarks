#!/bin/bash

output_folder="evaluation/out"
rm -rf ${output_folder}
mkdir -p ${output_folder}

function init_file() {
  echo "num_pings, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10," > ${1}.out
}

function init_caf_application_ini() {
  echo "[scheduler]" > caf-application.ini
  echo "max-threads=4" >> caf-application.ini
}

# -- netBenchmark -------------------------------------------------------------
# set maximum number of threads for each actor-system.
init_caf_application_ini

for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  net_file_name="${output_folder}/pingpong-multiple-${pings}-pings-net"
  init_file ${net_file_name}
  for num_nodes in {1..32..1}; do
    echo "starting netBench-${num_nodes}"
    ./release/pingpong_multiple -mnetBench -n${num_nodes} -p${pings} >> ${net_file_name}.out 2> ${net_file_name}.err
  done;
done;

echo ""

for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  io_file_name="${output_folder}/pingpong-multiple-${pings}-pings-io"
  init_file ${io_file_name}
  for num_nodes in {1..32..1}; do
    echo "starting ioBench-${num_nodes}"
    ./release/pingpong_multiple -mioBench -n${num_nodes} -p${pings}>> ${io_file_name}.out 2> ${io_file_name}.err
  done;
  echo ""
done;
