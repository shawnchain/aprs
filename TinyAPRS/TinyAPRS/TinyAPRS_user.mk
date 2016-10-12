#
# User makefile.
# Edit this file to change compiler options and related stuff.
#

# Programmer interface configuration, see http://dev.bertos.org/wiki/ProgrammerInterface for help
#TinyAPRS_PROGRAMMER_TYPE = usbasp
#TinyAPRS_PROGRAMMER_PORT = asp

TinyAPRS_PROGRAMMER_TYPE = arduino

# USB = PL2303
TinyAPRS_PROGRAMMER_PORT = /dev/cu.usbserial
TinyAPRS_PROGRAMMER_BAUD = 57600

# USB = CP2102
#TinyAPRS_PROGRAMMER_PORT = /dev/cu.SLAB_USBtoUART
#TinyAPRS_PROGRAMMER_BAUD = 57600

# USB = FT232
#TinyAPRS_PROGRAMMER_PORT = /dev/cu.usbserial-AH02KRAG
#TinyAPRS_PROGRAMMER_PORT = /dev/cu.usbserial-AH02KRAF
#TinyAPRS_PROGRAMMER_BAUD = 115200

# Files included by the user.
TinyAPRS_USER_CSRC = \
	$(TinyAPRS_SRC_PATH)/hw/hw_afsk.c \
	$(TinyAPRS_SRC_PATH)/utils.c \
	$(TinyAPRS_SRC_PATH)/reader.c \
	$(TinyAPRS_SRC_PATH)/settings.c \
	$(TinyAPRS_SRC_PATH)/twi.c

ifeq ($(I2C_TEST_MASTER),1)
TinyAPRS_USER_CSRC += $(TinyAPRS_SRC_PATH)/i2c_test_master.c
endif
ifeq ($(I2C_TEST_SLAVE),1)
TinyAPRS_USER_CSRC += $(TinyAPRS_SRC_PATH)/i2c_test_slave.c
endif
ifeq ($(I2C_TEST_MASTER)$(I2C_TEST_SLAVE),)
TinyAPRS_USER_CSRC += $(TinyAPRS_SRC_PATH)/main.c
endif

ifeq ($(ALL),1)
MOD_CONSOLE := 1
MOD_KISS := 1
MOD_TRACKER := 1
MOD_DIGI := 1
else
MOD_CONSOLE := 0
MOD_KISS := 0
MOD_TRACKER := 0
MOD_DIGI := 0
MOD_BEACON := 0
endif


ifeq ($(MOD_CONSOLE),1)
TinyAPRS_USER_CSRC += \
	$(TinyAPRS_SRC_PATH)/console.c
endif

ifeq ($(MOD_KISS),1)
TinyAPRS_USER_CSRC += \
	$(TinyAPRS_SRC_PATH)/net/kiss.c
endif

ifeq ($(MOD_TRACKER),1)
MOD_BEACON = 1
TinyAPRS_USER_CSRC += \
	$(TinyAPRS_SRC_PATH)/gps.c \
	$(TinyAPRS_SRC_PATH)/tracker.c
endif

ifeq ($(MOD_DIGI),1)
MOD_BEACON = 1
TinyAPRS_USER_CSRC += \
	$(TinyAPRS_SRC_PATH)/digi.c
endif

ifeq ($(MOD_WX),1)
endif

ifeq ($(MOD_BEACON),1)
TinyAPRS_USER_CSRC += \
	$(TinyAPRS_SRC_PATH)/beacon.c
endif

MOD_RADIO := 0
#TinyAPRS_USER_CSRC += \
	#$(TinyAPRS_SRC_PATH)/lcd/hw_lcd_4884.c \	
	#$(TinyAPRS_SRC_PATH)/hw/hw_softser.c \
	#$(TinyAPRS_SRC_PATH)/radio.c \
	#

# Files included by the user.
TinyAPRS_USER_PCSRC = \
	#

# Files included by the user.
TinyAPRS_USER_CPPASRC = \
	#

# Files included by the user.
TinyAPRS_USER_CXXSRC = \
	#

# Files included by the user.
TinyAPRS_USER_ASRC = \
	#

# Flags included by the user.
TinyAPRS_USER_LDFLAGS = \
	#

# Flags included by the user.
TinyAPRS_USER_CPPAFLAGS = \
	#

# Flags included by the user.
TinyAPRS_USER_CPPFLAGS = \
	-fno-strict-aliasing \
	-fwrapv \
	#

#Append the module flags to CC flags
TinyAPRS_USER_CPPFLAGS += \
	-D'MOD_KISS=$(MOD_KISS)' \
	-D'MOD_TRACKER=$(MOD_TRACKER)' \
	-D'MOD_DIGI=$(MOD_DIGI)' \
	-D'MOD_BEACON=$(MOD_BEACON)' \
	-D'MOD_RADIO=$(MOD_RADIO)' \
	-D'MOD_CONSOLE=$(MOD_CONSOLE)'

# Print binary size, make sure avr-size is in the PATH env
AVRSIZE=avr-size
print_size:
	$(AVRSIZE) --format=avr --mcu=$(TinyAPRS_MCU) $(OUTDIR)/$(TRG).elf

# Just flash the image, make sure avrdude is in the path
flash_image:
	$(AVRDUDE) -p $(TinyAPRS_MCU) -c $(TinyAPRS_PROGRAMMER_TYPE) -P $(TinyAPRS_PROGRAMMER_PORT) -b $(TinyAPRS_PROGRAMMER_BAUD) -F -U flash:w:$(OUTDIR)/$(TRG).hex:i

# Build and flash to the target
flash: flash_$(TRG)
	
