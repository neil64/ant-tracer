#
#	Build Tracer v0.1
#
######################################################################
#
#	The soft device to use.  Define only one of these.
#	(See Makefile-softdevice for more details)
#

# SOFTDEV = s110
# SOFTDEV = s120
# SOFTDEV = s130
# SOFTDEV = s132
# SOFTDEV = s132a
# SOFTDEV = s210
# SOFTDEV = s212a
# SOFTDEV = s212b
SOFTDEV = s212c
# SOFTDEV = s310
# SOFTDEV = s332a0
# SOFTDEV = s332a1
# SOFTDEV = s332a2
# SOFTDEV = s332a3

######################################################################
#
#	The defaults for various values.  Can be overridden in the local
#	Makefile.
#

RTTPort = 19021
VERSION_FILE = VERSION-production

######################################################################

include Makefile-softdevice

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

#
#   Enable development code.  It changes code to ease development, such
#   as fixing the network ID and extracting the node ID from the nRF52's
#   device ID.
#
OQ_FLAGS += -DOQ_DEVEL

#
#   Enable general debugging code, including the ability to printf
#   to the debugger comms port (RTT, or a serial port).
#
OQ_FLAGS += -DOQ_DEBUG

#
#   Enable tachyon debugging code-- high precision logging.  OQ_DEBUG
#   must also be defined for this to work.
#
OQ_FLAGS += -DOQ_TACHYON

#
#   Turn on various collections of debugging output.  See "include/oqdbg.h"
#   for #define's that define what each collection is associated with.
#
OQ_FLAGS += -DOQ_DEBUG_MASK=0xffff

#
#   Turn on the interactive command interpreter and debugger.
#
OQ_FLAGS += -DOQ_COMMAND

#
#	Turn on an alternate softdevice by adding the contents of the SD
# 	you wish to use in `Makefile-local'.
#

#
#	Debug and development flags
#

#
#   RTT Telnet Port Override
#
# RTTPort = 19021

#
#	The tracer requires debugging and tachyon.
#
OQ_FLAGS += -DOQ_DEBUG -DOQ_TACHYON

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
#  	Override DEVID with `make devid DEVID=12345678'.  Default is 0xffffffff
#  	which is the reset value for the UICR.  The `hexen' tool is used to
# 	generate the appropriate intel hex for the entry.
DEVID = ffffffff

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

ROOT =		../..
TOP =		..
TOOLS =		$(ROOT)/tools
LOADER =	$(ROOT)/loader

TARGET =	tracer

OBJS =	low.o ../cpu/system_nrf52.o main.o board.o cmd.o		\
	../app/tracer.o							\
	../time/rtc.o ../time/tempus.o					\
	../store/store.o ../store/config.o				\
	../misc/crc.o ../misc/rand.o             			\
	../debug/debugger.o ../debug/debug.o				\
	../debug/oq_nrf_debug.o 					\
	../debug/printf.o ../debug/tachyon.o				\
	../lib/libc/memcpy.o ../lib/libc/memset.o			\
	../lib/libc/strcmp.o ../lib/libc/strcpy.o			\
	../lib/libc/memcmp.o ../lib/libc/strlen.o			\
	../lib/rtt/SEGGER_RTT.o						\

##
X0FLAGS =	-mcpu=cortex-m4 -mthumb					\
		-mfloat-abi=hard -mfpu=fpv4-sp-d16 -fno-math-errno	\
		-g -Os -std=gnu99
		## -fno-strict-aliasing -nostdinc -nostdlib
		## -fno-builtin --short-enums
X1FLAGS =	## -ffunction-sections -fdata-sections
X2FLAGS =	-DOQ_RELAY -DNRF52 -DSOFTDEVICE_PRESENT $(NET_STACK_DEFS)
X3FLAGS =	-I.. -I../inc -I../lib $(DEBUG)		\
		-I$(ROOT)/proto -I$../modem/nrf
X4FLAGS =	-I$(NET_STACK_INCL)					\
		-I$(ROOT)/nordic/components/device			\
		-I$(ROOT)/nordic/components/toolchain			\
		-I$(ROOT)/nordic/components/toolchain/CMSIS/Include	\
		-I$(ROOT)/nordic/components/libraries/util		\
		-I$(ROOT)/nordic/components/libraries/timer		\
		-I$(ROOT)/nordic/components/libraries/trace		\
		-I$(ROOT)/nordic/components/drivers_nrf/spi_master	\
		-I$(ROOT)/nordic/components/drivers_nrf/twi_master	\
		-I$(ROOT)/nordic/components/drivers_nrf/config		\
		-I$(ROOT)/nordic/components/drivers_nrf/common		\
		-I$(ROOT)/nordic/components/drivers_nrf/pstorage	\
		-I$(ROOT)/nordic/components/drivers_nrf/hal		\
		-I$(ROOT)/nordic/components/drivers_nrf/delay		\
		-I$(ROOT)/nordic/components/libraries/util		\
		-I$(ROOT)/nordic/components/libraries/fstorage		\
		-I$(ROOT)/nordic/components/libraries/fstorage/config	\
		-I$(ROOT)/nordic/components/libraries/experimental_section_vars

