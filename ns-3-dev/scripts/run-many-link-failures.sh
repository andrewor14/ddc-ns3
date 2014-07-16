#!/bin/bash

# No reversal delay
for i in `seq 1 5`; do
  name=rhine
  seed=$i$i$i$i
  screenName=$name-$seed
  screen -dmS $screenName sh -c "DDC_SEED=$seed scripts/run-link-failure.sh ft16.topo $name 3 3600 1640 60s 0ms 10us"
  echo "Started running screen $screenName"
done

# 1ms reversal delay
for i in `seq 1 5`; do
  name=wine
  seed=$i$i$i$i
  screenName=$name-$seed
  screen -dmS $screenName sh -c "DDC_SEED=$seed scripts/run-link-failure.sh ft16.topo $name 3 3600 1640 60s 1ms 10us"
  echo "Started running screen $screenName"
done

## 10ms reversal delay
#for i in `seq 1 5`; do
#  name=pine
#  seed=$i$i$i$i
#  screenName=$name-$seed
#  screen -dmS $screenName sh -c "DDC_SEED=$seed scripts/run-link-failure.sh ft16.topo $name 3 3600 1640 60s 10ms 10us"
#  echo "Started running screen $screenName"
#done

