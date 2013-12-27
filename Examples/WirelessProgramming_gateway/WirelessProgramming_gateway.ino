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
#include <SPIFlash.h>
#include <WirelessHEX.h>

#define NETWORKID          100  //what network this node is on
#define NODEID               5  //this node's ID, should be unique among nodes on this NETWORKID
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY   RF12_433MHZ
//#define FREQUENCY   RF12_868MHZ
#define FREQUENCY   RF12_915MHZ

#define SERIAL_BAUD 115200
#define ACK_TIME    20  // # of ms to wait for an ack
#define TIMEOUT     3000
#define LED         9

RFM12B radio;
char c = 0;
char FLASH[97] = ":1234567890ABCDEFGHIJKLM:ABCDEFGHIJKLM1234567890:1234567890AAAAAABBBBBBB:CCCCCCDDDDDDD1234567890";
char input[64]; //serial input buffer
byte targetID=0;

void setup(){
  Serial.begin(SERIAL_BAUD);
  radio.Initialize(NODEID, FREQUENCY, NETWORKID);
  Serial.print("Start...");
}

void loop(){
  byte inputLen = readSerialLine(input, 10, 64, 100); //readSerialLine(char* input, char endOfLineChar=10, byte maxLength=64, uint16_t timeout=1000);
  
  if (inputLen == 4 && input[0]=='F' && input[1]=='L' && input[2]=='X' && input[3]=='?') {
    if (targetID==0)
      Serial.println("TO?");
    else
    CheckForSerialHEX((byte*)input, inputLen, radio, targetID, TIMEOUT, ACK_TIME, false);
  }
  else if (inputLen>3 && inputLen<=6 && input[0]=='T' && input[1]=='O' && input[2]==':')
  {
    byte newTarget=0;
    for (byte i = 3; i<inputLen; i++) //up to 3 characters for target ID
      if (input[i] >=48 && input[i]<=57)
        newTarget = newTarget*10+input[i]-48;
      else
      {
        newTarget=0;
        break;
      }
    if (newTarget>0)
    {
      targetID = newTarget;
      Serial.print("TO:");
      Serial.print(newTarget);
      Serial.println(":OK");
    }
    else
    {
      Serial.print(input);
      Serial.print(":INV");
    }
  }
  else if (inputLen>0) { //just echo back
    Serial.print("SERIAL IN > ");Serial.println(input);
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
  Blink(LED,5); //heartbeat
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}