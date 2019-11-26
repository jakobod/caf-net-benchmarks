#!/bin/bash

function update_caf_application_ini() {
  file="caf-application.ini"
  rm -rf $file
  echo [middleman] > $file
  echo "workers=$1" >> $file
  echo "serializing_workers=$2" >> $file
  
}

for i in {0..4}; do
  echo "Starting benchmark deserializing_workers = 4 and serializing_workers = $i:"
  file_name=streaming-4-$i
  update_caf_application_ini 4 $i
  ./release/simple_streaming -mnetBench > ${file_name}.out 2> ${file_name}.err
  echo "done"
  echo ""
done

paste streaming-4-0.out streaming-4-1.out streaming-4-2.out streaming-4-3.out streaming-4-4.out -d "," > concatenated.csv
