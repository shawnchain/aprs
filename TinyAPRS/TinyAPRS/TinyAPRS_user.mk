#
# User makefile.
# Edit this file to change compiler options and related stuff.
#

# Programmer interface configuration, see http://dev.bertos.org/wiki/ProgrammerInterface for help
TinyAPRS_PROGRAMMER_TYPE = usbasp
TinyAPRS_PROGRAMMER_PORT = asp

# Files included by the user.
TinyAPRS_USER_CSRC = \
	$(TinyAPRS_SRC_PATH)/main.c \
	$(TinyAPRS_HW_PATH)/hw/hw_afsk.c \
	$(TinyAPRS_HW_PATH)/lcd/hw_lcd_4884.c \
	$(TinyAPRS_HW_PATH)/console.c \
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
