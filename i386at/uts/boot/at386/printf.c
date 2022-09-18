/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/at386/printf.c	1.1"
#ident  "$Header: $"

#include "util/types.h"
#include "util/param.h"
#include "util/sysmacros.h"

#include "boot/boot.h"

/* Stolen from the kernel */

#define	CR

/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %lu %ld (==%lu)
 * %lo %lx are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much suspended.
 * Printf should not be used for chit-chat.
 */

printf(fmt, x1)
register char *fmt;
unsigned x1;
{
	register c;
	register char *adx;
	char *s;
	int	delay = 0;

	adx = (char *)&x1;
loop:
	while((c = *fmt++) != '%') {
		if(c == '\0') {
			if (delay) {
				for (;delay < 10;delay++) wait1s();
			}
			return;
		}
#ifdef	CR
		if (c == '\n')
			putchar('\r');
#endif /* CR */
		putchar(c);
	}
	c = *fmt++;
	if(c == 'd' || c == 'u' || c == 'o' || c == 'x') {
		printn((long)*(unsigned int *)adx, c=='o'? 8: (c=='x'? 16:10));
		adx += sizeof (int);
	}
	else if(c == 's') {
		s = *(char **)adx;
		while(c = *s++) {
#ifdef	CR
			if (c == '\n')
				putchar('\r');
#endif /* CR */
			putchar(c);
		}
		adx += sizeof (char *);
	} else if (c == 'l') {
		c = *fmt++;
		if(c == 'd' || c == 'u' || c == 'o' || c == 'x') {
			printn(*(long *)adx, c=='o'? 8: (c=='x'? 16:10));
			adx += sizeof (long);
		}
	} else if (c == 'Y') delay = 1;
	
	goto loop;
}

printn(n, b)
long n;
register b;
{
	register i, nd, c;
	int	flag, dflag;
	int	plmax;
	char d[12];

	c = 1;
	flag = n < 0;
	if (flag)
		n = (-n);
	dflag = b < 0;
	if (dflag)
		b = (-b);
	if (b==8)
		plmax = 11;
	else if (b==10)
		plmax = 10;
	else if (b==16)
		plmax = 8;
	if (flag && b==10) {
		flag = 0;
		putchar('-');
	}
	for (i=0;i<plmax;i++) {
		nd = n%b;
		if (flag) {
			nd = (b - 1) - nd + c;
			if (nd >= b) {
				nd -= b;
				c = 1;
			} else
				c = 0;
		}
		d[i] = nd;
		n = n/b;
		if ((n==0) && (flag==0))
			break;
	}
	if (i==plmax)
		i--;
	else if ( dflag && (i < --plmax))
		for (c = plmax - i; c > 0;c--) putchar(' ');
	for (;i>=0;i--) {
		putchar("0123456789ABCDEF"[d[i]]);
	}
}

