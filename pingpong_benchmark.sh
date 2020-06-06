#!/bin/bash

output_folder="evaluation/out"
rm -rf ${output_folder}
mkdir -p ${output_folder}

function init_caf_application_ini() {
  echo "[scheduler]" > caf-application.ini
  echo "max-threads=4" >> caf-application.ini
}

# -- netBenchmark -------------------------------------------------------------
# set maximum number of threads for each actor-system.
init_caf_application_ini

for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  net_file_name="${output_folder}/pingpong-timing-net"
  ./release/pingpong_multiple -mnetBench -n1 -p${pings} >> ${net_file_name}.out 2> ${net_file_name}.err
done;

echo ""

for pings in 1 10 100; do
  echo "------------------------ ${pings} pings ------------------------"
  io_file_name="${output_folder}/pingpong-timing-io"
  ./release/pingpong_multiple -mioBench -n1 -p${pings}>> ${io_file_name}.out 2> ${io_file_name}.err
done;
