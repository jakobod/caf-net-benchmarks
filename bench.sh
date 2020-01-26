#!/bin/bash

function init_file() {
  echo "serializer, deserializer, size, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${1}.out
}

outfile=$1

output_folder="evaluation/out"
mkdir -p $output_folder

function benchmark() {
  mode=${1}
  iterations=${2}
  host=${3}
  data_size=${4}
  file_name=${5}
  ssh mobi3 '/users/otto/caf-net-benchmarks/release/benchmark_server -m${mode} -d${data_size}' >> /dev/null 2> /dev/null &
  sleep 1
  ./release/benchmark_client -m${mode} -i${iterations} -H${host} -d${data_size} >> ${file_name}.out 2> ${file_name}.err
}

for size in 1 10 100 1000 10000; do
  for i in {0..4}; do
    file_name="${output_folder}/net-${size}-${i}-0"
    init_file ${file_name}
    echo "netBench-${size}-${i}-0:"
    benchmark netBench 11 localhost ${size} ${file_name}
    echo
  done;
done;

for size in 1 10 100 1000 10000; do
  for i in {0..4}; do
    file_name="${output_folder}/net-${size}-0-${i}"
    init_file ${file_name}
    echo "netBench-${size}-0-${i}:"
    benchmark netBench 11 localhost ${size} ${file_name}
    echo
  done;
done;

for size in 1 10 100 1000 10000; do
  for i in {0..4}; do
    file_name="${output_folder}/net-${size}-4-${i}"
    init_file ${file_name}
    echo "netBench-${size}-4-${i}:"
    benchmark netBench 11 localhost ${size} ${file_name}
    echo
  done;
done;

for size in 1 10 100 1000 10000; do
  for i in {0..4}; do
    file_name="${output_folder}/net-${size}-${i}-4"
    init_file ${file_name}
    echo "netBench-${size}-${i}-4:"
    benchmark netBench 11 localhost ${size} ${file_name}
    echo
  done;
done;
