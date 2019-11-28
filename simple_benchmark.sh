#!/bin/bash

function update_caf_application_ini() {
  file="caf-application.ini"
  echo [middleman] > $file
  echo "serializing_workers=$1" >> $file 
  echo "workers=$2" >> $file
   
}
output_folder="evaluation/out"
mkdir -p $output_folder

file_name="${output_folder}/4serializer-x-deserializer"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
  echo "[4:$i]:"
  update_caf_application_ini 4 $i
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

file_name="${output_folder}/0serializer-x-deserializer"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
    echo "[0:$i]:"
  update_caf_application_ini 0 $i
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

file_name="${output_folder}/x-serializer-4-deserializer"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
  echo "[$i:4]:"
  update_caf_application_ini $i 4
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

file_name="${output_folder}/x-serializer-0-deserializer"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
  echo "[$i:0]:"
  update_caf_application_ini $i 0
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

