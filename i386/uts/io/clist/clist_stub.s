/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86:io/clist/clist_stub.s	1.1"
	.ident	"$Header: $"
/
/ This file contains the stubs for clist.
/
include(../../util/mod/stub.m4)

MODULE(clist)
STUB(clist, getc, 0)
STUB(clist, putc, 0)
STUB(clist, getcf, 0)
STUB(clist, putcf, 0)
STUB(clist, getcb, 0)
STUB(clist, putcb, 0)
STUB(clist, getcbp, 0)
STUB(clist, putcbp, 0)
STUB(clist, ttopen, 0)
STUB(clist, ttclose, 0)
STUB(clist, ttread, 0)
STUB(clist, ttwrite, 0)
STUB(clist, ttin, 0)
STUB(clist, ttxput, 0)
STUB(clist, ttout, 0)
STUB(clist, tttimeo, 0)
STUB(clist, ttioctl, 0)
STUB(clist, ttiocom, 0)
STUB(clist, ttinit, 0)
STUB(clist, ttywait, 0)
STUB(clist, ttyflush, 0)
STUB(clist, canon, 0)
STUB(clist, ttrstrt, 0)
WSTUB(clist, cinit, mod_zero)
END_MODULE(clist)
