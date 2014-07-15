#!/bin/bash

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
  rm -rf $dir/*controller*log
done
