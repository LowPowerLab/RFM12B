#include <RFM12B.h>
#include <SPI.h>
#include <SPIFlash.h>

#define NODEID      1
#define NETWORKID   100
#define FREQUENCY   RF12_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define KEY         "thisIsEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define LED         9
#define SERIAL_BAUD 115200
#define ACK_TIME    30  // # of ms to wait for an ack

RFM12B radio;
SPIFlash flash(8, 0xEF30); //EF40 for 16mbit windbond chip
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network

typedef struct {		
  int           nodeId; //store this nodeId
  unsigned long uptime; //uptime in ms
  float         temp;   //temperature maybe?
} Payload;
Payload theData;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.Initialize(NODEID, FREQUENCY, NETWORKID);
  radio.Encrypt((byte*)KEY);
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF12_433MHZ ? 433 : FREQUENCY==RF12_868MHZ ? 868 : 915);
  Serial.println(buff);
  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");
}

byte ackCount=0;
void loop() {
  //process any serial input
  if (Serial.available() > 0)
  {
    char input = Serial.read();
    
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

  if (radio.ReceiveComplete())
  {
    if (radio.CRCPass())
    {
      Serial.print('[');Serial.print(radio.GetSender(), DEC);Serial.print("] ");

      if (radio.GetDataLen() != sizeof(Payload))
        Serial.print("Invalid payload received, not matching Payload struct!");
      else
      {
        theData = *(Payload*)radio.GetData(); //assume radio.DATA actually contains our struct and not something else
        Serial.print(" nodeId=");
        Serial.print(theData.nodeId);
        Serial.print(" uptime=");
        Serial.print(theData.uptime);
        Serial.print(" temp=");
        Serial.print(theData.temp);
      }
      
      if (radio.ACKRequested())
      {
        byte theNodeID = radio.GetSender();
        radio.SendACK();
        //when a node requests an ACK, respond to the ACK and also send a packet requesting an ACK
        //This way both TX/RX NODE functions are tested on 1 end at the GATEWAY
        Serial.print(" - ACK sent. Sending packet to node ");
        Serial.print(theNodeID);
        delay(10);
        radio.Send(theNodeID, "ACK TEST", 8, true);
        Serial.print(" - waiting for ACK...");
        if (waitForAck(theNodeID)) Serial.print("ok!");
        else Serial.print("nothing...");
      }
      Serial.println();
      Blink(LED,3);
    }
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
