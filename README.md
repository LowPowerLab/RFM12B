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
- 66 bytes max message length
- customizable transmit power (8 levels) for low-power transmission control
- customizable air-Kbps rate
- Low battery detector with customizable low voltage threshold
- Interrupt driven
- encryption with XXTEA algorithm by David Wheeler, adapted from http://en.wikipedia.org/wiki/XXTEA

###Installation
Copy the content of this library in the "Arduino/libraries/RFM12B" folder.
<br />
To find your Arduino folder go to File>Preferences in the Arduino IDE.
<br/>
See [this tutorial](http://www.ladyada.net/library/arduino/libraries.html) on Arduino libraries.

###Saple usage
- [Sender](https://github.com/LowPowerLab/RFM12B/blob/master/Examples/Send/Send.ino)
- [Receiver](https://github.com/LowPowerLab/RFM12B/blob/master/Examples/Receive/Receive.ino)


###TODOs:
- Add support for hosting multiple radios on 1 MCU
