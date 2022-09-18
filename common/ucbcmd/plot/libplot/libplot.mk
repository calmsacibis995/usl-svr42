#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/libplot.mk	1.1"
#ident	"$Header: $"

#	Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#       Makefile for libplot

include $(CMDRULES)


ALL=	libf77plot libplot lib300 lib300s lib4013 lib4014 lib450 libvt0 \
	libplotaed libplotbg libplotdumb libplotgigi libplot2648 \
	libplot7221 libplotimagen

SUBDIRS=tf77 plot t4013 t4014 t300 t300s t450 vt0\
	aed bitgraph dumb gigi hp2648 hp7221 imagen

all:	${ALL}

libf77plot: FRC
	cd tf77; $(MAKE) -f tf77.mk 

libplot: FRC
	cd plot; $(MAKE) -f plot.mk 

lib4013: FRC
	cd t4013; $(MAKE) -f t4013.mk 

lib4014: FRC
	cd t4014; $(MAKE) -f t4014.mk 

lib300: FRC
	cd t300; $(MAKE) -f t300.mk 

lib300s: FRC
	cd t300s; $(MAKE) -f t300s.mk 

lib450: FRC
	cd t450; $(MAKE) -f t450.mk 

libvt0: FRC
	cd vt0; $(MAKE) -f vt0.mk 

libplotaed: FRC
	cd aed; $(MAKE) -f aed.mk 

libplotbg: FRC
	cd bitgraph; $(MAKE) -f bitgraph.mk 

libplotdumb: FRC
	cd dumb; $(MAKE) -f dumb.mk 

libplotgigi: FRC
	cd gigi; $(MAKE) -f gigi.mk 

libplot2648: FRC
	cd hp2648; $(MAKE) -f hp2648.mk 

libplot7221: FRC
	cd hp7221; $(MAKE) -f hp7221.mk 

libplotimagen: FRC
	cd imagen; $(MAKE) -f imagen.mk 

FRC:

clean:
	rm -f errs a.out core
	-for i in ${SUBDIRS}; do \
		(cd $$i; $(MAKE) -f $$i.mk clean); \
	done

clobber: 
	rm -f errs a.out core
	-for i in ${SUBDIRS}; do \
		(cd $$i; $(MAKE) -f $$i.mk clobber); \
	done
