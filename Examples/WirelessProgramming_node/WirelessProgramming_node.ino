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
// The sketch includes logic to receive the new sketch 'over-the-air' and store it in
// the FLASH chip, then restart the Moteino so the bootloader can continue the job of
// actually reflashing the internal flash memory from the external FLASH memory chip flash image
// The handshake protocol that receives the sketch wirelessly by means of the RFM12B radio
// is handled by the SPIFLash/WirelessHEX library, which also relies on the RFM12B library
// These libraries and custom 1k Optiboot bootloader are at: http://github.com/lowpowerlab

#include <RFM12B.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <WirelessHEX.h>

#define MYID       55       // node ID used for this unit
#define GROUPID   100
#define GATEWAYID   1
#define SERIAL_BAUD      115200
#define ACK_TIME             50  // # of ms to wait for an ack

RFM12B radio;
char input = 0;
long lastPeriod = -1;

//////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0x1F44 for adesto(ex atmel) 4mbit flash
//                             0xEF30 for windbond 4mbit flash
//////////////////////////////////////////
SPIFlash flash(8, 0x1F44);

void setup(){
  Serial.begin(SERIAL_BAUD);
  radio.Initialize(MYID, RF12_915MHZ, GROUPID);

  Serial.print("Start...");
  
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL!");
}

void loop(){
  /*
  // This part is optional.
  // Handle serial input (to allow basic DEBUGGING of FLASH chip)
  // ie: display first 256 bytes in FLASH, erase chip, write bytes at first 10 positions, etc
  if (Serial.available() > 0) {
    input = Serial.read();
    if (input == 'd') //d=dump flash area
    {
      Serial.println("Flash content:");
      int counter = 0;

      while(counter<=256){
        Serial.print(flash.readByte(counter++), HEX);
        Serial.print('.');
      }
      
      Serial.println();
    }
    else if (input == 'e')
    {
      Serial.print("Erasing Flash chip ... ");
      flash.chipErase();
      while(flash.busy());
      Serial.println("DONE");
    }
    else if (input == 'i')
    {
      Serial.print("DeviceID: ");
      Serial.println(flash.readDeviceId(), HEX);
    }
    else if (input == 'r')
    {
      Serial.print("Rebooting");
      resetUsingWatchdog(true);
    }
    else if (input >= 48 && input <= 57) //0-9
    {
      Serial.print("\nWriteByte("); Serial.print(input); Serial.print(")");
      flash.writeByte(input-48, millis()%2 ? 0xaa : 0xbb);
    }
  }
  */
  
  // Check for existing RF data, potentially for a new sketch wireless upload
  // For this to work this check has to be done often enough to be
  // picked up when a GATEWAY is trying hard to reach this node for a new sketch wireless upload
  if (radio.ReceiveComplete())
  {
    if (radio.CRCPass())
    {
      Serial.print("Got [");
      Serial.print(*radio.DataLen);
      Serial.print("] > ");
      for (byte i = 0; i < *radio.DataLen; i++)
        Serial.print((char)radio.Data[i], HEX);
      Serial.println();
      
      CheckForWirelessHEX(radio, flash, true);
    }
    else Serial.print("BAD-CRC");
    
    Serial.println();
  }
  
  ////////////////////////////////////////////////////////////////////////////////////////////
  // Real sketch code here, let's blink the onboard LED every 0.5sec
  if ((int)(millis()/500) > lastPeriod)
  {
    lastPeriod++;
    pinMode(9, OUTPUT);
    digitalWrite(9, lastPeriod%2);
  }
  ////////////////////////////////////////////////////////////////////////////////////////////
}
