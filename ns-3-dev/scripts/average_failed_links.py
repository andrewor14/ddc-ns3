#!/usr/bin/env python

import sys
from average import *

def process(line):
  if "### DATA ###" in line and "failed links" in line:
    return line.split("+++")[1]
  else:
    return ""

def averageFailedLinks(path):
  f = open(path)
  lines = f.readlines()
  f.close()
  return average(lines, process)

if __name__ == "__main__":
  if len(sys.argv) < 2:
    print "Usage: average_failed_links.py [filename]"
    sys.exit(1)
  print averageFailedLinks(sys.argv[1])
  
