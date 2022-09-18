#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/comm.mk	1.7"
# "@(#)comm.mk	1.16 'attmail mail(1) command'"
#
#	mail make file
#

PREFIX=ma
HDR = mail.h maillock.h libmail.h s_string.h config.h stdc.h
SURRDIR = $(USRLIB)/mail/surrcmd

INS = install
DIRS =	$(MBOXDIR) \
	$(MBOXDIR)/:saved \
	$(MBOXDIR)/:forward \
	$(MPDIR) \
	$(FILEDIR) \
	$(FILEDIR)/lists \
	$(SURRDIR) \
	$(SHRLIB)/mail

PRODUCT = mail
LPDEST =
TMPDIR = /usr/tmp

MAKE= make
SLIST=$(PREFIX).sl
ID=$(PREFIX)id

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(CPPDEFS) -c $*.c > $*.lerr || { cat $*.lerr; exit 1; }

MPSRC = _mail_pipe.c
MPOBJS = $(MPSRC:.c=.o) $(ID).o

CBSRC = ckbinarsys.c
CBOBJS = $(CBSRC:.c=.o) $(ID).o

NSRC =	notify2.c
NOBJS = $(NSRC:.c=.o) $(ID).o

LSRC = maillog.c
LOBJS = $(LSRC:.c=.o) $(ID).o

VSRC = vacation.sh vacation2.sh STD_VAC_MSG

MCSRC = mailcheck.c
MCOBJS = $(MCSRC:.c=.o) $(ID).o

LTOOLS = pmkid

SRC = \
	Dout.c Tout.c add_recip.c altenviron.c arefwding.c \
	cat.c ckdlivopts.c cksaved.c cksurg_rc.c clr_hinfo.c \
	cmdexpand.c copyback.c copylet.c copymt.c createmf.c del_Hdrs.c \
	del_Msg.c del_Recip.c delete.c doeopt.c doFopt.c done.c doopen.c \
	dowait.c dumpaff.c dumprcv.c errmsg.c fini_Let.c fini_Msg.c \
	fini_Rcpl.c fini_Tmp.c gendeliv.c getarg.c getcomment.c \
	gethead.c getlasthdr.c getline.c getnumbr.c getsurr.c goback.c init.c \
	init_Hdri.c init_Let.c init_Msg.c init_Rcpl.c init_Tmp.c \
	initsurr.c isheader.c isit.c istext.c legal.c lock.c \
	mailcomp.c main.c matchsurr.c maxbatch.c \
	mcopylet.c mkdate.c mkdead.c mktmp.c msetup_ex.c mta_ercode.c my_open.c \
	new_Hdrs.c new_Msg.c new_Recip.c nwsendlist.c ofrwd.c parse.c pckaffspot.c \
	pckrcvspot.c pickFrom.c pipletr.c poplist.c printhdr.c printmail.c \
	recip_par.c retmail.c savdead.c savehdrs.c sel_disp.c \
	send2acc.c send2bmvr.c send2clean.c send2d_p.c send2deny.c send2exec.c \
	send2fleft.c send2frt.c send2loc.c send2move.c send2mvr.c \
	send2post.c send2quit.c send2tran.c sendlist.c sendmail.c setletr.c \
	setmail.c setsig.c setsurg_bt.c setsurg_rc.c sizehdrs.c \
	stamp.c systm.c tokdef.c validmsg.c
OBJS = $(SRC:.c=.o) $(ID).o

LIBSRC = abspath.c basename.c bcollapse.c cascmp.c casncmp.c config.c check4mld.c \
	closefiles.c compat.c copystream.c delempty.c \
	errexit.c expargvec.c getdomain.c loopfork.c islocal.c \
	maillock.c mailsystem.c mgetenv.c newer.c notifyu.c \
	parse_ex.c popenvp.c poschown.c rename.c \
	rmfopen.c rmopendir.c s_string.c setup_exec.c sortafile.c strmove.c \
	skipspace.c skip2space.c substr.c systemvp.c trimnl.c xgetenv.c
LIBOBJS = $(LIBSRC:.c=.o)

