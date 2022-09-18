#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libeti:form/makefile	1.15.2.4"
#

include $(LIBRULES)

PWD=.

GLOBALINC =
LOCALINC = -I$(PWD) -I$(PWD)/.. -I$(INC)

LIBRARY=	libform.a

PRODUCT= 	$(LIBRARY) llib-lform.ln llib-lform

SOURCES=form.c			\
	form_win.c		\
	form_sub.c		\
	form_init.c		\
	form_term.c		\
	field_init.c		\
	field_term.c		\
	form_user.c		\
	form_opts.c		\
	field.c			\
	field_just.c		\
	field_fore.c		\
	field_back.c		\
	field_pad.c		\
	field_buf.c		\
	field_stat.c		\
	field_user.c		\
	field_opts.c		\
	driver.c		\
	chg_field.c		\
	chg_page.c		\
	chg_char.c		\
	chg_data.c		\
	post.c			\
	utility.c		\
	fieldtype.c		\
	ty_enum.c		\
	ty_int.c		\
	ty_num.c		\
	ty_regexp.c		\
	ty_alpha.c		\
	ty_alnum.c		\
	regcmp.c		\
	regex.c

OBJ = \
	chg_char.o chg_data.o chg_field.o chg_page.o driver.o field.o \
	field_back.o field_buf.o field_fore.o field_init.o field_just.o \
	field_opts.o field_pad.o field_stat.o field_term.o field_user.o \
	fieldtype.o form.o form_init.o form_opts.o form_sub.o form_term.o \
	form_user.o form_win.o post.o regcmp.o regex.o \
	ty_alnum.o ty_alpha.o ty_enum.o ty_int.o ty_num.o ty_regexp.o utility.o

all:		$(PRODUCT)

$(LIBRARY):	$(OBJ)
	$(AR) $(ARFLAGS) $(LIBRARY) `$(LORDER) $(OBJ) | $(TSORT)`

chg_char.o: chg_char.c \
	./utility.h

chg_data.o: chg_data.c \
	./utility.h

chg_field.o: chg_field.c \
	./utility.h

chg_page.o: chg_page.c \
	./utility.h

driver.o: driver.c \
	./utility.h

field.o: field.c \
	./utility.h

field_back.o: field_back.c \
	./utility.h

field_buf.o: field_buf.c \
	./utility.h

field_fore.o: field_fore.c \
	./utility.h

field_init.o: field_init.c \
	./utility.h

field_just.o: field_just.c \
	./utility.h

field_opts.o: field_opts.c \
	./utility.h

field_pad.o: field_pad.c \
	./utility.h

field_stat.o: field_stat.c \
	./utility.h

field_term.o: field_term.c \
	./utility.h

field_user.o: field_user.c \
	./utility.h

fieldtype.o: fieldtype.c \
	./utility.h

form.o: form.c \
	./utility.h

form_init.o: form_init.c \
	./utility.h

form_opts.o: form_opts.c \
	./utility.h

form_sub.o: form_sub.c \
	./utility.h

form_term.o: form_term.c \
	./utility.h

form_user.o: form_user.c \
	./utility.h

form_win.o: form_win.c \
	./utility.h

llib-lform.o: llib-lform.c \
	./form.h

post.o: post.c \
	./utility.h

regcmp.o: regcmp.c

regex.o: regex.c

ty_alnum.o: ty_alnum.c \
	./utility.h

ty_alpha.o: ty_alpha.c \
	./utility.h

ty_enum.o: ty_enum.c \
	./utility.h

ty_int.o: ty_int.c \
	./utility.h

ty_num.o: ty_num.c \
	./utility.h

ty_regexp.o: ty_regexp.c \
	./utility.h

utility.o: utility.c \
	./utility.h

llib-lform:	llib-lform.c
		$(CP) llib-lform.c llib-lform

llib-lform.ln:	llib-lform.c
		$(LINT) -o form -vx $(LOCALINC) llib-lform.c

install:	all
	install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) $(LIBRARY)
	install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-lform
	install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-lform.ln
	install -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) form.h

lintit:		
		$(LINT) $(LOCALINC) $(LINTFLAGS) -l curses $(SOURCES)

clean:
		rm -f *.o

clobber:	clean
		rm -f $(PRODUCT)

.PRECIOUS:	$(LIBRARY)
