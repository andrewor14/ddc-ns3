#!/bin/bash

if [[ $# -lt 1 ]]; then
  echo "Usage: averageLinks.sh [filename]"
  exit 1
fi

cat $1 | sed s/###.*+++//g > $1.parsed
/mnt/andrew/ddc-ns3/ns-3-dev/scripts/averageLatency.py $1.parsed
