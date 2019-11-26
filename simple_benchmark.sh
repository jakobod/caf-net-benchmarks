#!/bin/bash

function update_caf_application_ini() {
  file="caf-application.ini"
  rm -rf $file
  echo [middleman] > $file
  echo "workers=$1" >> $file
  echo "serializing_workers=$2" >> $file
  
}

file_name="evaluation/simple_streaming"
echo "num_serializing_workers, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10" > ${file_name}.out
for i in {0..4}; do
  echo "Starting benchmark deserializing_workers = 4 and serializing_workers = $i:"
  update_caf_application_ini 4 $i
  ./release/simple_streaming -mnetBench >> ${file_name}.out 2> ${file_name}.err
  echo ""
  echo ""
done

