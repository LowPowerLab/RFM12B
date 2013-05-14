// Test sketch that is loaded on slave Moteinos
// It will send an encrypted message to the master/gateway every TRANSMITPERIOD
// It will respond to any ACKed messages from the master
// Every 3rd message will also be ACKed (request ACK from master).
#include <RFM12B.h>
#include <SPIFlash.h>
#include <SPI.h>b
#include <avr/sleep.h>

#define MYID        123       // node ID used for this unit
#define NETWORKID   100
#define GATEWAYID     1
#define FREQUENCY  RF12_433MHZ //Match this with the version of your Moteino! (others: RF12_433MHZ, RF12_915MHZ)
#define KEY  "ABCDABCDABCDABCD"
int TRANSMITPERIOD = 500; //transmit a packet to gateway so often (in ms)

#define SERIAL_BAUD      115200
#define ACK_TIME             50  // # of ms to wait for an ack

char input = 0;
RFM12B radio;
SPIFlash flash(8, 0xEF30); //EF30 for Windbond 4mbit flash
boolean requestACK = false;
byte sendSize=0;
char payload[] = "123 ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void setup()
{
  Serial.begin(SERIAL_BAUD);
  radio.Initialize(MYID, FREQUENCY, NETWORKID, 0);
  radio.Encrypt((byte*)KEY);
  char buff[50];
  sprintf(buff, "Transmitting at %d Mhz...", FREQUENCY == RF12_433MHZ ? 433 : FREQUENCY== RF12_868MHZ ? 868 : 915);
  Serial.println(buff);
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
}

long lastPeriod = -1;
void loop()
{
  //process any serial input
  if (Serial.available() > 0) {
    input = Serial.read();
    if (input >= 48 && input <= 57) //[0,9]
    {
      TRANSMITPERIOD = 100 * (input-48);
      if (TRANSMITPERIOD == 0) TRANSMITPERIOD = 1000;
      Serial.print("\nChanging delay to ");
      Serial.print(TRANSMITPERIOD);
      Serial.println("ms\n");
    }
    else if (input == 'd') //d=dump flash area
    {
      Serial.println("Flash content:");
      int counter = 0;

      while(counter<=256){
        Serial.print(flash.readByte(counter++), HEX);
        Serial.print('.');
      }
      while(flash.busy());      
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
  }

  //check for any received packets
  if (radio.ReceiveComplete())
  {
    if (radio.CRCPass())
    {
      Serial.print('[');Serial.print(radio.GetSender(), DEC);Serial.print("] ");
      for (byte i = 0; i < *radio.DataLen; i++)
        Serial.print((char)radio.Data[i]);

      if (radio.ACKRequested())
      {
        radio.SendACK();
        Serial.print(" - ACK sent.");
      }
      Blink(9,5);
    }
  }
  
  int currPeriod = millis()/TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    lastPeriod=currPeriod;
    //Send data periodically to GATEWAY
    Serial.print("Sending[");
    Serial.print(sendSize);
    Serial.print("]: ");
    for(byte i = 0; i < sendSize; i++)
      Serial.print((char)payload[i]);
    
    requestACK = ((sendSize % 3) == 0); //request ACK every 3rd xmission
    radio.Send(GATEWAYID, payload, sendSize, requestACK);
    if (requestACK)
    {
      Serial.print(" - waiting for ACK...");
      if (waitForAck(GATEWAYID)) Serial.print("ok!");
      else Serial.print("nothing...");
    }
    
    sendSize = (sendSize + 1) % 31;
    Serial.println();
    Blink(9,5);
  }
  //else { Serial.print("currPeriod = "); Serial.print(currPeriod); Serial.print("   lastPeriod = "); Serial.println(lastPeriod); }
}

// wait a few milliseconds for proper ACK to me, return true if indeed received
static bool waitForAck(byte theNodeID) {
  long now = millis();
  while (millis() - now <= ACK_TIME) {
    if (radio.ACKReceived(theNodeID))
      return true;
  }
  return false;
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
