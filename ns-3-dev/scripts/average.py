#!/usr/bin/env python

import sys

def average(lines, processLine):
  '''
    Compute the average of all the numbers (representing control
    latency) in the given file, one number per line.
  '''
  numLines = 0
  total = 0
  for line in lines:
    line = line.strip()
    line = processLine(line)
    if not line or line == "":
      continue
    line = long(line)
    numLines += 1
    total += line
  average = 0
  if numLines > 0:
    average = float(total) / float(numLines)
  return average 

