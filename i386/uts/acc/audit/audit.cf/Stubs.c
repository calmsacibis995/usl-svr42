/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:acc/audit/audit.cf/Stubs.c	1.6"

int audit_on = 0;

void adt_priv() {}
void adt_exit() {}
void adt_dupaproc() {}
int adt_installed() { return 0; }
void adtflush() {}
void adt_pathupd() {}
void adt_init() {}
void adt_recvfd() {}
void adt_record() {}
void adt_filenm() {}
void adt_symlink() {}
void adt_admin() {}
void adt_parmsset() {}
void adt_auditchk() {}
void adt_auditme() {}
void adt_freeaproc() {}
void adt_cc() {}
void adt_modload() {}
void adt_moduload() {}
void adt_kill() {}
int auditbuf() { return nopkg(); }
int auditlog() { return nopkg(); }
int auditdmp() { return nopkg(); }
int auditctl() { return nopkg(); }
int auditevt() { return nopkg(); }
