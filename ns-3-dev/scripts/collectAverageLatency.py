#!/usr/bin/env python

from averageLatency import *
import sys
import os
import re

if __name__ == "__main__":
  if len(sys.argv) < 2:
    print "Usage: collect-average-latency.py [base dir] [verbose = False]"
    sys.exit(1)
  verbose = len(sys.argv) > 2 and (sys.argv[2].lower() == "true")
  baseDir = os.path.abspath(sys.argv[1])
  
  # Return (link fail percent, average control latency) pairs
  results = []
  for f in sorted(os.listdir(baseDir)):
    key = re.compile("-.*%").search(f).group()
    if key is None:
      continue
    key = key.strip("-").strip("%")
    key = float(key) / 100
    f = os.path.join(baseDir, f)
    value = computeAverage(f + "/all.log", verbose)
    results.append("%s %s" % (key, value))

  for result in results:
    print result
