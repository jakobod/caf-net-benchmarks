#!/bin/sh
# usage: 
# $1 = interface name
# $2 = rx_bytes | tx_bytes
# $3 = seconds this program should run
# $4 = file_name

iface=$1
direction=$2
seconds=$3
file_name=$4

declare -a arr
F="/sys/class/net/${iface}/statistics/${direction}"
BPS=0
for (( c=1; c<=$seconds; c++ )); do 
  X=`cat $F`; sleep 1; Y=`cat $F`; BPS="$(((Y-X)/1))";
  arr+=($BPS)
done

for val in "${arr[*]}"; do
  printf "%d, " $val >> $file_name
done
echo >> $file_name
