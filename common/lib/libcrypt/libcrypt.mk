#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libcrypt:libcrypt.mk	1.20.4.5"

#	Makefile for libcrypt

include $(LIBRULES)

LDFLAGS =

LFLAGS = -G -dy -ztext

HFLAGS = -h /usr/lib/libcrypt.so

LOCALDEF = $(PICFLAG) $(INTERNATIONAL)

LINTFLAGS = 

MAKEFILE = libcrypt.mk

LIBRARY = libcrypt.a
LIBRARY_I = libcrypt_i.a
LIBRARY_D = libcrypt_d.a

DOTSO = libcrypt.so
DOTSO_I = libcrypt_i.so
DOTSO_D = libcrypt_d.so

INTLOBJS =  crypt.o cryptio.o des_crypt.o des_encrypt.o \
		cryptbuf.o enigma.o

OBJECTS =  crypt.o cryptio.o des_crypt.o des_decrypt.o des_encrypt.o \
		cryptbuf.o enigma.o

SOURCES =  crypt.c cryptio.c des_crypt.c des_decrypt.c des_encrypt.c \
		cryptbuf.c enigma.c

all:
	$(MAKE) -f libcrypt.mk clean $(LIBRARY_I) INTERNATIONAL=-DINTERNATIONAL PICFLAG=''
	$(MAKE) -f libcrypt.mk clean $(DOTSO_I)   INTERNATIONAL=-DINTERNATIONAL PICFLAG='$(PICFLAG)'
	if [ -s des_decrypt.c ] ;\
	then \
		$(MAKE) -f libcrypt.mk clean $(LIBRARY_D) PICFLAG='' ;\
		$(MAKE) -f libcrypt.mk clean $(DOTSO_D) PICFLAG='$(PICFLAG)' ;\
	fi
		
$(LIBRARY_I):	$(INTLOBJS)
	-rm -f des_decrypt.o
	$(AR) $(ARFLAGS) $(LIBRARY_I) `$(LORDER) $(INTLOBJS) | $(TSORT)`

$(DOTSO_I):	$(INTLOBJS)
	-rm -f des_decrypt.o
	$(CC) $(LFLAGS) $(HFLAGS) -o $(DOTSO_I) $(INTLOBJS)

$(LIBRARY_D): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY_D) `$(LORDER) $(OBJECTS) | $(TSORT)`

$(DOTSO_D):	$(OBJECTS)
	$(CC) $(LFLAGS) $(HFLAGS) -o $(DOTSO_D) $(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(OBJECTS) $(LIBRARY_I) $(LIBRARY_D) $(DOTSO_I) $(DOTSO_D)

install: all
	rm -f $(USRLIB)/$(LIBRARY)
	rm -f $(USRLIB)/$(DOTSO)
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY_I)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO_I)
	if [ -s des_decrypt.c ] ;\
	then \
		$(INS) -f $(USRLIB) -m 644 $(LIBRARY_D) ;\
		$(INS) -f $(USRLIB) -m 755 $(DOTSO_D) ;\
		ln -f $(USRLIB)/$(LIBRARY_D) $(USRLIB)/$(LIBRARY) ;\
		ln -f $(USRLIB)/$(DOTSO_D) $(USRLIB)/$(DOTSO) ;\
	else \
		ln -f $(USRLIB)/$(LIBRARY_I) $(USRLIB)/$(LIBRARY) ;\
		ln -f $(USRLIB)/$(DOTSO_I) $(USRLIB)/$(DOTSO) ;\
	fi

lintit:	
	$(LINT) $(LINTFLAGS) *.c
	
