/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:sum.c	1.1.6.2"
#ident  "$Header: sum.c 1.2 91/06/26 $"

#include "uucp.h"
#include <stdio.h>
#include <sys/types.h>
#include <crypt.h>

extern int Debug;

/* checksum algorithm used in uux and uuxqt to authenticate X. files */
/* checksum alrogithm borrowed from sum command (alternate algorithm */

unsigned
cksum(char *file)
{
	register unsigned sum = 0;
	FILE *f;
	int c;

	if ( (f = fopen(file, "r")) == NULL )
		return(0);

	while((c = getc(f)) != EOF) {
		if(sum & 01)
			sum = (sum >> 1) + 0x8000;
		else
			sum >>= 1;
		sum += c;
		sum &= 0xFFFF;
	}

	if (ferror(f))
		sum = 0;

	(void) fclose(f);

	return(sum);
}

