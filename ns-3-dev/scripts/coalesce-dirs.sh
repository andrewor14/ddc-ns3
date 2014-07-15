#!/bin/bash

# This must be run from the ns-3-dev directory
cwd=$(basename $PWD)
if ! [[ "$cwd" == "ns-3-dev" ]]; then
  echo "You must run this in the ns-3-dev directory!"
  exit 1
fi

if [[ $# -lt 1 ]]; then
  echo "Usage: coalesce-dirs.sh [experiment name]"
  exit 1
fi

exp_name=$1
final_dir="results-final-$exp_name"
mkdir $final_dir
cp -R results-"$exp_name"-*/* $final_dir
for dir in $final_dir/*; do
  cat $dir/all.log.* >> $dir/all.log
  rm -rf $dir/*controller*log*
done
