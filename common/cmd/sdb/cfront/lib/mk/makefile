#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:cfront/lib/mk/makefile	1.3"

include $(CMDRULES)

O_NEW		=	_delete.o _handler.o _new.o _vec.o
O_STATIC	=	_ctor.o _dtor.o _main.o exit.o dtors.o
OFILES	=	$(O_NEW) $(O_STATIC)
CCFLAGS	=

libC.a:	$(OFILES)
	ar r libC.a $(OFILES)
	if test "$(BSD)" ; then ranlib libC.a; fi

_delete.o:	../new/_delete.c
	$(CC) $(CCFLAGS) -c ../new/_delete.c

_handler.o:	../new/_handler.c
	$(CC) $(CCFLAGS) -c ../new/_handler.c

_new.o:	../new/_new.c
	$(CC) $(CCFLAGS) -c ../new/_new.c

_vec.o:	../new/_vec.c
	$(CC) $(CCFLAGS) -c ../new/_vec.c

_ctor.o:	../static/_ctor.c
	$(CC) $(CCFLAGS) -c ../static/_ctor.c

_dtor.o:	../static/_dtor.c
	$(CC) $(CCFLAGS) -c ../static/_dtor.c

_main.o:	../static/_main.c
	$(CC) $(CCFLAGS) -c ../static/_main.c

dtors.o:	../static/dtors.c
	$(CC) $(CCFLAGS) -c ../static/dtors.c

exit.o:	../static/exit.c
	$(CC) $(CCFLAGS) -c ../static/exit.c

munch.o:	../static/munch.c
	$(CC) $(CCFLAGS) -c ../static/munch.c

clean:	
	rm -f $(OFILES)

clobber:	clean
	rm -f libC.a
