#
#	Make build tools
#

PROGS1 =	version b2c fixup hexen
PROGS2 =	lkt tn

CFLAGS =	-m32 -Wall
## CFLAGS =	-g -m32

##############################################################

all:		one two

one:		$(PROGS1)

two:		$(PROGS2)


lkt:		lkt.o
	$(CC) $(CFLAGS) -o lkt $^



version:	version.c
	-@[ `uname` = Darwin ] && $(CC) $(CFLAGS) -o version-darwin version.c
	-@[ `uname` = Linux ] && $(CC) $(CFLAGS) -o version-linux version.c
	cp tool version


clean:
	rm -rf $(PROGS1) $(PROGS2) *.o *.dSYM
