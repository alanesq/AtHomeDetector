          AtHomeDetector - alanesq@disroot.org - Jan2020
          ==============================================
        
This is a project I created for a friend so that his home automation system would know if he is at home or not,
it does this by monitoring the wifi for his phone and if no data has been transmitted to or from his phone
for a certain length of time it assumes he has gone out and so switches the status of an IO pin to signify
this to his home automation (by switching a relay in this case).

I didn't expect this to prove to be very useful as I expected it to think he had gone out when he hadn't or
fail to detect him when he returns home but it has proved to be amazingly effective so I am publishing it here
in case it is of use/interest to anyone else.

It is based on a project by ricardooliveira - here: https://www.hackster.io/ricardooliveira/esp8266-friend-detector-12542e

uses a ESP8266 board with a cheap Nokia 5110 display on pins:
    #define RST D2
    #define CE D3
    #define DC D4
    #define DIN D5
    #define CLK D6
    
and signals the persons at home status on pin D1.

You need to enter the phones MAC address under " Known MAC addresses " as the top entry.
You can add other Mac addresses so that these will also be shown on the display when they transmit wifi data.
There are two dummy MAC addresses entered in this sketch as an example of the format.



