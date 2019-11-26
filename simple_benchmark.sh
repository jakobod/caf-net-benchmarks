#!/bin/bash

function update_caf_application_ini() {
  file="caf-application.ini"
  rm -rf $file
  echo [middleman] > $file
  echo "workers=$1" >> $file
  echo "serializing_workers=$2" >> $file
  
}

file_name="evaluation/4-deserializing-workers"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
  echo "Starting benchmark deserializing_workers = 4 and serializing_workers = $i:"
  update_caf_application_ini 4 $i
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

file_name="evaluation/4-serializing-workers"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
  echo "Starting benchmark deserializing_workers = $i and serializing_workers = 4:"
  update_caf_application_ini $i 4
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

file_name="evaluation/0-serializing-workers"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
  echo "Starting benchmark deserializing_workers = $i and serializing_workers = 4:"
  update_caf_application_ini $i 0
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

file_name="evaluation/0-deserializing-workers"
echo "num_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${file_name}.csv
for i in {0..4}; do
  echo "Starting benchmark deserializing_workers = 4 and serializing_workers = $i:"
  update_caf_application_ini 4 $i
  ./release/simple_streaming -mnetBench >> ${file_name}.csv 2> ${file_name}.err
  echo ""
  echo ""
done

