/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86at:io/hba/mcesdi_stub.s	1.1"
	.ident	"$Header: $"

/
/ This file contains the DLM stubs for mcesdi.
/

include(../../util/mod/stub.m4)

MODULE(mcesdi)
STUB(mcesdi, mces_bdinit, mod_zero)
WSTUB(mcesdi, mces_drvinit, mod_zero)
WSTUB(mcesdi, mces_cmd, mod_enoload)
WSTUB(mcesdi, mces_int, mod_zero)
END_MODULE(mcesdi)
