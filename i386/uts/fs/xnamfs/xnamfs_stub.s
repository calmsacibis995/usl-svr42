/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86:fs/xnamfs/xnamfs_stub.s	1.3"
	.ident	"$Header: $"
/
/ This file contains the stubs for xnamfs.
/
include(../../util/mod/stub.m4)

MODULE(xnamfs)
STUB(xnamfs, xnamvp, mod_zero)
STUB(xnamfs, xnampreval, mod_enoload)
STUB(xnamfs, sdget, mod_enoload)
STUB(xnamfs, creatsem, mod_enoload)
WSTUB(xnamfs, xsemfork, mod_zero)
WSTUB(xnamfs, xsdexit, mod_einval)
WSTUB(xnamfs, xsdfork, mod_zero)
WSTUB(xnamfs, xsdfree, mod_einval)
WSTUB(xnamfs, sdenter, mod_einval)
WSTUB(xnamfs, sdleave, mod_einval)
WSTUB(xnamfs, sdgetv, mod_einval)
WSTUB(xnamfs, sdwaitv, mod_einval)
WSTUB(xnamfs, sdsrch, mod_einval)
WSTUB(xnamfs, opensem, mod_einval)
WSTUB(xnamfs, sigsem, mod_einval)
WSTUB(xnamfs, waitsem, mod_einval)
WSTUB(xnamfs, nbwaitsem, mod_einval)
WSTUB(xnamfs, closesem, mod_einval)
END_MODULE(xnamfs)
