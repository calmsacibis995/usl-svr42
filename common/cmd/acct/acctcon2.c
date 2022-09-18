/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/acctcon2.c	1.9.3.3"
#ident "$Header: $"
/*
 *	acctcon2 <ctmp >ctacct
 *	reads std. input (ctmp.h/ascii format)
 *	converts to tacct.h form, writes to std. output
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>

struct	ctmp	cb;
struct	tacct	tb;

main(argc, argv)
char **argv;
{
	tb.ta_sc = 1;
	while (scanf("%lu\t%ld\t%s\t%lu\t%lu\t%lu\t%*[^\n]",
		&cb.ct_tty,
		&cb.ct_uid,
		cb.ct_name,
		&cb.ct_con[0],
		&cb.ct_con[1],
		&cb.ct_start) != EOF) {

		tb.ta_uid = cb.ct_uid;
		CPYN(tb.ta_name, cb.ct_name);
		tb.ta_con[0] = MINS(cb.ct_con[0]);
		tb.ta_con[1] = MINS(cb.ct_con[1]);
		fwrite(&tb, sizeof(tb), 1, stdout);
	}
}
