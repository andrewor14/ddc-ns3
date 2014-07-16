#!/bin/bash

# This must be run from the ns-3-dev directory
cwd=$(basename $PWD)
if ! [[ "$cwd" == "ns-3-dev" ]]; then
  echo "You must run this in the ns-3-dev directory!"
  exit 1
fi

if [[ $# -lt 8 ]]; then
  echo "Usage: run-link-failure.sh
          [topology name]
          [experiment name]
          [num controllers]
          [controller max epoch]
          [num links to fail]
          [recovery interval]
          [reversal delay]
          [link latency]"
  exit 1
fi

topo_name=$1
exp_name=$2
num_controllers=$3
controller_max_epoch=$4
num_links_to_fail=$5 # upper bound
link_recovery_interval=$6
reversal_delay=$7
link_latency=$8

if [[ -z "$DDC_SEED" ]]; then
  echo "You must set DDC_SEED!"
  exit 1
fi

seed=$DDC_SEED # This varies
link_failure_interval=0 # This varies
switch_max_violation=20

runSimulation () {
  echo "Running simulation $exp_name with failure interval $link_failure_interval ($seed)"
  dirname=results-$exp_name-$seed/$exp_name-link-failure-$link_failure_interval
  ./waf --run "scratch/sdn-real-topo \
    --TopologyFile=/mnt/andrew/ddc-ns3/topos/$topo_name \
    --ExperimentName=$exp_name \
    --NumControllers=$num_controllers \
    --ControllerMaxEpoch=$controller_max_epoch \
    --SwitchMaxViolation=$switch_max_violation \
    --NumLinksToFail=$num_links_to_fail \
    --MeanRecoveryInterval=$link_recovery_interval \
    --MeanFailureInterval=$link_failure_interval \
    --Seed=$seed \
    --ReversalDelay=$reversal_delay \
    --LinkLatency=$link_latency" 2>&1 | tee $exp_name-link-failure-$link_failure_interval.log.$seed
  mkdir -p $dirname
  mv $exp_name-controller*.log.$seed $dirname
  mv $exp_name-link-failure-$link_failure_interval.log.$seed $dirname
  cat $dirname/$exp_name-controller*log.$seed > $dirname/all.log.$seed
}

link_failure_interval=10s runSimulation # 0.1 link / s
link_failure_interval=1s runSimulation # 1 link / s
link_failure_interval=500ms runSimulation # 2 links / s
link_failure_interval=100ms runSimulation # 10 links / s
link_failure_interval=50ms runSimulation # 20 links / s
link_failure_interval=10ms runSimulation # 100 links / s

