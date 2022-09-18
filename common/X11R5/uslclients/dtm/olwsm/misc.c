/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/misc.c	1.9"
#endif

#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include "misc.h"
#include "error.h"
#include "wsm.h"  

char *
_ASSERT(x, line, file)
	char *			x;
	int			line;
	char *			file;
{
	if (!x) {
		Dm__VaPrintMsg(TXT_errorMsg_badAssert, line, file);
	}
	return x;
}

char *
_strndup(p, q, n)
	char *			p;
	char *			q;
	int			n;
{
	strncpy(p, q, n);
	p[n] = 0;
	return p;
}

#ifdef NEED_BCOPY

int bcopy (b1, b2, length)
register unsigned char *b1, *b2;
register length;
{
    if ((b1 + length) <= b2 || (b2 + length) <= b1) 
       memcpy(b2, b1, length);
    else
    if (b1 < b2) {
	b2 += length;
	b1 += length;
	while (length--) {
	    *--b2 = *--b1;
	}
    }
    else 
	while (length--) {
	    *b2++ = *b1++;
	}
}

int bcmp(b1, b2, length)
char * b1;
char * b2;
int    length;
{
        return (memcmp(b1, b2, length));
}

int bzero(b, length)
char * b;
int    length;
{
	memset(b, 0, length);
}
/* Find the first set bit
 * i.e. least signifigant 1 bit:
 * 0 => 0
 * 1 => 1
 * 2 => 2
 * 3 => 1
 * 4 => 3
 */
int ffs(mask)
unsigned int	mask;
{
	register i;

	if ( ! mask ) return 0;
	i = 1;
	while (! (mask & 1)) {
		i++;
		mask >>= 1;
	}
	return i;
}
#endif
