RFM12B Library
----------------
By Felix Rusu (felix@lowpowerlab.com)
<br/>
Based on the RFM12 driver from jeelabs.com (2009-02-09 <jc@wippler.nl>)
<br/>
http://opensource.org/licenses/mit-license.php

###Features:
- easy API with a few simple functions for basic usage
- 127 possible nodes on 256 possible networks
- 128 bytes max message length
- customizable transmit power (8 levels) for low-power transmission control
- customizable air-Kbps rate allows fine tuning the transmission reliability vs speed (transmitting slower is more reliable but takes more time which implies more power usage)
- Sleep/Wakeup functionality for power saving
- Low battery detector with customizable low voltage threshold
- Interrupt driven
- Support for targeted ACK instead of broadcasted ACK (possible because of the new source byte in the header)
encryption with XXTEA algorithm by David Wheeler, adapted from http://en.wikipedia.org/wiki/XXTEA
Support for these chips: ATMega8 family (ATmega168, ATMega328) ATMega2560, ATMega1280, ATMega644P, ATTiny84, ATTiny44, ATMega32u4. So far only tested on ATMega 328/P
- wireless programming (for more info click [here](http://lowpowerlab.com/blog/2013/04/18/moteino-wireless-programming-source-code/), [here](http://lowpowerlab.com/?p=643) and [here](http://lowpowerlab.com/?p=669))

###Installation
Copy the content of this library in the "Arduino/libraries/RFM12B" folder.
<br />
To find your Arduino folder go to File>Preferences in the Arduino IDE.
<br/>
See [this tutorial](http://learn.adafruit.com/arduino-tips-tricks-and-techniques/arduino-libraries) on Arduino libraries.

###Saple usage
- [Sender](https://github.com/LowPowerLab/RFM12B/blob/master/Examples/Send/Send.ino)
- [Receiver](https://github.com/LowPowerLab/RFM12B/blob/master/Examples/Receive/Receive.ino)
- More examples in the [Exameples folder](https://github.com/LowPowerLab/RFM12B/tree/master/Examples)


###TODOs (in order of priority):
- Support automatic ACK handling
- Refactor changing the SPI CS signal
- Add support for hosting multiple radios on 1 MCU
