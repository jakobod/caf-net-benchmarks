#!/bin/bash

function update_caf_application_ini() {
  file="caf-application.ini"
  echo [middleman] > $file
  echo "serializing_workers=$1" >> $file 
  echo "workers=$2" >> $file
   
}

function init_file() {
  echo "serializer, deserializer, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > $1.out
}

output_folder="evaluation/out"
mkdir -p $output_folder

file_name="${output_folder}/net-0-ser-x-deser"
init_file $file_name
rm -rf ${file_name}-if-load.out
for i in {0..4}; do
  echo "[0:$i]:"
  sh evaluation/interface_load.sh lo rx_bytes 12 ${file_name}-if-load.out &
  update_caf_application_ini 0 $i
  ./release/simple_streaming -i11 -mnetBench >> ${file_name}.out 2> ${file_name}.err
  echo ""
  echo ""
  sleep 2
done

#file_name="${output_folder}/io-x-deserializer"
#init_file $file_name
#for i in {0..4}; do
#  echo "[$i:0]:"
#  update_caf_application_ini 0 $i
#  ./release/simple_streaming -mioBench >> ${file_name}.csv 2> ${file_name}.err
#  echo ""
#  echo ""
#done
