#!/bin/bash

for i in `seq 0 15`; do
  k=$(($i * 10))
  cat ../results-7890/ft8-link-failure-$k/all.log >> ft8-link-failure-$k/all.log
done
