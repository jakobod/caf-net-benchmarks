#!/bin/bash

mkdir merged
rm -rf merged/*

function merge() {
  program_file=${1}.csv
  if_load_file=${1}-if-load.out
  out_file=$2

  # print only first line of output from program
  awk '{if(NR==1) print $0}' ${program_file} > ${out_file} # cool!
  # delete line afterwards
  sed -i '1d' ${program_file} # check
  # remove messages/s from output and leave number of workers
  awk '{print $1,$2}' ${program_file} > tmp && mv tmp ${program_file} #check

  # remove first and last result from if_load
  awk '{$1=$12=""; print $0}' ${if_load_file} > tmp && mv tmp ${if_load_file}
  # Concatenate both files by column
  paste -d , ${program_file} ${if_load_file} >> ${out_file}

  # Remove double commas from resulting file.
  sed 's/,,/,/g' ${out_file} > tmp && mv tmp ${out_file}
}

for file in 4-ser-x-deser 0-ser-x-deser x-ser-4-deser x-ser-0-deser io-x-deser; do
  echo "out/${file}.csv"
  merge out/${file} merged/${file}.csv
done
