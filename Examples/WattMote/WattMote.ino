#include <RFM12B.h>
#include <avr\sleep.h>
#include <avr\delay.h>
#include <LowPower.h> 	         //Writeup: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/
				 //Get library here: https://github.com/rocketscream/Low-Power

#define NETWORKID            99  //what network this node is on
#define NODEID                2  //this node's ID, should be unique among nodes on this NETWORKID
#define GATEWAYID             1  //central node to report data to
#define KEY                   "XXXXXXXXXXXXXXXX" //(16 bytes of your choice - keep the same on all encrypted nodes)
#define MAINS_SAMPLES         28  //take this many samples
#define FASTADC             true  //enable faster ADC sampling
#define HALFCYCLEDELAY  SLEEP_1S  //this will cause transmissions every 2s
//#define TMP36				   2  //uncomment this if you have a TMP36 sensor (value is analog pin it's connected to)
//#define CELSIUS                 //uncomment this line to transmit in Celsius instead of Fahrenheit
//#define SERIAL_EN               //uncomment this line to enable serial IO (when you debug Moteino and need serial output)
#define SERIAL_BAUD       115200
#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input);}
  #define DEBUGln(input) {Serial.println(input);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif

RFM12B radio;
void setup(void)
{
  #if FASTADC
    // set ADC prescale to 64 for faster analog readings (http://arduino.cc/forum/index.php?topic=6549.msg51573#msg51573)
    bitSet(ADCSRA,ADPS2) ;
    bitSet(ADCSRA,ADPS1) ;
    bitClear(ADCSRA,ADPS0) ;
  #endif

  // ASAP go to sleep for a while to give KAW a chance to boot up to a normal state
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_ON);
  radio.Initialize(NODEID, RF12_433MHZ, NETWORKID);
  radio.Encrypt((uint8_t*)KEY);
  radio.Sleep();
  #ifdef SERIAL_EN
    Serial.begin(SERIAL_BAUD);
    DEBUGln("\nTransmitting...");
  #endif
}

char sendBuf[RF12_MAXDATA];
byte sendLen;
byte temperatureCounter = 0;
float temperature = 0;

void loop(void)
{
  uint16_t SAMPLES_V[MAINS_SAMPLES];
  uint16_t SAMPLES_A[MAINS_SAMPLES];
  String tempstr;
  
  //read about 2 cycles to compute VAVG (DC BIAS)
  int maxv=0, minv=1024;
  int volts;
  boolean foundmax=false, foundmin=false;
  long startTime = millis();
  while(millis()-startTime <50){
    volts=analogRead(0);
    if (volts > maxv) maxv=volts;
    if (volts < minv) minv=volts;
  }
  int vavg = (maxv+minv)/2;
  
  //seek next VAVG for sampling starting point
  //this ensures the waveform is always symmetric and starts sampling very close to "0"
  while(abs(vavg-analogRead(0))>2);
  
  //sample A0(volts) and A1(amps) FROM KAW
  for(byte i=0; i<MAINS_SAMPLES; i++)
  {
    SAMPLES_V[i]=analogRead(0);
    SAMPLES_A[i]=analogRead(1);
    _delay_us(FASTADC ? 500 : 400); //these numbers are tuned to yield 28 samples in 1 complete cycle
  }
  ADCSRA &= ~(1 << ADEN); //done sampling, turn off ADC

  //Transmit VOLTS
  tempstr = String("KAW");
  tempstr += NODEID;
  tempstr += "_V";
  tempstr += ':';
  for(byte i=0; i<MAINS_SAMPLES; i++)
  {
    //base 32 encoding (59 positions shifted to the right to avoid [ and : characters)
    tempstr += (char)((SAMPLES_V[i] / 32) + 59);
    tempstr += (char)((SAMPLES_V[i] % 32) + 59);
  }
  tempstr.toCharArray(sendBuf, RF12_MAXDATA);
  sendLen = tempstr.length();
  DEBUG(tempstr);
  radio.Wakeup();
  radio.Send(GATEWAYID, sendBuf, sendLen);
  radio.Sleep();
  
  //Transmit AMPS
  tempstr= String("KAW");
  tempstr += NODEID;
  tempstr += "_A";
  tempstr += ':';
  for(byte i=0; i<MAINS_SAMPLES; i++)
  {
    //base 32 encoding (59 positions shifted to the right to avoid [ and : characters)
    tempstr += (char)((SAMPLES_A[i] / 32) + 59);
    tempstr += (char)((SAMPLES_A[i] % 32) + 59);
  }
  tempstr.toCharArray(sendBuf, RF12_MAXDATA);
  sendLen = tempstr.length();
  DEBUG(tempstr);
  radio.Wakeup();
  radio.Send(GATEWAYID, sendBuf, sendLen);
  radio.Sleep();

  DEBUGln();
  
  //SLEEP for half transmit cycle, then transmit temperature, then BLINK, then go back to sleep for another half transmit cycle
  LowPower.powerDown(HALFCYCLEDELAY, ADC_OFF, BOD_ON);

  #ifdef TMP36
  //report temperature every 30 transmit cycles
  temperature += getTemperature();
  if (++temperatureCounter%30==0)
  {
    temperature /= 30;
    int whole = temperature;
    int decimal = ((int)(temperature * 100)) % 100;
    tempstr=String("KAW");
    tempstr += NODEID;
    tempstr += "_T";
    tempstr += ':';
    tempstr += whole;
    tempstr += '.';
    tempstr += decimal;
    tempstr.toCharArray(sendBuf, RF12_MAXDATA);
    sendLen = tempstr.length();
    DEBUG(tempstr);
    radio.Wakeup();
    radio.Send(GATEWAYID, sendBuf, sendLen);
    radio.Sleep();
    temperatureCounter = temperature = 0;
  }
  #endif
  
  Blink(9, 3); //Blink half cycle later (to offset LED current drain)
  LowPower.powerDown(HALFCYCLEDELAY, ADC_OFF, BOD_ON);
}

void Blink(byte PIN, byte DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

#ifdef TMP36
float getTemperature()
{
  int reading = analogRead(TMP36);
  // converting TMP36 reading to voltage, for 3.3v use ~3.3
  float voltage = reading * 3.297;
  voltage /= 1024.0;
  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
												//to degrees ((volatge - 500mV) times 100)
  #ifdef CELSIUS
	return temperatureC;
  #else
	float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
	return temperatureF;
  #endif
}
#endif
