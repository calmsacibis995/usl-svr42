#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/plot/plot.mk	1.1"
#ident	"$Header: $"

#       Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved

include $(CMDRULES)

LOCALINC = -I$(ROOT)/$(MACH)/usr/ucbinclude

LIB_OPT = -L $(ROOT)/$(MACH)/usr/ucblib

LDFLAGS = -s $(LIB_OPT)

OWN = bin

GRP = bin

LDLIBS = -lucb -lm

INSDIR = $(ROOT)/$(MACH)/usr/ucb

ALL =	tek t4013 t300 t300s t450 aedplot bgplot dumbplot gigiplot \
	hpplot hp7221plot implot atoplot plottoa vplot crtplot plot

all:	${ALL} debug 

tek:	libplot/libt4014.a driver.o
	${CC} -o tek  driver.o libplot/libt4014.a $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

t4013:	libplot/libt4013.a driver.o
	${CC} -o t4013  driver.o libplot/libt4013.a  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

t300:	libplot/libt300.a driver.o 
	${CC} -o t300 driver.o libplot/libt300.a  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

t300s:	libplot/libt300s.a driver.o 
	${CC} -o t300s driver.o libplot/libt300s.a $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

t450:	libplot/libt450.a driver.o 
	${CC} -o t450 driver.o libplot/libt450.a $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

vplot:	vplot.o chrtab.o
	${CC} -o vplot vplot.o chrtab.o $(LDFLAGS) -lucb $(PERFLIBS)
crtplot:	crtplot.o crtdriver.o
	${CC} -o crtplot crtplot.o crtdriver.o $(LDFLAGS) $(LDLIBS) -lcurses -ltermcap  -lucb $(PERFLIBS)

aedplot: libplot/libaed.a driver.o
	${CC} -o aedplot driver.o libplot/libaed.a $(LDFLAGS) -lucb $(PERFLIBS)

bgplot: libplot/plotbg.a driver.o
	${CC} -o bgplot driver.o libplot/plotbg.a $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

dumbplot: libplot/libdumb.a driver.o
	${CC} -o dumbplot driver.o libplot/libdumb.a $(LDFLAGS) -lucb -ltermcap -lm -lucb $(PERFLIBS)

gigiplot: libplot/libgigi.a driver.o
	${CC} -o gigiplot driver.o libplot/libgigi.a $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

hpplot: libplot/libhp2648.a driver.o
	${CC} -o hpplot driver.o libplot/libhp2648.a $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

hp7221plot: libplot/libhp7221.a driver.o
	${CC} -o hp7221plot driver.o libplot/libhp7221.a  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

implot: libplot/libimagen.a driver.o
	${CC} -o implot driver.o libplot/libimagen.a  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

atoplot: libplot/libplot.a atoplot.o
	${CC} -o atoplot atoplot.o libplot/libplot.a  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

plottoa: plottoa.o
	${CC} -o plottoa plottoa.o $(LDFLAGS) -lucb $(PERFLIBS)

libplot/libt300.a:
	cd libplot; $(MAKE) -f libplot.mk lib300

libplot/libt300s.a: 
	cd libplot; $(MAKE) -f libplot.mk lib300s

libplot/libt450.a: 
	cd libplot; $(MAKE) -f libplot.mk lib450

libplot/libt4014.a: 
	cd libplot; $(MAKE) -f libplot.mk lib4014

libplot/libaed.a:
	cd libplot; $(MAKE) -f libplot.mk libplotaed

libplot/plotbg.a:
	cd libplot; $(MAKE) -f libplot.mk libplotbg

libplot/libdumb.a:
	cd libplot; $(MAKE) -f libplot.mk libplotdumb

libplot/libf77plot.a:
	cd libplot; $(MAKE) -f libplot.mk libf77plot

libplot/libgigi.a:
	cd libplot; $(MAKE) -f libplot.mk libplotgigi

libplot/libhp2648.a:
	cd libplot; $(MAKE) -f libplot.mk libplot2648

libplot/libhp7221.a:
	cd libplot; $(MAKE) -f libplot.mk libplot7221

libplot/libimagen.a:
	cd libplot; $(MAKE) -f libplot.mk libplotimagen

libplot/libplot.a:
	cd libplot; $(MAKE) -f libplot.mk libplot

libplot/libt4013.a:
	cd libplot; $(MAKE) -f libplot.mk lib4013

plot: plot.sh
	cp plot.sh plot

debug: debug.o
	${CC} -o debug debug.o $(LDFLAGS) -lucb $(PERFLIBS)

install: all plot
	-for i in ${ALL}; do \
		($(INS) -f  $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $$i); done

clean:
	cd libplot; $(MAKE) -f libplot.mk clean
	rm -f *.o a.out core errs

clobber: 
	cd libplot; $(MAKE) -f libplot.mk clobber
	rm -f *.o a.out core errs
	rm -f ${ALL} debug