CFLAGS =	-Wall $(X0FLAGS) $(X1FLAGS) $(X2FLAGS) $(X3FLAGS)	\
		$(X4FLAGS) $(OQ_FLAGS)
ASFLAGS =	$(X0FLAGS) $(X1FLAGS) $(X2FLAGS) $(X4FLAGS) -D__ASM__
LDFLAGS =	$(X0FLAGS) $(X1FLAGS)

.DELETE_ON_ERROR:

########################################################################

all:		version-dot-h $(TARGET).hex
	@echo
	@echo "=========  Version `cat ${VERSION_FILE}`  ========="
	@echo


dist:		$(TARGET).hex $(TARGET).bin $(TARGET).elf
	$(TOOLS)/mkout `cat VERSION-production` $^


#
#	NOTE: When using the JLink's `loadfile' method to load this hex file
#		  the extended linear address records resetting the range
#		  to 0 must be inserted between these record, otherwise only
#		  one range will be programmed.
#
#	NOTE: When using the better `nrfjprog' tool, the end of file record
#		  must be removed from all but the last hex file.  The linear
#		  address reset is overkill but safe to do.
#
$(TARGET).hex:	$(OBJS) loader
	@echo 'Linking:'
	arm-ld -g -T$(NET_LD_SCRIPT) -Tlinker-script.ld -o$(TARGET).elf \
		$(OBJS)
	arm-objcopy -Obinary $(TARGET).elf $(TARGET).bin
	arm-objcopy -Oihex $(TARGET).elf $(TARGET).h1

	sed '/:00000001FF/d' $(LOADER)/uicr1.hex >$(TARGET).hex
	@echo ':020000040000FA' >>$(TARGET).hex
	sed '/:00000001FF/d' $(LOADER)/uicr2.hex >>$(TARGET).hex
	@echo ':020000040000FA' >>$(TARGET).hex
ifeq ("$(shell test -e ./devid.hex && echo devid.hex)","devid.hex")
	sed '/:00000001FF/d' devid.hex >>$(TARGET).hex;
	@echo ':020000040000FA' >>$(TARGET).hex
endif
	sed '/:00000001FF/d' $(NET_STACK_HEX) >>$(TARGET).hex
	@echo ':020000040000FA' >>$(TARGET).hex
	sed '/:00000001FF/d' $(TARGET).h1 >>$(TARGET).hex
	@echo ':020000040000FA' >>$(TARGET).hex
	cat $(LOADER)/boot-loader.hex >>$(TARGET).hex
	@rm -f $(TARGET).h1


devid:
ifneq ($(DEVID), "ffffffff")
	$(TOOLS)/hexen 1000108C $(DEVID) -o devid.hex
endif


version-dot-h:
	$(TOOLS)/version2 $(VERSION_FILE) $(VERSION_INC) >version.h


loader:
	cd $(LOADER); $(MAKE) $(MAKEFLAGS)


depend:
	echo >Makefile-depend
	$(TOOLS)/makedep Makefile-depend $(CC) $(CFLAGS) $(OBJS)


clean:
	rm -f $(OBJS) $(BLEOBJS) \
		  flash.jlink erase-all.jlink  \
	      $(TARGET).elf $(TARGET).hex $(TARGET).bin \
	      $(TARGET)-uicr.hex \
	      devid.hex \
	      version.h
	cd $(LOADER); $(MAKE) $(MAKEFLAGS) clean


main.o:		version.h

######################################################################
#
#   Flash and debug helpers using JLink.
#

JLINK_OPTS = -device nRF52832_xxAA -if swd -speed 4000 -RTTTelnetPort $(RTTPort)
JLINK_GDB_OPTS = -noir
JLINK = JLinkExe $(JLINK_OPTS)
JLINKD_GDB = JLinkGDBServer $(JLINK_GDB_OPTS)


flash-jlink: flash.jlink
	$(JLINK) flash.jlink


flash.jlink:
	printf "loadfile tracer.hex\n" > flash.jlink
	printf "loadfile tracer-uicr.hex\n" >> flash.jlink
	printf "r\ng\n" >> flash.jlink


erase-all: erase-all.jlink
	$(JLINK) erase-all.jlink


erase-all.jlink:
	# Write to NVMC to enable erase, do erase all, wait for completion. reset
	printf "w4 4001e504 2\nw4 4001e50c 1\nsleep 100\nr\nexit\n" > erase-all.jlink


######################################################################
#
#	Dependencies
#

-include Makefile-depend

######################################################################

# vim:set noexpandtab shiftwidth=8 softtabstop=8:
