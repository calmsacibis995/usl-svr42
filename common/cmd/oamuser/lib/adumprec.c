/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/adumprec.c	1.1.5.2"
#ident  "$Header: adumprec.c 2.0 91/07/13 $"

#include <sys/types.h>
#include <sys/param.h>
#include <audit.h>

/*
 * USER level interface to auditdmp(2)
 * for USER level audit event records
 */
void
adumprec(rtype,status,size,argp)
int rtype;			/* event type */
int status;			/* event status */
int size;			/* size of argp */
char *argp;			/* data/arguments */
{
        arec_t rec;

        rec.rtype = rtype;
        rec.rstatus = status;
        rec.rsize = size;
        rec.argp = argp;

        auditdmp(&rec, sizeof(arec_t));
        return;
}
