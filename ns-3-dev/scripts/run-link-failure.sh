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
seed=0

runSimulation () {
  echo "Running simulation with $numFail links failed"
  dirname=results-"$expname-$seed"/"$expname"-link-failure-"$numFail"
  ./waf --run "scratch/sdn-real-topo /home/ddc/Documents/ddc-ns3/topos/$toponame $controller_start_id $controller_end_id $numFail $seed" 2>&1 | tee link-failure-"$numFail".log
  mkdir -p $dirname
  mv controller-*-latency.log $dirname
  cat $dirname/controller-*-latency.log > $dirname/all.log
  mv link-failure-"$numFail".log $dirname
}

seed=4444
numFail=256 runSimulation

#for i in `seq 0 16`; do
#  numFail=$(($i * 10)) runSimulation
#done
#
#seed=5555
#for i in `seq 0 12`; do
#  numFail=$(($i * 100)) runSimulation
#done
#
#seed=6666
#for i in `seq 0 12`; do
#  numFail=$(($i * 100)) runSimulation
#done

