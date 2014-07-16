#!/bin/bash

# No reversal delay
for i in `seq 1 5`; do
  name=sinner
  seed=$i$i$i$i
  screenName=$name-$seed
  screen -dmS $screenName sh -c "DDC_SEED=$seed scripts/run-link-failure.sh 1239.bb $name 3 3600 1180 60s 0ms 10ms"
  echo "Started running screen $screenName"
done

# 1ms reversal delay
for i in `seq 1 5`; do
  name=dinner
  seed=$i$i$i$i
  screenName=$name-$seed
  screen -dmS $screenName sh -c "DDC_SEED=$seed scripts/run-link-failure.sh 1239.bb $name 3 3600 1180 60s 1ms 10ms"
  echo "Started running screen $screenName"
done

# 10ms reversal delay
for i in `seq 1 5`; do
  name=winner
  seed=$i$i$i$i
  screenName=$name-$seed
  screen -dmS $screenName sh -c "DDC_SEED=$seed scripts/run-link-failure.sh 1239.bb $name 3 3600 1180 60s 10ms 10ms"
  echo "Started running screen $screenName"
done

