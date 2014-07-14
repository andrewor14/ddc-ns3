#!/bin/bash

# This must be run from the ns-3-dev directory
cwd=$(basename $PWD)
if ! [[ "$cwd" == "ns-3-dev" ]]; then
  echo "You must run this in the ns-3-dev directory!"
  exit 1
fi

if [[ $# -lt 4 ]]; then
  echo "Usage: run-link-failure.sh [topology name] [experiment name] [start controller ID] [end controller ID]"
  exit 1
fi

toponame=$1
expname=$2
controller_start_id=$3
controller_end_id=$4
percent=""
decimal=""

runSimulation () {
  echo "Running simulation with "$percent" links failed"
  dirname=results/"$expname"-"$percent"
  ./waf --run "scratch/sdn-real-topo /home/ddc/Documents/ddc-ns3/topos/$toponame $controller_start_id $controller_end_id $decimal" 2>&1 | tee link-percent-"$percent".log
  mkdir -p $dirname
  mv controller-*-latency.log $dirname
  cat $dirname/controller-*-latency.log > $dirname/all.log
  mv link-percent-"$percent".log $dirname
}

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
