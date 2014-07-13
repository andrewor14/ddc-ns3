#!/bin/bash

percent=""
decimal=""

runSimulation () {
  echo "Running simulation with "$percent" links failed"
  dirname=results/ft8-"$percent"
  ./waf --run "scratch/sdn-real-topo /home/ddc/Documents/ddc-ns3/topos/FatTree_8.topo 10 20 $decimal" 2>&1 | tee link-percent-"$percent".log
  mkdir $dirname
  mv controller-*-latency.log $dirname
  cat $dirname/controller-*-latency.log > $dirname/all.log
  mv link-percent-"$percent".log $dirname
}

cd ..

# 0%
percent="0%"
decimal="0"
runSimulation

# 10% - 90% (inclusive)
for i in `seq 1 9`; do
  percent="$i""0%"
  decimal="0.""$i"
  runSimulation
done

# 100%
percent="100%"
decimal="1"
runSimulation
