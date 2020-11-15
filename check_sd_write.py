#!/usr/bin/python

import sys
import os
import time
import re
import array
import argparse
import logging

lines = []

files = os.listdir('.')
for file in files:
  match = re.search('uartlog\d*.txt',file)
#  match = re.search('uartlog.txt',file)
  if match:
    print(file)
    f = open(match.group(0))
    lines += f.read().split('\n')
    f.close()

sector = {}

for l in lines:
  match = re.search('write ([0-9A-F]+)',l)
  if match:
    address = match.group(1)
#    print(address)
    if address not in sector.keys():
        sector[address]=1
    else:
        sector[address]+=1

#address = sector.keys()
#address = sorted(sector.keys())
#address = sorted(sector.keys(), key=lambda x:x[1])
#for a in address:
#  if sector[a]>1:
#    print("%s %d"%(a,sector[a]))

for k, v in sorted(sector.items(), key=lambda x: x[1]):
  if v>1:
    print(str(k) + ": " + str(v))



