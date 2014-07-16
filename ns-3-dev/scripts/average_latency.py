#!/usr/bin/env python

import sys
from average import *

def process(line):
  try:
    if "### DATA ###" in line and "latency" in line:
      return long(line.split("+++")[1])
  except:
    pass
  return ""

def averageLatency(path, p):
  f = open(path)
  lines = f.readlines()
  f.close()
  return percentile(lines, process, p)

if __name__ == "__main__":
  if len(sys.argv) < 2:
    print "Usage: average_latency.py [filename]"
    sys.exit(1)
  print averageLatency(sys.argv[1], 50)
  
