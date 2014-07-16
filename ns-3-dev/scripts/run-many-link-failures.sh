#!/bin/bash

for i in `seq 1 5`; do
  i=$i$i$i$i
  screen -dmS grape-$i sh -c "DDC_SEED=$i scripts/run-link-failure.sh ft16.topo grape 3 3600 1640 60s 10ms 10us"
  echo "Started running screen with seed $i"
done
