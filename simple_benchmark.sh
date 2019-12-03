#!/bin/bash

function update_caf_application_ini() {
  file="caf-application.ini"
  echo [middleman] > $file
  echo "serializing_workers=$1" >> $file 
  echo "workers=$2" >> $file
   
}

function init_files() {
  echo "serializer, deserializer, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10," > $1.csv
  rm -rf ${1}-if-load.out
}

output_folder="evaluation/out"
mkdir -p $output_folder

file_name="${output_folder}/4-ser-x-deser"
init_files $file_name
for i in {0..4}; do
  echo "[4:$i]:"
  update_caf_application_ini 4 $i
  sh evaluation/interface_load.sh lo rx_bytes 12 ${file_name}-if-load.out &
  ./release/simple_streaming -i11 -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  sleep 2
done

file_name="${output_folder}/0-ser-x-deser"
init_files $file_name
for i in {0..4}; do
  echo "[0:$i]:"
  update_caf_application_ini 0 $i
  sh evaluation/interface_load.sh lo rx_bytes 12 ${file_name}-if-load.out &
  ./release/simple_streaming -i11 -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  sleep 2
done

file_name="${output_folder}/x-ser-4-deser"
init_files $file_name
for i in {0..4}; do
  echo "[$i:4]:"
  update_caf_application_ini $i 4
  sh evaluation/interface_load.sh lo rx_bytes 12 ${file_name}-if-load.out &
  ./release/simple_streaming -i11 -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  sleep 2
done

file_name="${output_folder}/x-ser-0-deser"
init_files $file_name
for i in {0..4}; do
  echo "[$i:0]:"
  update_caf_application_ini $i 0
  sh evaluation/interface_load.sh lo rx_bytes 12 ${file_name}-if-load.out &
  ./release/simple_streaming -i11 -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  sleep 2
done

file_name="${output_folder}/io-x-deser"
init_files $file_name
for i in {0..4}; do
  echo "[$i:0]:"
  update_caf_application_ini 0 $i
  sh evaluation/interface_load.sh lo rx_bytes 12 ${file_name}-if-load.out &
  ./release/simple_streaming -i11 -mioBench >> ${file_name}.csv 2> ${file_name}.err
  sleep 2
done
