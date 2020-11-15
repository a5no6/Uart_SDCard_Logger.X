#!/usr/bin/python3

import serial
import sys
import os
import time
import re
import array
import argparse
import logging

if os.name == 'posix' : # assume ubuntu
    SerailPortScanName = ['/dev/ttyUSB0','/dev/ttyACM0','/dev/ttyUSB1','/dev/ttyACM1']
else:
#    SerailPortScanName = ['COM14','COM13','COM12','COM11','COM10','COM9','COM8','COM7','COM6','COM5','COM4','COM3','COM2','COM1','COM0']
    SerailPortScanName = []
#    for i in range(30):
    for i in range(4,30):   ## Gateway machine have unknown PORT 3. have to skip it
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

def SerialSend(ser,data):
    if ser == None:
        return(False)

    if isinstance(data,str):
        print('Send "%s"'%(data))
        data = data+'\n'
        command = data.encode()
    else:
        command = data

    if len(command)>0:
        ser.write(command)


def InteractiveUartSend(ser):
    while True:
        s = input('input to send. ')
        data = s+'\n'
        data = data.encode()
        ser.write(data)


def RecvEEPROM(ser):
    total_recv = 0
    data = b''
    next_print_len = 1024
    while True:
        reply = ser.read(64)
        if reply==b'':
            break
        total_recv += len(reply)
        data += reply
        if total_recv >= next_print_len:
            print('recieved %05x'%(total_recv))
            next_print_len += 1024
        disp = ''
        for i in range(len(reply)):
            disp += '%02x'%reply[i]
    return(data)

def RecvUART(ser):
    c = 0
    last_print = 0
    recv_byte = 0
    lines_read = 0
    if args.file:
        f = open(args.file,'ab')
    else:
        f = None
    reply = b''
    init_time = time.time()
    while 1:
        try:
            if args.uart_disp == "ascii":
    #          recv = ser.readline() # 1line , Freeze if \n does not come
                recv = ser.readline(1024)
            else:
                recv = ser.read(256)
             #   recv = ser.read() # 1byte only
            if len(recv)==0:
                continue
            lines_read += 1
        except :
            break
        if args.file and len(recv)>0:
            f.write(recv)
        if recv==b'':
            c += 1
            if c> 1000:
                break

        recv_byte += len(recv)
        data_kb = int(recv_byte/1024)

        if args.uart_disp=="hex":
            disp = ''
            for i in range(len(recv)):
                disp += '%02x'%recv[i]
            print(disp)
        elif args.uart_disp=="ascii":
            try:
                print(recv[:-1].decode())
            except UnicodeDecodeError:
                pass
        else:
            if data_kb>=last_print+1:
                print("%dKB %dbps"%(data_kb,recv_byte*10/(time.time()-init_time)))
                last_print = data_kb
                
#        time.sleep(0.005) just 5ms will cause large delay  
    if args.file:
        f.close()
    ser.close()

def upload_file(ser,filename):
    f = open(filename)
    ls = f.read().split('\n')
    f.close()
    while '' in ls:
        ls.remove('')
    
    b = [0]*int(args.eeprom_length_hex,16)
    for i in range(len(ls)):
        if i>=len(b):
            break
        b[i] = int(ls[i])
    eepromdata = array.array("B")
    eepromdata.fromlist(b)

    msg = 'write %s %s'%(args.eeprom_address_hex,args.eeprom_length_hex)
    print(msg)
    ser.write(msg.encode());
    for i in range(0,int(args.eeprom_length_hex,16),64):
        ser.write(eepromdata[i:i+64])
        reply = ''
        while 'OK' not in reply:
            reply = ser.readline(64).decode();
            time.sleep(0.1)
            logger.debug(reply)
    reply = ''
    while 'EEPROM Write Done' not in reply:
        reply = ser.readline(64).decode()
        time.sleep(0.1)
        logger.debug(reply)
    while len(ser.readline(64).decode())>0:
        time.sleep(0.1)
    SerialSend(ser,'read %s %s'%(args.eeprom_address_hex,args.eeprom_length_hex))
    d1 = RecvEEPROM(ser)
    if len(d1) != len(b):
        print("Verfy error. Size not match")
        print("%d %d"%(len(d1),len(b)))
    else:
        for i in range(len(b)):
            if b[i] != d1[i]:
                print("Verify error. Data not the same at %d"%i)
                break
        if i==len(b)-1:
            print("Verify complete")                
    return(eepromdata.tobytes())

