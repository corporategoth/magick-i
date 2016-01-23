# Makefile for Magick


include Makefile.inc

VERSION = 1.4

########################## Configuration section ##########################


# Compilation options:
#      -DWIN32 - This will make a binary for Win32 instead of UNIX.
#
# If you change this value, REMEMBER to "make clean", or you may come out
# with a confused executable!

CDEFS =


######################## End configuration section ########################


# I lied: you may need to change this depending on what flags your `cp'
# takes.  This should be the command to copy a directory and all its
# files to another directory.  (This is correct for Linux.)

CFLAGS = $(CDEFS) $(EXTRA_CFLAGS)

OBJS =	channels.$(OBJ) chanserv.$(OBJ) helpserv.$(OBJ) main.$(OBJ) memoserv.$(OBJ) misc.$(OBJ) \
        nickserv.$(OBJ) operserv.$(OBJ) process.$(OBJ) send.$(OBJ) sockutil.$(OBJ) users.$(OBJ) \
        cfgopts.$(OBJ) win32util.$(OBJ)
SRCS =	channels.c chanserv.c helpserv.c main.c memoserv.c misc.c \
        nickserv.c operserv.c process.c send.c sockutil.c users.c \
        cfgopts.c win32util.c


all: magick em

clean:
	rm -f *.$(OBJ) magick listnicks listchans magick.exe em

distclean: spotless

spotless: clean
	rm -rf config.cache sysconf.h Makefile.inc version.h configure.log tmp

install: all
	$(INSTALL) magick $(BINDEST)/magick
	rm -f $(BINDEST)/listnicks $(BINDEST)/listchans
	ln $(BINDEST)/magick $(BINDEST)/listnicks
	ln $(BINDEST)/magick $(BINDEST)/listchans
	$(INSTALL) serviceschk $(BINDEST)/serviceschk

install-data:
	$(CP_ALL) data/* $(DATDEST)
	$(CP_ALL) doc $(DATDEST)
	@if [ "$(RUNGROUP)" ] ; then \
		echo chgrp -R $(RUNGROUP) $(DATDEST) ; \
		chgrp -R $(RUNGROUP) $(DATDEST) ; \
		echo chmod -R g+rw $(DATDEST) ; \
		chmod -R g+rw $(DATDEST) ; \
		echo chmod g+xs \`find $(DATDEST) -type d -print\` ; \
		chmod g+xs `find $(DATDEST) -type d -print` ; \
	fi

backup: em

install-backup: em
	$(INSTALL) em $(BINDEST)/em
	$(INSTALL) getdbases $(BINDEST)/getdbases

########

magick: version.h $(OBJS)
	$(CC) $(LFLAGS) $(LIBS) $(OBJS) -o $@

em:
	$(CC) $(LFLAGS) $(LIBS) em.c -o $@

.c.$(OBJ):
	$(CC) $(CFLAGS) -c $<

channels.$(OBJ): channels.c services.h
chanserv.$(OBJ): chanserv.c cs-help.c services.h
helpserv.$(OBJ): helpserv.c services.h
main.$(OBJ): main.c services.h cfgopts.h
memoserv.$(OBJ): memoserv.c ms-help.c services.h
misc.$(OBJ): misc.c services.h
nickserv.$(OBJ): nickserv.c ns-help.c services.h
operserv.$(OBJ): operserv.c os-help.c services.h
process.$(OBJ): process.c services.h version.h
send.$(OBJ): send.c services.h
sockutil.$(OBJ): sockutil.c services.h
users.$(OBJ): users.c services.h
cfgopts.$(OBJ): cfgopts.c cfgopts.h
win32util.$(OBJ): win32util.c win32util.h

services.h: config.h extern.h
	touch $@

config.h: sysconf.h
	touch $@

version.h: services.h $(SRCS)
	sh version.sh $(VERSION)
