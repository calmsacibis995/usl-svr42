/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86at:io/hba/athd_stub.s	1.1"
	.ident	"$Header: $"
/
/ This file contains the DLM stubs for athd.
/
include(../../util/mod/stub.m4)

MODULE(athd)
STUB(athd, athd_bdinit, mod_zero)
WSTUB(athd, athd_drvinit, mod_zero)
WSTUB(athd, athd_cmd, mod_enoload)
WSTUB(athd, athd_int, mod_zero)
END_MODULE(athd)
