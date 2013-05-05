#/*
# * Copyright (c) 2013 by Felix Rusu <felix@lowpowerlab.com>
# *
# * This file is free software; you can redistribute it and/or modify
# * it under the terms of either the GNU General Public License version 2
# * or the GNU Lesser General Public License version 2.1, both as
# * published by the Free Software Foundation.
# */

# This script will handle the transmission of a compiled sketch in the
# form of an INTEL HEX flash image to an attached gateway/master Moteino node,
# for further wireless transmission to a target Moteino node that will receive it de-HEXified and
# store it in external memory. Once received by the target (which is also loaded with a custom bootloader
# capable of reading back that image) it will reset and reprogram itself with the new sketch
#
# EXAMPLE USAGE: WirelessProgramming.py -f FILE.hex -s COM100 -b 115200

import time, sys, serial
import collections
import re

### GENERAL SETTINGS ###
SERIALPORT = "COM102"  # the default com/serial port the receiver is connected to
BAUDRATE = 115200      # default baud rate we talk to Moteino
DEBUG = False
HEX = "flash.hex"
retries = 2

# Read command line arguments
if (sys.argv and len(sys.argv) > 1):
  if len(sys.argv)==2 and sys.argv[1] == "-h":
    print " -d               Set DEBUG=True"
    print " -f               Set HEX file (Default: ", HEX, ")"
    print " -s SPort         Read from serial port SPort (Default: ", SERIALPORT, ")"
    print " -b Baud          Set serial port bit rate to Baud (Default: ", BAUDRATE, ")"
    print " -h               Print this message"
    exit(0)
    
  for i in range(len(sys.argv)):
    if sys.argv[i] == "-d":
      DEBUG = True
    if sys.argv[i] == "-s" and len(sys.argv) >= i+2:
      SERIALPORT = sys.argv[i+1]
    if sys.argv[i] == "-b" and len(sys.argv) >= i+2:
      BAUD = sys.argv[i+1]
    if sys.argv[i] == "-f" and len(sys.argv) >= i+2:
      HEX = sys.argv[i+1]

# open up the FTDI serial port to get data transmitted to Moteino
ser = serial.Serial(SERIALPORT, BAUDRATE, timeout=1) #timeout=0 means nonblocking
time.sleep(1) #wait for Moteino reset after port open

def millis():
  return int(round(time.time() * 1000)) 

def waitForHandshake(isEOF=False):
  now = millis()
  count = 0
  while True:
    if millis()-now < 5000:
      count += 1
      if isEOF:
        ser.write("FLX?EOF" + '\n')
      else: ser.write("FLX?" + '\n')
      ser.flush()
      rx = ser.readline().rstrip()
      if len(rx) > 0:
        print "Moteino: [" + rx + "]"
        if rx == "FLX?OK":
          print "HANDSHAKE OK!"
          return True
    else: return False


# return 0:timeout, 1:OK!, 2:match but out of synch
def waitForSEQ(seq):
  now = millis()
  while True:
    if millis()-now < 3000:
      rx = ser.readline()
      if len(rx) > 0:
        rx = rx.strip()
        print "Moteino: " + rx
        result = re.match("FLX:([0-9]*):OK", rx)
        if result != None:
          if int(result.group(1)) == seq:
            return 1
          else: return 2
    else: return False

    
# MAIN()
if __name__ == "__main__":
  try:
    with open(HEX) as f:
      print "File found, passing to Moteino..."
      
      if waitForHandshake():
        seq = 0
        content = f.readlines()

        while seq < len(content):
          tx = "FLX:" + str(seq) + content[seq].strip()
          isEOF = (content[seq].strip() == ":00000001FF") #this should be the last line in any valid intel HEX file
          
          if isEOF==False:
            print "TX > " + tx
            ser.write(tx + '\n')
            result = waitForSEQ(seq)
          elif waitForHandshake(True):
            print "SUCCESS!"
            break;

          if result == 1: seq+=1
          elif result == 2: continue # out of synch, retry
          else:
            if retries > 0:
              retries-=1
              print "Timeout, retry...\n"
              continue
            else:
              print "TIMEOUT, aborting..."
              break;

        while 1:
          rx = ser.readline()
          if (len(rx) > 0): print rx.strip()
      else: print "No response from Moteino, exiting..."

  except IOError:
    print "File ", HEX, " not found, exiting..."
