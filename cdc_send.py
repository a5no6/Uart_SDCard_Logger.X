#!/usr/bin/env python3
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

Test program for Uart_SDCard_Logger.
Send some data to USB uart.

"""

import serial
import sys
import os
import time
import re
import array
import argparse
import logging

if os.name == 'posix' : 
    SerailPortScanName = ['/dev/ttyUSB0','/dev/ttyACM0','/dev/ttyUSB1','/dev/ttyACM1']
else:
    SerailPortScanName = []
    for i in range(4,30):  
        SerailPortScanName.append('COM%d'%i)
    
def OpenPort(COM_port=None,baudrate=38400):
    if COM_port == None:
        scan_port = SerailPortScanName
    else:
        scan_port = [COM_port]
    ser = None
    for port in scan_port:
        try:
            ser = serial.Serial(port,baudrate,timeout=1)
            print("Found port %s. Opened with %d bps."%(port,baudrate))
            break
        except serial.serialutil.SerialException:
            pass
    return(ser)


if __name__ == '__main__':
    logging.basicConfig(filename='cdc_send.log', level=logging.DEBUG,format='%(asctime)s %(message)s')
    logger = logging.getLogger('cdcsendlogging')
    logger.addHandler(logging.StreamHandler())
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-b','--uart_baud', help='38400 or you like')
    args = parser.parse_args()

        
    if args.uart_baud :
        baud = int(args.uart_baud)
    else:
        baud = int(38400)
        
    ser=OpenPort(baudrate = baud)

    if ser:
          c = 0
          sleepsec = 1
          while True:
             for i in range(100):
               c+=1
               data = "%d Hello This is test .\n"%c
               print(data)
               c+=1
               ser.write(data.encode())
               time.sleep(sleepsec)
               data = "%d This is test .\n"%c
               print(data)
               c+=1
               ser.write(data.encode())
               time.sleep(sleepsec)
               data = "%d Are you aweke .\n"%c
               print(data)
               ser.write(data.encode())
               time.sleep(sleepsec)
               time.sleep(30)
    else:
        print("No port found.")

