# TinyAPRS - A cheap APRS TNC for HAM #



### What is TinyAPRS ###

Like most of the APRS TNCs, TinyAPRS is an AFSK1200 modem that decodes radio signals into AX25 data packets. TinyAPRS also supports KISS protocol, so you can use it with most of the APRS applications.

Unlike most of the APRS TNCs, tinyAPRS is VERY VERY CHEAP. It will only cost $10 to build one. That means you can build your own APRS IGATE or tracking beacon with very low cost.

TinyAPRS has good performance on decoding. 
Currently, it works perfectly with my GM300 radio and decodes packets successfully from OT++(The local Digipeater)/FTM350R(My Car) and other devices. Based on my test, it decodes 850+ records from WA8LMF APRS TestCD/Track1. 

### How did it work ###

TinyAPRS is built on the giants shoulder.

 - An ALC/Filter circut that is the same as OpenTracker used, which will  greatly improve the decode capability.(Thank you to BH4TDV for sharing the circut design)
 - A fine-tunned AFSK/AX25/KISS stack that runs on AVR chip with very small memory footprints. (Thank you to Mark Qvist for his MicroAPRS, but mine now is better :)
 - Good abstraction to the hardware with BertOS, which made it easy to port TinyAPRS firmware to other MCUs.


Technical specification 

 - Main Chip : ATMEL/ATMEGA328P (Cheap clones of Arduino ProMini, could be ported to other type of AVR MCUs with ease)
 - ALC circut: MCP6001 OPA
 - Firmware  : AFSK1200/KISS(P-CSMA)/AT Serial

### The License ###

TinyAPRS is released under GPL license, **but with the following restriction** :

 - Any person, who decide to use and/or get benefits from any part of the TinyAPRS project, will be obliged to send the author(me/BG5HHP/Shawn) a gift. (Any kind of gift will be fine but Beers/PostCards/Parts of HAM radios are preferred :)
 - No commercial use.
 - All GPL restrictions

### Pictures ###
![Image0](http://ww4.sinaimg.cn/mw1024/8a58507cjw1epkgrq4w49j218g18gtqq.jpg)

![Image1](http://ww2.sinaimg.cn/mw1024/8a58507cjw1enksv3eme7j218g0xc4cf.jpg)

![Image2](http://ww2.sinaimg.cn/mw1024/8a58507cjw1enksv2ekyfj218g0xcnd6.jpg)

![Image3](http://ww2.sinaimg.cn/mw1024/8a58507cjw1enksv58n73j218g0xcwt3.jpg)

![Image4](http://ww3.sinaimg.cn/mw1024/8a58507cjw1epkgro575lj218g18g19p.jpg)


Please visit my [weibo album](http://www.weibo.com/p/1005052321043580/album?from=page_100505&mod=TAB#place "Weibo Album") for more pictures.

Cheers,

Shawn (BG5HHP)




