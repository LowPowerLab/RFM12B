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
#define ACK_TIME         100  // # of ms to wait for an ack
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
  
  if (inputLen == 1) {
    c = Serial.read();
    if (c == 'f') //f=transmit flash to node
    {
      sprintf(buffer, "Sending FLASH to NODE %d", TARGET_ID);
      Serial.println(buffer);
      HandleOutgoingFlashData();
    }
  }
  else if (inputLen == 4 && input[0]=='F' && input[1]=='L' && input[2]=='X' && input[3]=='?') {
    CheckForSerialHEX((byte*)input, inputLen, radio, TARGET_ID, TIMEOUT, ACK_TIME, true);
  }
  else if (inputLen>0) { //just echo back
    Serial.print("RECEIVED > ");Serial.println(input);
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


//handle transmission of a FLASH image
void HandleOutgoingFlashData()
{
  Serial.println();
  int seq=0;
  int tmp,len;
  long now=millis();

  while(1){
    if (seq==0)
    {
      //begin handshake
      radio.Send(TARGET_ID, "FLX?", 4, true);
      Serial.println("FLX?");
    }

    if (seq >=1 && seq<=4)
    {
      uint8_t b = sprintf(buffer, "FLX:%d:", seq);
      for(byte i=0;i<24;i++)
        buffer[b+i] = FLASH[(seq-1)*24+i];
      
      for(byte i=0;i<24+b;i++)
        Serial.print((char)buffer[i]);
      Serial.println();

      radio.Send(TARGET_ID, buffer, 24+b, true);
    }

    if (seq==-1)
    {
      radio.Send(TARGET_ID, "FLX?EOF", 7, true);
      Serial.println("FLX?EOF");
    }

    if (waitForAck(radio, ACK_TIME))
    {
      tmp = *radio.DataLen;
      for(byte i=0;i<tmp;i++)
        Serial.print((char)radio.Data[i]);
      Serial.println();
      
      if (tmp >= 4 && radio.Data[0]=='F' && radio.Data[1]=='L' && radio.Data[2]=='X')
      {
        now = millis();
        if (radio.Data[3]==':')
        {
          sscanf((const char*)radio.Data, "FLX:%d:OK", &tmp);
          if (tmp==4) seq=-1;
          else if (tmp == seq) seq++;
        }
        if (radio.Data[3]=='?' && radio.Data[4]=='O' && radio.Data[5]=='K')
        {
          if (seq==-1)
          {
            Serial.println("FINISHED!");
            break;
          }
          if (seq==0) seq++;
        }
      }
    }

    if (millis()-now > TIMEOUT)
    {
      Serial.println("Timeout, aborting FLASH operation ...");
      break; //abort FLASH sequence if no valid ACK was received for a long time
    }
  }
}
