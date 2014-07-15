#!/bin/bash

# This must be run from the ns-3-dev directory
cwd=$(basename $PWD)
if ! [[ "$cwd" == "ns-3-dev" ]]; then
  echo "You must run this in the ns-3-dev directory!"
  exit 1
fi

if [[ $# -lt 4 ]]; then
  echo "Usage: run-link-failure.sh [topology name] [experiment name] [start controller ID] [end controller ID] [reverse delay]"
  exit 1
fi

toponame=$1
expname=$2
controller_start_id=$3
controller_end_id=$4
reverse_delay=$5
seed=0

runSimulation () {
  echo "Running simulation with $num_fail links failed (seed = $seed) with delay $reverse_delay""us"
  dirname=results-"$expname-$seed"/"$expname"-link-failure-"$num_fail"
  ./waf --run "scratch/sdn-real-topo /mnt/andrew/ddc-ns3/topos/$toponame $controller_start_id $controller_end_id $num_fail $seed $reverse_delay" 2>&1 | tee link-failure-"$num_fail".log
  mkdir -p $dirname
  mv controller-*-latency.log $dirname
  cat $dirname/controller-*-latency.log > $dirname/all.log
  mv $dirname/all.log $dirname/"all.log.$seed"
  mv link-failure-"$num_fail".log $dirname
}

for i in `seq 0 12`; do
  num_fail=$(($i * 100))
  seed=6127 runSimulation
  seed=4500 runSimulation
  seed=8994 runSimulation
done

finaldir="results-final-$expname"
mkdir $finaldir
cp -R results-"$expname"-*/* $finaldir
for dir in $finaldir/*; do
  cat $dir/all.log.* >> $dir/all.log
  rm -rf $dir/controller*log
done

