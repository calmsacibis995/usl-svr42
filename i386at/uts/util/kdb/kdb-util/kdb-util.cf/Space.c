/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:util/kdb/kdb-util/kdb-util.cf/Space.c	1.8"
#ident	"$Header: $"

#include <config.h>
#include <sys/kdebugger.h>
#include <sys/conf.h>

/*
 * To install additional debugger interfaces, add them to this table.
 * You must then also provide a stub for your init routine in case your
 * module is de-configured.
 */
extern void db_init();
void (*kdb_inittbl[])() = {
	db_init,
	(void (*)())0
};


extern struct conssw conssw;

#ifdef IASY
int iasyputchar(), iasygetchar();
static struct conssw asysw = {
	iasyputchar,	0,	iasygetchar
};
#endif

struct conssw *dbg_io[] = {
	&conssw,
#ifdef IASY
	&asysw,
#endif
};

struct conssw **cdbg_io = dbg_io;
int ndbg_io = sizeof(dbg_io) / sizeof(struct conssw *);

/*
 * As a security feature, the kdb_security flag (set by the KDBSECURITY
 * tuneable) is provided.  If it is non-zero, the debugger should ignore
 * attempts to enter from a console key sequence.
 */
int kdb_security = KDBSECURITY;

/*
 * Do flow control on dbprintf()
 */
int dodbwait = 1;
