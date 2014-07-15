#!/usr/bin/env python

import sys

def computeAverage(filename, verbose = False):
  '''
    Compute the average of all the numbers (representing control
    latency) in the given file, one number per line.
  '''
  numLines = 0
  total = 0
  f = open(filename)
  for line in f.readlines():
    line = line.strip()
    line = long(line)
    numLines += 1
    total += line
  average = 0
  if numLines > 0:
    average = long((float(total) / float(numLines)))
  if verbose:
    print "Results for %s:" % filename
    print "  # entries = %s" % numLines
    print "  total = %s" % total
    print "  average = %s " % average
  return average 

if __name__ == "__main__":
  if len(sys.argv) < 2:
    print "Usage: average-latency.py [filename] [verbose = False]"
    sys.exit(1)
  verbose = len(sys.argv) > 2 and (sys.argv[2].lower() == "true")
  print computeAverage(sys.argv[1], verbose)
