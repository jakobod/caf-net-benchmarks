#!/bin/bash

output_folder="evaluation/out"
rm -rf ${output_folder}
mkdir -p ${output_folder}

# -- netBenchmark -------------------------------------------------------------

echo "------------------------ raw ------------------------"
raw_file="${output_folder}/timing-raw"
./release/pingpong_tcp_blank >> ${raw_file}.out 2> ${raw_file}.err

echo ""

echo "------------------------ net ------------------------"
net_file="${output_folder}/timing-net"
./release/pingpong_tcp_timing -m netBench>> ${net_file}.out 2> ${net_file}.err

echo ""

echo "------------------------ io ------------------------"
io_file="${output_folder}/timing-io"
./release/pingpong_tcp_timing -mioBench >> ${io_file}.out 2> ${io_file}.err
