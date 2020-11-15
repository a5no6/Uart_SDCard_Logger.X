#!/usr/bin/python
"""
 MIT License

Copyright (c) 2020 Konomu Abe

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---------------------------------------------------------------------

Description

For the guys who care flush drive ware, this script counts write access to the SD card.
Uart_SDCard_Logger send debug data to debug uart port.
Save uart data in append mode to somefile, and run this program like python check_sd_write.py somefie.

"""

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



