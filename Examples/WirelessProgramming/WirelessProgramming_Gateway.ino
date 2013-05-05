/*
 * Copyright (c) 2013 by Felix Rusu <felix@lowpowerlab.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

// This sketch is an example of how wireless programming can be achieved with a Moteino
// that was loaded with a custom 1k Optiboot that is capable of loading a new sketch from
// an external SPI flash chip
// This is the GATEWAY node, it does not need a custom Optiboot nor any external FLASH memory chip
// (ONLY the target node will need those)
// The sketch includes logic to receive the new sketch from the serial port (from a host computer) and 
// transmit it wirelessly to the target node
// The handshake protocol that receives the sketch from the serial port 
// is handled by the SPIFLash/WirelessHEX library, which also relies on the RFM12B library
// These libraries and custom 1k Optiboot bootloader for the target node are at: http://github.com/lowpowerlab

#include <RFM12B.h>
#include <SPI.h>
#include <WirelessHEX.h>

#define MYID               1       // node ID used for this unit
#define TARGET_ID         55      //ID of node being wirelessly reprogrammed
#define GROUPID          100
#define SERIAL_BAUD   115200
#define ACK_TIME          20  // # of ms to wait for an ack
#define TIMEOUT         3000

RFM12B radio;
char c = 0;
char FLASH[97] = ":1234567890ABCDEFGHIJKLM:ABCDEFGHIJKLM1234567890:1234567890AAAAAABBBBBBB:CCCCCCDDDDDDD1234567890";
char buffer[256];
char input[64]; //serial input buffer

void setup(){
  Serial.begin(SERIAL_BAUD);
  radio.Initialize(MYID, RF12_915MHZ, GROUPID);
  Serial.print("Start...");
}

void loop(){
  byte inputLen = readSerialLine(input);
  
  if (inputLen == 4 && input[0]=='F' && input[1]=='L' && input[2]=='X' && input[3]=='?') {
    CheckForSerialHEX((byte*)input, inputLen, radio, TARGET_ID, TIMEOUT, ACK_TIME, false);
  }
  else if (inputLen>0) { //just echo back
    Serial.print("SERIAL IN > ");Serial.println(input);
    //Serial.print(", ");for (byte x =0;x<inputLen;x++) Serial.print(input[x], HEX);
  }
  
  if (radio.ReceiveComplete())
  {
    if (radio.CRCPass())
    {
      for (byte i = 0; i < *radio.DataLen; i++)
        Serial.print((char)radio.Data[i]);
      
      if (radio.ACKRequested())
      {
        radio.SendACK();
        Serial.print(" - ACK sent");
      }
    }
    else Serial.print("BAD-CRC");
    
    Serial.println();
  }
}
