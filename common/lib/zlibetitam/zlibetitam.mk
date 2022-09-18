#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libetitam:zlibetitam.mk	1.7.3.2"
#

include $(LIBRULES)

PWD=.
ADDON = $(ROOT)/$(MACH)/usr/add-on
LOCALINC = -I$(PWD)

CVTSRC=listop.c wcreate.c winit.c wdelete.c wselect.c \
    wprintf.c wgetsel.c wrefresh.c validwin.c wsetstat.c \
    wgetstat.c undowindow.c doborder.c wput.c wgoto.c \
    wgetpos.c wgetc.c wcmd.c wprompt.c wprexec.c wpostwait.c keypad.c \
    post.c wuser.c wlabel.c tamdefs.c compat.c wslk.c \
    ReadMagic.c Virt2Ansi.c winsize.c werase.c wnodelay.c envinit.c \
    wexit.c track.c

MENUSRC=mbegin.c mmatch.c mcitems.c mscroll.c mctitle.c mshape.c \
    mdisplay.c mtitle.c mend.c mtrunc.c menu.c minput.c 

WINDSRC=wind.c
FORMSRC=form.c
PBFSRC=pbf.c
MESSAGESRC=message.c
HELPSRC=help.c help_ds.c help_kb.c

CVTOBJS=listop.o wcreate.o winit.o wdelete.o wselect.o \
    wprintf.o wgetsel.o wrefresh.o validwin.o wsetstat.o \
    wgetstat.o undowindow.o doborder.o wput.o wgoto.o \
    wgetpos.o wgetc.o wcmd.o wprompt.o wprexec.o wpostwait.o keypad.o \
    post.o wuser.o wlabel.o tamdefs.o compat.o wslk.o \
    ReadMagic.o Virt2Ansi.o winsize.o werase.o wnodelay.o envinit.o \
    wexit.o track.o

MENUOBJS=mbegin.o mmatch.o mcitems.o mscroll.o mctitle.o mshape.o \
	mdisplay.o mtitle.o mend.o mtrunc.o menu.o minput.o

WINDOBJS=wind.o
FORMOBJS=form.o
PBFOBJS=pbf.o
MESSAGEOBJS=message.o
HELPOBJS=help.o help_ds.o help_kb.o

TAMOBJS=$(MENUOBJS) $(FORMOBJS) $(PBFOBJS) $(MESSAGEOBJS) $(WINDOBJS)

LLIBC=llib-lmenu.c llib-lpbf.c llib-lform.c llib-lmsg.c llib-lwind.c llib-lcvt.c
LLIBLN=llib-lmenu.ln llib-lpbf.ln llib-lform.ln llib-lmsg.ln llib-lwind.ln llib-lcvt.ln

HEADERS=cvttam.h path.h \
    	   $(INC)/curses.h \
    	   $(INC)/varargs.h \
    	   $(INC)/unctrl.h \
    	   $(INC)/stdio.h

TAMINC=menu.h track.h message.h kcodes.h chartam.h subcurses.h \
       form.h pbf.h tam.h wind.h tamwin.h print.h temp.h

TAMSYSINC=sys/window.h sys/font.h sys/mouse.h sys/iohw.h sys/signal.h

PRODUCT=libtam.a llib-ltam.ln llib-ltam tamhelp

###################

all:		$(PRODUCT)

$(CVTOBJS):	$(HEADERS)

$(TAMOBJS):	$(TAMINC)

libtam.a:	$(CVTOBJS) $(TAMOBJS)
		$(AR) $(ARFLAGS) libtam.a `$(LORDER) *.o | $(TSORT)`

tamhelp:	$(HELPOBJS) help.h libtam.a
		$(CC) $(LDFLAGS) $(HELPOBJS) libtam.a -lcurses -o tamhelp

.SUFFIXES: .ln .c

.c.ln:
		$(LINT) $(LOCALINC) -v -u -x -o xx $<
		mv llib-lxx.ln $(<:.c=.ln)

$(LLIBLN):	$(LLIBC)

llib-ltam:	llib-ltam.c
		$(CP) llib-ltam.c llib-ltam

llib-ltam.ln:	llib-ltam.c
		$(LINT) -o tam -vx $(LOCALINC) llib-ltam.c

lintit:		$(LLIBLN)
		$(LINT) $(LOCALINC) $(LINTFLAGS) -l curses $(CVTSRC)
		$(LINT) $(LOCALINC) $(LINTFLAGS) llib-lcvt.ln $(WINDSRC)
		$(LINT) $(LOCALINC) $(LINTFLAGS) llib-lcvt.ln llib-lwind.ln $(MENUSRC)
		$(LINT) $(LOCALINC) $(LINTFLAGS) llib-lcvt.ln llib-lmenu.ln $(MESSAGESRC)
		$(LINT) $(LOCALINC) $(LINTFLAGS) llib-lcvt.ln llib-lwind.ln llib-lmenu.ln llib-lmsg.ln $(FORMSRC)
		$(LINT) $(LOCALINC) $(LINTFLAGS) $(PBFSRC)

install:	all
		$(INS) -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) libtam.a
		$(INS) -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-ltam
		$(INS) -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-ltam.ln
		$(STRIP) tamhelp
		$(INS) -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) tamhelp
		if [ ! -d $(ADDON) ]; \
		then \
			mkdir $(ADDON); \
			$(CH)chmod 775 $(ADDON); \
		fi
		if [ ! -d $(ADDON)/include ]; \
		then \
			mkdir $(ADDON)/include; \
			$(CH)chmod 775 $(ADDON)/include; \
		fi
		if [ ! -d $(ADDON)/include/sys ]; \
		then \
			mkdir $(ADDON)/include/sys; \
			$(CH)chmod 775 $(ADDON)/include/sys; \
		fi
		$(INS) -f $(ADDON)/include/sys -m 644 -u $(OWN) -g $(GRP) sys/window.h
		$(INS) -f $(ADDON)/include/sys -m 644 -u $(OWN) -g $(GRP) sys/font.h
		$(INS) -f $(ADDON)/include/sys -m 644 -u $(OWN) -g $(GRP) sys/mouse.h
		$(INS) -f $(ADDON)/include/sys -m 644 -u $(OWN) -g $(GRP) sys/iohw.h
		$(INS) -f $(ADDON)/include/sys -m 644 -u $(OWN) -g $(GRP) sys/signal.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) track.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) message.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) kcodes.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) chartam.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) subcurses.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) menu.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) form.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) pbf.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) tam.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) wind.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) tamwin.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) print.h
		$(INS) -f $(ADDON)/include -m 644 -u $(OWN) -g $(GRP) temp.h

clean:		
		rm -f *.o

clobber:	clean
		rm -f $(PRODUCT)        # everything thats created
		rm -f *.ln 2>/dev/null
