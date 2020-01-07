#!/bin/bash

function init_file() {
  echo "serializer, deserializer, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > $1.out
}

outfile=$1

output_folder="evaluation/out"
mkdir -p $output_folder

file_name="${output_folder}/${outfile}-net"
init_file $file_name
echo netBench:
./release/pingpong -mnetBench >> ${file_name}.out 2> /dev/null
echo

file_name="${output_folder}/${outfile}-io"
init_file $file_name
echo ioBench
./release/pingpong -mioBench >> ${file_name}.out 2> /dev/null
echo
