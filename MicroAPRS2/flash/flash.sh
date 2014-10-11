#!/bin/bash
AVRDUDE=/Users/shawn/Arduino/developer/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avrdude
AVRDUDE_CONFIG=/Users/shawn/Arduino/developer/Arduino.app/Contents/Resources/Java/hardware/tools/avr/etc/avrdude.conf
PROGRAMMER=/dev/cu.SLAB_USBtoUART

echo Flashing...
$AVRDUDE -C $AVRDUDE_CONFIG -p m328p -c arduino -P $PROGRAMMER -U flash:w:target/Modem.hex
#avrdude -p m328p -c arduino -P /dev/tty$1 -b 115200 -F -U flash:w:images/Modem.hex
