/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:acc/mac/mac.cf/Stubs.c	1.3"
#ident	"$Header: $"

int mac_installed = 0;

int mac_init() { return 0; }
int mac_vaccess() { return 0; }
int mac_lvldom() { return 0; }
int mac_liddom() { return 0; }
int mac_getlevel() { return 0; }
int mac_valid() { return 0; }
void mac_free() {}
int mac_openlid() { return nopkg(); }
int mac_rootlid() { return nopkg(); }
int mac_adtlid() { return nopkg(); }

void cc_limiter() {}
int cc_getinfo(){ return 0; }

int lvldom() { return nopkg(); }
int lvlequal() { return nopkg(); }
int lvlipc() { return nopkg(); }
int lvlproc() { return nopkg(); }
int lvlvfs() { return nopkg(); }

int mldmode() { return nopkg(); }
int mkmld() { return nopkg(); }

int devstat() { return nopkg(); }
int fdevstat() { return nopkg(); }
