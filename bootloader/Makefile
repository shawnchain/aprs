# Based on XBoot

.checkout_source:
	@echo Checkout source ...
	@git clone https://github.com/alexforencich/xboot .xboot
	@touch .checkout_source

.patch_applied:
	@echo Applying patches ...
	@cp patches/tinyaprs.conf.mk .xboot/conf
	@touch .patch_applied

all: .checkout_source .patch_applied
	@echo Building bootloader ...
	make -C .xboot conf/tinyaprs.conf.mk
	@mkdir -p bin;cp -f .xboot/xboot.hex bin


clean:
	rm -fr .xboot
	rm -f .checkout_source .patch_applied
