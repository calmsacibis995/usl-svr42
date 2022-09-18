/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)truss:i386/cmd/truss/prt.c	1.1"
#ident	"$Header: prt.c 1.1 91/07/09 $"

#include <stdio.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "machdep.h"

void
prt_si86(val, raw)	/* print sysi86 code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : si86name(val);

	if (s == NULL)
		prt_dec(val, 0);
	else
		outstring(s);
}

extern char *cxenixname();

void
prt_cxen(val, raw)	/* print cxenix code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : cxenixname(val);

	if (s == NULL)
		prt_dec(val, 0);
	else
		outstring(s);
}
