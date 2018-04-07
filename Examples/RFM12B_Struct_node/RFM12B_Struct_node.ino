#include <RFM12B.h>
#include <SPI.h>
#include <SPIFlash.h>

#define NODEID      99
#define NETWORKID   100
#define GATEWAYID   1
#define FREQUENCY   RF12_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define KEY         "thisIsEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define LED         9
#define SERIAL_BAUD 115200
#define ACK_TIME    30  // # of ms to wait for an ack

int TRANSMITPERIOD = 600; //transmit a packet to gateway so often (in ms)
byte sendSize=0;
boolean requestACK = false;
SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
RFM12B radio;

typedef struct {		
  int           nodeId; //store this nodeId
  unsigned long uptime; //uptime in ms
  float         temp;   //temperature maybe?
} Payload;
Payload theData;

void setup() {
  Serial.begin(SERIAL_BAUD);
  radio.Initialize(NODEID, FREQUENCY, NETWORKID, 0);
  radio.Encrypt((byte*)KEY);
  
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF12_433MHZ ? 433 : FREQUENCY==RF12_868MHZ ? 868 : 915);
  Serial.println(buff);
  
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
}

long lastPeriod = -1;
void loop() {
  //process any serial input
  if (Serial.available() > 0)
  {
    char input = Serial.read();
    if (input >= 48 && input <= 57) //[0,9]
    {
      TRANSMITPERIOD = 100 * (input-48);
      if (TRANSMITPERIOD == 0) TRANSMITPERIOD = 1000;
      Serial.print("\nChanging delay to ");
      Serial.print(TRANSMITPERIOD);
      Serial.println("ms\n");
    }
    
    if (input == 'd') //d=dump flash area
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
    if (input == 'e')
    {
      Serial.print("Erasing Flash chip ... ");
      flash.chipErase();
      while(flash.busy());
      Serial.println("DONE");
    }
    if (input == 'i')
    {
      Serial.print("DeviceID: ");
      word jedecid = flash.readDeviceId();
      Serial.println(jedecid, HEX);
    }
  }

  //check for any received packets
  if (radio.ReceiveComplete())
  {
    if (radio.CRCPass())
    {
      Serial.print('[');Serial.print(radio.GetSender(), DEC);Serial.print("] ");
      for (byte i = 0; i < radio.GetDataLen(); i++)
        Serial.print((char)radio.GetData()[i]);

      if (radio.ACKRequested())
      {
        radio.SendACK();
        Serial.println(" - ACK sent.");
      }
      Blink(LED,5);
    }
  }
  
  int currPeriod = millis()/TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    //fill in the struct with new values
    theData.nodeId = NODEID;
    theData.uptime = millis();
    theData.temp = 91.23; //it's hot!
    
    Serial.print("Sending struct (");
    Serial.print(sizeof(theData));
    Serial.print(" bytes) ... ");
    
    requestACK = !requestACK; //request every other time
    
    radio.Send(GATEWAYID, (const void*)(&theData), sizeof(theData), requestACK);
    if (requestACK)
    {
      Serial.print(" - waiting for ACK...");
      if (waitForAck(GATEWAYID)) Serial.print("ok!");
      else Serial.print("nothing...");
    }

    Serial.println();
    Blink(LED,3);
    lastPeriod=currPeriod;
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

// wait a few milliseconds for proper ACK to me, return true if indeed received
static bool waitForAck(byte theNodeID) {
  long now = millis();
  do {
    if (radio.ACKReceived(theNodeID))
      return true;
  } while (millis() - now <= ACK_TIME);
  return false;
}