RESRC = re/bm.c re/cw.c re/eg.c re/egbr.c re/egcanon.c re/egcomp.c \
	re/egcw.c re/egerror.c re/eglit.c re/egmatch.c re/egparen.c \
	re/egpos.c re/egstate.c re/re.c re/refile.c
REHDRS= re/io.h re/libc.h re/lre.h re/re.h

DSRC =	my_open.c
DOBJS =

LINTOBJS = $(SRC:.c=.ln) $(ID).ln
LINTERR = $(SRC:.c=.lerr) $(ID).lerr

ALSRC = alias.c
ALOBJS = alias.o $(ID).o

MISRC = mailinfo.c
MIOBJS = mailinfo.o $(ID).o

PCHSRC = pchown.c
PCHOBJS= pchown.o $(ID).o

UUCSRC = uucollapse.c
UUCOBJS= uucollapse.o $(ID).o

LMSRC = localmail.c
LMOBJS= localmail.o $(ID).o

RTSRC = retest.c
RTOBJS= retest.o $(ID).o

ALLSRC = $(SRC) $(DSRC) $(MPSRC) $(CBSRC) $(NSRC) $(VSRC) $(LSRC) $(HDR) $(MCSRC) \
	$(MAKEFILE) $(LIBSRC) $(LTOOLS) mailsurr binarsys names namefiles \
	$(ALSRC) $(MISRC) $(PCHSRC) $(UUCSRC) $(LMSRC)

.MUTEX:	all

all: maillock.h allib allmail allsmtp

allib: $(LIBMAIL) $(LIBRE)

allmail: mail mail_pipe ckbinarsys notify notify2 mailalias pchown \
	llib-lmail.ln std_vac_msg vacation vacation2 mailcheck \
	maillog mailinfo email.str Cmailsurr uucollapse localmail

allsmtp: ; @echo '\t( cd smtp;'; cd smtp; $(MAKE) -f smtp.mk smtp SMSRCMAKE=$(SMSRCMAKE) CMDRULES=$(CMDRULES); echo '\t)'

mail:	$(OBJS) $(DOBJS) $(LD_LIBS)
	$(CC) $(LD_FLAGS) -o $(PRODUCT) $(OBJS) $(DOBJS) $(MAILLIBS)