def read_out(ser):
    print('Make sure to connect device with EEPROM before plug into USB.')
    input('read. hit any key')
    print('1st read')
    SerialSend(ser,'read %s %s'%(args.eeprom_address_hex,args.eeprom_length_hex))
    d1 = RecvEEPROM(ser)
    print('2nd read')
    SerialSend(ser,'read %s %s'%(args.eeprom_address_hex,args.eeprom_length_hex))
    d2 = RecvEEPROM(ser)
    if not d1==d2:
        print('Error. Retrieved data did not match between twice read.')
    else:
        print('Data check OK.')

    file_num = 0;
    while True:
        fname = 'eeprom_dat_%02d.txt'%file_num
        file_num += 1
        try:
            os.stat(fname)
        except FileNotFoundError:
            break
    f = open(fname,'wt')
    for i in range(0,len(d2)):
        f.write('%d\n'%(d2[i]))
    f.close()

def eeprom_erase(ser):
    input('To erase,hit any key')
    erase_data = int(args.value_hex,16)
    SerialSend(ser,'erase %s %s %s'%(args.eeprom_address_hex,args.eeprom_length_hex,args.value_hex))
    reply = ''
    while 'EEPROM Set Done' not in reply:
        reply = ser.readline(64).decode()
        time.sleep(0.1)
        logger.debug(reply)
    SerialSend(ser,'read %s %s'%(args.eeprom_address_hex,args.eeprom_length_hex))
    d2=RecvEEPROM(ser)
    for d in d2:
        if d != erase_data:
            print('Data wrong %x'%d)
    print('Check done')

if __name__ == '__main__':
    logging.basicConfig(filename='cdc_tool.log', level=logging.DEBUG,format='%(asctime)s %(message)s')
    logger = logging.getLogger('cdctoollogging')
    logger.addHandler(logging.StreamHandler())
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('-m','--mode', help='uart_recv|uart_send|eeprom_read|eeprom_write|eeprom_erase')
    parser.add_argument('-f','--file', help='uart log dump file|eeprom upload file')
    parser.add_argument('-v','--value_hex', help='value to erase eeprom')
    parser.add_argument('-d','--uart_disp', help='ascii|hex|num')
    parser.add_argument('-b','--uart_baud', help='38400 or you like')
    parser.add_argument('-a','--eeprom_address_hex', help='eeprom start address. Microchip 1024kbit uses 0x40000 for upper 64KBytes.')
    parser.add_argument('-l','--eeprom_length_hex', help='eeprom access length')
    args = parser.parse_args()

    if False:
        args.mode = "uart_send"
        args.eeprom_address_hex = "0"
        args.value_hex = "5"
        args.eeprom_length_hex = "20000"
        args.file = "cat4.txt"
        args.uart_baud = "38400"
        args.uart_disp = "num"
        
    if not args.mode:
        print("mode not specified. set uart receive mode with baudrate 38400")
        args.mode = "uart_recv"
        args.uart_baud = "38400"
        args.uart_disp = "ascii"

    if args.uart_baud :
        baud = int(args.uart_baud)
    else:
        baud = int(38400)
        
#    ser=OpenPort(baudrate = (57600*2))
#    ser=OpenPort(baudrate = 57600)
    ser=OpenPort(baudrate = 38400)
#    ser=OpenPort(baudrate = 9600)
#    ser=OpenPort(baudrate = 4800)
#    ser=OpenPort(baudrate = 2400)
    if 0:
        if args.mode == "eeprom_read":
            read_out(ser)
        elif args.mode == "eeprom_write":
            upload_file(ser,args.file)
        elif args.mode == "eeprom_erase":
            eeprom_erase(ser)
        elif args.mode == "uart_recv":
#            SerialSend(ser,"uart %s"%args.uart_baud)
#            SerialSend(ser,"cdc_rx_speed_test")
            RecvUART(ser)
        elif args.mode == "uart_send":
            InteractiveUartSend(ser)

    if ser:
          c = 0;
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

