/*
 * Copyright (c) 2013 by Felix Rusu <felix@lowpowerlab.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */
 
// This is a NODE sketch (LEDControl_Node) for a Moteino that can act as
// a endpoint which blinks/pulses/stops the onboard LEDs per requests from Moteino(s)
// loaded with GATEWAY sketches (LEDControl_Gateway)
// Designed by Felix Rusu (felix@lowpowerlab.com), www.LowPowerLab.com

// The LEDs on the endpoint Moteinos can be controlled as follows. In the GATEWAY's serial terminal:
// Type a node ID in the rage [2-9] to set the target node
// Type a LED mode [b,p,o] - blink, pulse, off
// Type a speed for the LED action [s,m,f] - slow, medium, fast
// Hit [ENTER] to send request to target
// The target Moteino receives the message and applies the requested LED mode and speed

#include <RFM12B.h>
#include <avr\sleep.h>
#include <avr\delay.h>
#include <LowPower.h>

#define NETWORKID         100  //what network this node is on
#define NODEID              2  //this node's ID, should be unique among nodes on this NETWORKID
#define GATEWAYID           1  //central node to report data to
#define KEY "ABCDABCDABCDABCD" //(16 bytes of your choice - keep the same on all encrypted nodes)
#define LED                 9  //pin connected to onboard LED (digital 9, pulse width modulation capable for pulsing)
#define SERIAL_BAUD    115200

RFM12B radio;
char mode = 'b';      // mode : 'b' = blinking, 'p' = pulsing, 'o' = 'off'
char theSpeed = 'm';  // speed : 's' = slow, 'm' = medium, 'f' = 'fast'
boolean LEDState;     // current LED state when blinking
long now=0, LED_lastStateChange=0;
float in, out;        // used for fading the LED

void setup(void)
{
  radio.Initialize(NODEID, RF12_915MHZ, NETWORKID);
  radio.Encrypt((uint8_t*)KEY);
  Serial.begin(SERIAL_BAUD);
  pinMode(LED, OUTPUT);
  Serial.println("Listening for LED blink/pulse/stop requests...\n");
  handleRequest(mode, theSpeed);
  Serial.println();
}

void loop()
{
  now = millis();
  
  //handle blinking/fading/stopping
  if (mode=='p')
  {
    //do some fading for 50ms & continue next loop where fading left off (to give radio a chance to listen)
    while(millis()-now < 50)
    {
      if (in > 6.283) in = 0;
      in += .00628;
      out = sin(in) * 127.5 + 127.5;
      analogWrite(LED, out);
      delayMicroseconds(theSpeed=='s'?1500:theSpeed=='m'?1000:500);
    }
  }
  else if (mode=='b' && millis()-LED_lastStateChange >= (theSpeed=='s'?1000:theSpeed=='m'?500:100))
  {
    LEDState = !LEDState;
    digitalWrite(LED, LEDState);
    LED_lastStateChange = now;
  }
  else if (mode == 'o')//not 'b'linking or 'p'ulsing so turn LED off
    digitalWrite(LED, 0);
  
  //pass any received RF message to serial
  if (radio.ReceiveComplete())
  {
    if (radio.CRCPass())
    {
      Serial.print('[');Serial.print(radio.GetSender(), DEC);Serial.print("] ");
      for (byte i = 0; i < *radio.DataLen; i++)
        Serial.print((char)radio.Data[i]);

      Serial.print(" - ");
      //check for a LED blink/pulse/stop request
      if (*radio.DataLen == 3
          && (radio.Data[0]=='b' || radio.Data[0]=='p' || radio.Data[0]=='o')
          &&  radio.Data[1] == ':'
          && (radio.Data[2]=='s' || radio.Data[2]=='m' || radio.Data[2]=='f'))
        handleRequest(radio.Data[0], radio.Data[2]);

      if (radio.ACKRequested())
      {
        radio.SendACK();
        Serial.print(" (ACK sent)");
      }
    }
    else Serial.print("BAD-CRC");
    
    Serial.println();
  }
}

void handleRequest(char requestMode, char requestSpeed)
{
  Serial.print(requestMode=='b'?"BLINK ":requestMode=='p'?"PULSE ":"OFF");
  if (requestMode != 'o')
    Serial.print(requestSpeed=='s'?"SLOW":requestSpeed=='m'?"MEDIUM":"FAST");
  mode = requestMode;
  theSpeed = requestSpeed;
}