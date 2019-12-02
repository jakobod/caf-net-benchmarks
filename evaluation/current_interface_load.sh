#!/bin/sh
S=1; F=/sys/class/net/lo/statistics/tx_bytes
echo $F
BPS=999999
while true
do
  X=`cat $F`; sleep $S; Y=`cat $F`; BPS="$(((Y-X)/S))";
  echo BPS is currently $BPS
done
offlineimap
