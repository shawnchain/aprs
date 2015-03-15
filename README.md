# TinyAPRS - A cheap APRS TNC for HAMs #



### What is TinyAPRS ###


TinyAPRS is my experiment on APRS after became a HAM in middle of 2014.

Like most of the APRS TNCs, TinyAPRS is an AFSK1200 modem that decodes radio signals into AX25 data packets. TinyAPRS also supports KISS protocol, so you can use it with most of the APRS applications.

Unlike most the APRS TNCs, tinyAPRS is VERY VERY CHEAP. It will only cost $10 to build one. That means you can build your own APRS IGATE or tracking beacon with very low cost.

### How TinyAPRS works ###

TinyAPRS has good performance on decoding. 
Currently, it works perfectly with my GM300 radio and decodes packets successfully from OT++(The local Digipeater)/FTM350R(My Car) and other devices. Based on my test, it decodes 850+ records from WA8LMF APRS TestCD/Track1. 

### How TinyAPRS is built ###

TinyAPRS is built on the giants shoulder.

 - An ALC/Filter circut that is the same as OpenTracker used, which will  greatly improve the decode capability.(Thank you to BH4TDV for sharing the circut design)
 - A fine-tunned AFSK/AX25/KISS stack that runs on AVR chip with very small memory footprints. (Thank you to Mark Qvist for his MicroAPRS, but mine now is better :)
 - Good abstraction to the hardware with BertOS, which made it easy to port TinyAPRS firmware to other MCUs.


Technical specification 

 - Main Chip : ATMEL/ATMEGA328P (Cheap clones of Arduino ProMini)
 - ALC circut: MCP6001 OPA
 - Firmware  : AFSK1200/KISS(P-CSMA)/AT Serial

### The License ###

TinyAPRS is released under GPL license, **but with the following restriction** :

 - Any person, who decide to use and/or get benefits from any part of the TinyAPRS project, will be obliged to send the author(me/BG5HHP/Shawn) a gift. (Any kind of gift will be fine but Beers/PostCards/Parts of HAM radios are preferred :)
 - No commercial use.
 - All GPL restrictions


Cheers,

Shawn (BG5HHP)




