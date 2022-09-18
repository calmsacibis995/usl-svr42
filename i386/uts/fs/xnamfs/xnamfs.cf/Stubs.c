/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:fs/xnamfs/xnamfs.cf/Stubs.c	1.4"
#ident	"$Header: $"

int xnamvp() { return 0; }
int xnampreval() { return nopkg(); }

int xsdfree() { return nopkg(); }
void xsdexit() {}
int xsdfork() {return 0; }
int sdget() { return nopkg(); }
int sdenter() { return nopkg(); }
int sdleave() { return nopkg(); }
int sdgetv() { return nopkg(); }
int sdwaitv() { return nopkg(); }
int sdsrch() { return 0; }

int xsemfork() { return 0; }
int creatsem() { return nopkg(); }
int opensem() { return nopkg(); }
int sigsem() { return nopkg(); }
int waitsem() { return nopkg(); }
int nbwaitsem() { return nopkg(); }
void closesem() {}
