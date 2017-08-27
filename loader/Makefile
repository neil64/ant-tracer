#
#	Build the boot loader

######################################################################
#
#	The defaults for various values.  Can be overridden in the local
#	Makefile.
#

VERSION_FILE = VERSION-production

######################################################################

-include Makefile-local

######################################################################
#
#   Copy the following to the file "Makefile-local" in this directory,
#   and comment out anything you would like to set.
#
##########
#
#	Versioning -- define these to use a personal version number file,
#	with a revision that increases by 1 for every build.
# VERSION_FILE =	VERSION-local
# VERSION_INC =	1


########################################################################
#
#	Compiler
#

CROSS_COMPILE = arm-
CC =		$(CROSS_COMPILE)gcc
CXX =		$(CROSS_COMPILE)gcc
LD =		$(CROSS_COMPILE)ld
OBJCOPY =	$(CROSS_COMPILE)objcopy
AR =		$(CROSS_COMPILE)ar
RANLIB =	$(CROSS_COMPILE)ranlib

########################################################################
#
#	Build rules
#

%.o: %.c
	@$(TOOLS)/compile $< $(CC) -c $(CFLAGS) -o $@ $<
%.o: %.cxx
	@$(TOOLS)/compile $< $(CC) -c $(CXXFLAGS) -o $@ $<
%.o: %.S
	@$(TOOLS)/compile $< $(CC) -c $(ASFLAGS) -o $@ $<

########################################################################

ROOT =		..
TOOLS =		$(ROOT)/tools

TARGET =	boot-loader

OBJS =		low.o main.o store.o map.o \
		system_nrf52.o memset.o memcmp.o

##
X0FLAGS =	-mcpu=cortex-m4 -mthumb					\
		-mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-math-errno	\
		-g -Os -std=gnu99
		## -fno-strict-aliasing -nostdinc -nostdlib
		## -fno-builtin --short-enums
X1FLAGS =	## -ffunction-sections -fdata-sections
X2FLAGS =	-DNRF52 -DLOADER
X3FLAGS =	-I.. $(DEBUG)

CFLAGS =	-Wall $(X0FLAGS) $(X1FLAGS) $(X2FLAGS) $(X3FLAGS) $(OQ_FLAGS)
ASFLAGS =	$(X0FLAGS) $(X1FLAGS) $(X2FLAGS) -D__ASM__
LDFLAGS =	$(X0FLAGS) $(X1FLAGS)

.DELETE_ON_ERROR:

########################################################################

all:		version-dot-h $(TARGET).hex uicr1.hex uicr2.hex uicrnfc.hex


$(TARGET).hex:	$(OBJS)
	@echo 'Linking:'
	arm-ld -g -Tlinker-script.ld -o$(TARGET).elf \
		$(OBJS)
	arm-objcopy -Obinary $(TARGET).elf $(TARGET).bin
	arm-objcopy -Oihex $(TARGET).elf $(TARGET).hex


version-dot-h:
	$(TOOLS)/version2 $(VERSION_FILE) $(VERSION_INC) >version.h


clean:
	rm -f $(OBJS) \
	      $(TARGET).elf $(TARGET).hex $(TARGET).bin \
	      uicr1.o uicr1.elf uicr1.bin uicr1.hex \
	      uicr2.o uicr2.elf uicr2.bin uicr2.hex \
	      uicrnfc.o uicrnfc.elf uicrnfc.bin uicrnfc.hex


$(OBJS):	bl.h map.h


main.o: 	version.h

######################################################################
#
#	UICR configuration

uicr1.hex:	uicr1.o uicr1.ld
	arm-ld -Tuicr1.ld -ouicr1.elf uicr1.o
	arm-objcopy -Oihex uicr1.elf uicr1.hex


uicr2.hex:	uicr2.o uicr2.ld
	arm-ld -Tuicr2.ld -ouicr2.elf uicr2.o
	arm-objcopy -Oihex uicr2.elf uicr2.hex

uicrnfc.hex: 	uicrnfc.o uicrnfc.ld
	arm-ld -Tuicrnfc.ld -ouicrnfc.elf uicrnfc.o
	arm-objcopy -Oihex uicrnfc.elf uicrnfc.hex


######################################################################
#
#	Dependencies
#

-include Makefile-depend

######################################################################

# vim:set noexpandtab shiftwidth=8 softtabstop=8:
