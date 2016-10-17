# Based on XBoot

.checkout_source:
	@echo Checkout source ...
	@git clone https://github.com/alexforencich/xboot .xboot
	@touch .checkout_source

.patch_applied:
	@echo Applying patches ...
	@cd .xboot; cat ../patches/*.patch | patch -p2
	@touch .patch_applied

all: .checkout_source .patch_applied
	@echo Building bootloader ...
	@cp -f conf/tinyaprs.conf.mk .xboot/conf
	make -C .xboot conf/tinyaprs.conf.mk
	@mkdir -p bin;cp -f .xboot/xboot.hex bin

flash:
	avrdude -p atmega328p -c usbasp -P usb -F -U flash:w:bin/xboot.hex:i -U hfuse:w:0xD8:m

clean:
	@[ -d .xboot ] && { make -C .xboot clean; } || :

distclean:
	rm -fr .xboot
	rm -f .checkout_source .patch_applied