mail_pipe:	$(MPOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o mail_pipe $(MPOBJS) $(LIBMAIL)

ckbinarsys:	$(CBOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o ckbinarsys $(CBOBJS) $(LIBMAIL)

notify2:	$(NOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o notify2 $(NOBJS) $(LIBMAIL)

mailcheck:	$(MCOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o mailcheck $(MCOBJS) $(LIBMAIL)

maillog: $(LOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o maillog $(LOBJS) $(LIBMAIL)

mailalias: $(ALOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o mailalias $(ALOBJS) $(LIBMAIL)

mailinfo: $(MIOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o mailinfo $(MIOBJS) $(LIBMAIL)

pchown: $(PCHOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o pchown $(PCHOBJS) $(LIBMAIL)

uucollapse: $(UUCOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o uucollapse $(UUCOBJS) $(LIBMAIL)

localmail: $(LMOBJS) $(LIBMAIL)
	$(CC) $(LD_FLAGS) -o localmail $(LMOBJS) $(LIBMAIL)

retest: $(RTOBJS) $(LIBRE)
	$(CC) $(LD_FLAGS) -o retest $(RTOBJS) $(LIBRE)

email.str: pfmt.msgs
	sed	-e 's/^:[0-9][0-9]*://' \
		-e 's/^:U_[0-9][0-9]*://' \
		-e '/^#ident.*attmail/d' < pfmt.msgs > email.str

debug:
	$(MAKE) "CFLAGS=-g -DDEBUG" DOBJS=my_open.o LD_FLAGS= PRODUCT=Dmail mail

$(LIBMAIL): $(LIBOBJS)
	$(AR) $(ARFLAGS) $(LIBMAIL) $?
	$(RANLIB) $(LIBMAIL)

$(LIBRE): $(RESRC) $(REHDRS)
	@echo '\t( cd re;'; cd re && $(MAKE) $(LIBRE) VERS=$(VERS) VERS2=$(VERS2) REFLAGS="$(REFLAGS)" CMDRULES=$(CMDRULES); echo '\t)'
	$(RANLIB) $(LIBRE)

llib-lmail.ln: llib-lmail maillock.h libmail.h
	case $(LINT_C) in \
		lint-c ) $(LINT) $(LINTFLAGS) $(LOCALDEF) $(LOCALINC) -o mail llib-lmail ;; \
		*    )	 $(CC) -E -C -Dlint -D$(VERS) -I. -I$(INC) -I$(CRX)/usr/include llib-lmail | \
			 /usr/lib/lint1 -vx -H$(TMPDIR)/hlint.$$$$ > $(TMPDIR)/nlint$$$$ && \
			 mv $(TMPDIR)/nlint$$$$ llib-lmail.ln; \
			 rm -f $(TMPDIR)/hlint$$$$ ;; \
	esac

std_vac_msg: STD_VAC_MSG
	grep -v '^#.*ident' < STD_VAC_MSG > std_vac_msg

EDITPATH=	\
	sed < $? > $@ \
	    -e 's!REAL_PATH!$(REAL_PATH)!g' \
	    -e 's!USR_SHARE_LIB!$(REAL_SHRLIB)!g' \
	    -e 's!FORWARDDIR!$(REAL_MBOXDIR)/:forward!g' \
	    -e 's!MBOXDIR!$(REAL_MBOXDIR)!g' \
	    -e 's!VARSPOOLSMTPQ!$(REAL_VARSPOOLSMTPQ)!g' \
	    -e 's!SHELL!$(REAL_SHELL)!g' \
	    -e 's!^$(SH_OPTCMD) !!' \
	    -e 's!^$(SH_PRTCMD) !!' \
	    -e '/^USE/d' \
	    -e 's!VAR_MAIL!$(REAL_MBOXDIR)!g'

notify: notify.sh
	$(EDITPATH)

vacation: vacation.sh
	$(EDITPATH)

vacation2: vacation2.sh
	$(EDITPATH)

maillock.h: maillock.H
	$(EDITPATH)

Cmailsurr:
	: > Cmailsurr

ckdirs:
	@for i in $(DIRS); \
	do \
		echo "\t$(CH)mkdir $${i}"; \
		[ ! -d $$i ] && mkdir -p $$i; \
		echo "\t$(CH)chmod 775 $${i}"; \
		$(CH)chmod 775 $${i}; \
		echo "\t$(CH)chgrp mail $${i}"; \
		$(CH)chgrp mail $${i}; \
		echo; \
	done

calls:
	cflow $(CPPDEFS) $(SRC) > cflow.out
	cflow -r $(CPPDEFS) $(SRC) > cflow-r.out
	calls $(CPPDEFS) $(SRC) > calls.out
	cscope -b $(SRC) > calls.out
	ccalls | sort -u > ccalls.edges
	ccalls -p | sort -u > ccalls.params

install: ckdirs installmail installsmtp

installmail: allmail nocheckinstallmail

nocheckinstallmail:
	case $(INST_MSGFILES) in \
	  yes ) \
	    [ ! -d $(USRLIB)/locale/C/MSGFILES ] && mkdir -p $(USRLIB)/locale/C/MSGFILES ; \
	    $(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 -u bin -g mail email.str ;; \
	esac

	rm -f $(MBINDIR)/$(PRODUCT) $(MBINDIR)/r$(PRODUCT)
	$(INS) -f $(MBINDIR) -m 2511 -u bin -g mail $(PRODUCT)
	ln $(MBINDIR)/$(PRODUCT) $(MBINDIR)/r$(PRODUCT)
	$(INS) -f $(VAC_MSG_LOC) -m 644 -u bin -g mail std_vac_msg ; \

	rm -f $(MPDIR)/mail_pipe
	$(INS) -f $(MPDIR)   -m 4511 -u root -g bin mail_pipe
	$(INS) -f $(MPDIR)   -m 4511 -u root -g bin pchown

	$(INS) -f $(SURRDIR) -m 555 -u bin -g bin ckbinarsys
	$(INS) -f $(SURRDIR) -m 2755 -u bin -g mail maillog
	$(INS) -f $(SURRDIR) -m 2755 -u bin -g mail mailinfo
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin uucollapse
	$(INS) -f $(SURRDIR) -m 755 -u bin -g bin localmail

	$(INS) -f $(CBINDIR) -m 755 -u bin -g bin notify
	$(INS) -f $(MPDIR)   -m 755 -u bin -g bin notify2

	$(INS) -f $(CBINDIR) -m 755 -u bin -g bin vacation
	$(INS) -f $(MPDIR)   -m 755 -u bin -g bin vacation2
	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin namefiles
	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin names

	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin mailsurr
	rm -f $(USRLIB)/mail/mailsurr
	$(CH)$(SYMLINK) $(FILEDIR)/mailsurr $(USRLIB)/mail/mailsurr
	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin Cmailsurr

	$(INS) -f $(FILEDIR) -m 644 -u bin -g bin binarsys
	rm -f $(USRLIB)/binarsys
	$(CH)$(SYMLINK) $(LFILEDIR)/binarsys $(USRLIB)/binarsys

	$(INS) -f $(USRLIB) -m 644 -u bin -g bin $(LIBMAIL)
	$(INS) -f $(USRLIB) -m 644 -u bin -g bin llib-lmail
	$(INS) -f $(USRLIB) -m 644 -u bin -g bin llib-lmail.ln
	$(INS) -f $(USRINC) -m 644 -u bin -g bin maillock.h

	$(INS) -f $(CBINDIR) -m 755 -u bin -g bin mailalias
	$(INS) -f $(CBINDIR) -m 755 -u bin -g bin mailcheck

installsmtp: allsmtp
	@echo '\t( cd smtp'; cd smtp; $(MAKE) -f smtp.mk install SMSRCMAKE=$(SMSRCMAKE) CMDRULES=$(CMDRULES); echo '\t)'

$(OBJS) $(DOBJS):	$(HDR)

$(LINTOBJS): $(HDR)

print:
	pr -n $(ALLSRC) | lp $(LPDEST)

lintit: $(LINTOBJS) llib-lmail.ln /tmp
	@echo ==== libmail ====
	$(LINT) $(LINTFLAGS) $(CPPDEFS) $(LIBSRC)
	@echo ==== mail ====
	for i in $(LINTERR);do if [ -s $$i ]; then echo `basename $$i .lerr`.c ; cat $$i; fi; done
	$(LINT) $(LINTFLAGS) $(LINTOBJS) llib-lmail.ln
	@echo ==== ckbinarsys ====
	$(LINT) $(LINTFLAGS) $(CPPDEFS) $(CBSRC) casncmp.c $(ID).c
	@echo ==== mail_pipe ====
	$(LINT) $(LINTFLAGS) $(CPPDEFS) $(MPSRC) xgetenv.c setup_exec.c skipspace.c $(ID).c

clean: cleanmail cleansmtp

cleanmail: cleanlint cleanre
	-rm -f *.o
cleanlint:
	-rm -f $(LINTOBJS)
	-rm -f $(LINTERR)
cleanre:
	@echo '\t( cd re;'; cd re; $(MAKE) clean CMDRULES=$(CMDRULES); echo '\t)'
cleansmtp:
	@echo '\t( cd smtp;'; cd smtp; $(MAKE) -f smtp.mk clean SMSRCMAKE=$(SMSRCMAKE) CMDRULES=$(CMDRULES); echo '\t)'

clobber: clobbermail clobbersmtp

clobbermail: cleanmail
	rm -f mail rmail Dmail a.out mail_pipe ckbinarsys notify notify2 \
		std_vac_msg core *makeout* mon.out nohup.out mailalias pchown \
		llib-lmail.ln vacation vacation2 libmail.a libre.a maillog \
		maillock.h mailinfo mailcheck email.str $(ID).* Cmailsurr \
		uucollapse localmail retest
	@echo '\t( cd re;'; cd re; $(MAKE) clobber CMDRULES=$(CMDRULES); echo '\t)'
clobbersmtp:
	@echo '\t( cd smtp;'; cd smtp; $(MAKE) -f smtp.mk clobber SMSRCMAKE=$(SMSRCMAKE) CMDRULES=$(CMDRULES); echo '\t)'

$(ID).c: $(SLIST)
	chmod 775 ./pmkid
	./pmkid	$(SLIST)

chgrp:
	-chgrp mail mail
	chmod g+s mail
