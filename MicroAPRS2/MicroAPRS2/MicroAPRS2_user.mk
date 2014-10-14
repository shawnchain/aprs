#
# User makefile.
# Edit this file to change compiler options and related stuff.
#

# Programmer interface configuration, see http://dev.bertos.org/wiki/ProgrammerInterface for help
MicroAPRS2_PROGRAMMER_TYPE = usbasp
MicroAPRS2_PROGRAMMER_PORT = usb

# Files included by the user.
MicroAPRS2_USER_CSRC = \
	$(MicroAPRS2_SRC_PATH)/main.c \
	$(MicroAPRS2_HW_PATH)/hw/hw_afsk.c \
	$(MicroAPRS2_HW_PATH)/console.c \
	$(MicroAPRS2_SRC_PATH)/lcd/hw_lcd_4884.c \
	#

# Files included by the user.
MicroAPRS2_USER_PCSRC = \
	#

# Files included by the user.
MicroAPRS2_USER_CPPASRC = \
	#

# Files included by the user.
MicroAPRS2_USER_CXXSRC = \
	#

# Files included by the user.
MicroAPRS2_USER_ASRC = \
	#

# Flags included by the user.
MicroAPRS2_USER_LDFLAGS = \
	#

# Flags included by the user.
MicroAPRS2_USER_CPPAFLAGS = \
	#

# Flags included by the user.
MicroAPRS2_USER_CPPFLAGS = \
	-fno-strict-aliasing \
	-fwrapv \
	#
