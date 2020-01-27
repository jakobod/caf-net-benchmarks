#!/bin/bash

mobi3="10.28.255.13"
mobi7="10.28.255.17"
output_folder="evaluation/out"
rm -rf ${output_folder}
mkdir -p ${output_folder}

function update_caf_application_ini() {
  file="caf-application.ini"
  echo [middleman] > $file
  echo "serializing_workers=$1" >> $file
  echo "workers=$2" >> $file
 
  echo SSH
  remote_file="/users/otto/caf-net-benchmarks/$file"
  ssh mobi3 "echo [middleman] > $remote_file"
  ssh mobi3 "echo serializing_workers=$1 >> $remote_file"
  ssh mobi3 "echo workers=$2 >> $remote_file "
}

function init_file() {
  echo "serializer, deserializer, size, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10, value11," > ${1}.out
}

function benchmark() {
  mode=${1}
  iterations=${2}
  host=${3}
  data_size=${4}
  file_name=${5}
  ssh mobi3 "/users/otto/caf-net-benchmarks/release/benchmark_server -m${mode} -d${data_size}" >> ${file_name}-server.out 2> ${file_name}-server.err &
  sleep 2
  ./release/benchmark_client -m${mode} -i${iterations} -H${host} -d${data_size} >> ${file_name}.out 2> ${file_name}.err
  sleep 2
}

# -- netBenchmark -------------------------------------------------------------

for i in 0 1 2 4 8; do
  file_name="${output_folder}/net-${i}-0"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "netBench-${size}-${i}-0:"
    update_caf_application_ini ${i} 0
    benchmark netBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;

for i in 1 2 4 8; do
  file_name="${output_folder}/net-0-${i}"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "netBench-${size}-0-${i}:"
    update_caf_application_ini 0 ${i}
    benchmark netBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;

for i in 0 1 2 4 8; do
  file_name="${output_folder}/net-4-${i}"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "netBench-${size}-4-${i}:"
    update_caf_application_ini 4 ${i}
    benchmark netBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;


for i in 0 1 2 8; do
  file_name="${output_folder}/net-${i}-4"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "netBench-${size}-${i}-4:"
    update_caf_application_ini ${i} 4
    benchmark netBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;

# -- IO benchmark -------------------------------------------------------------
for i in 0 1 2 4 8; do
  file_name="${output_folder}/io-${i}-0"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "ioBench-${size}-${i}-0:"
    update_caf_application_ini ${i} 0
    benchmark ioBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;

for i in 1 2 4 8; do
  file_name="${output_folder}/io-0-${i}"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "ioBench-${size}-0-${i}:"
    update_caf_application_ini 0 ${i}
    benchmark ioBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;

for i in 0 1 2 4 8; do
  file_name="${output_folder}/io-4-${i}"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "ioBench-${size}-4-${i}:"
    update_caf_application_ini 4 ${i}
    benchmark ioBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;


for i in 0 1 2 8; do
  file_name="${output_folder}/io-${i}-4"
  init_file ${file_name}
  for size in 1 10 100 1000 10000; do
    echo "ioBench-${size}-${i}-4:"
    update_caf_application_ini ${i} 4
    benchmark ioBench 11 ${mobi3} ${size} ${file_name}
    echo
  done;
done;
