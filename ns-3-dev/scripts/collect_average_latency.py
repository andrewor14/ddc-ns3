#!/usr/bin/env python

from average_latency import *
from average_failed_links import *
import sys
import os
import re

if __name__ == "__main__":
  if len(sys.argv) < 3:
    print "Usage: collect_average_latency.py [base dir] [latency percentile]"
    sys.exit(1)

  baseDir = os.path.abspath(sys.argv[1])
  
  # Return (number of links failed, average control latency) pairs
  results = {}
  for f in sorted(os.listdir(baseDir)):
    #key = re.compile("link-failure-.*").search(f)
    #if key is None:
    #  continue
    #key = key.group().replace("link-failure-", "")
    f = os.path.join(baseDir, f)
    key = averageFailedLinks(f + "/all.log")
    value = averageLatency(f + "/all.log", int(sys.argv[2]))
    key = key / 2048
    value = value / 1000
    results[key] = value

  for k in sorted(results.keys()):
    print k, results[k]
