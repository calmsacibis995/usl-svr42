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

#ident	"@(#)uts-x86at:boot/at386/gets.c	1.2"
#ident	"$Header: $"

#include "util/types.h"
#include "boot/boot.h"

/*
 * bgets():	Sort of like gets(), but not quite.
 *		Read from the console (using getchar) into string str,
 *		until a carriage return or until n-1 characters are read.
 *		Null terminate the string, and return.
 *		This all is made complicated by the fact that we must
 *		do our own echoing during input.
 *		N.B.: Returns the *number of characters in str*.
 */

int
bgets( str, n )
char	*str;
int	n;
{
	int 	c;
	int	t;
	char	*p;

	p = str;
	c = 0;

	while ( (t = getchar()) != '\r' ) {
		putchar(t);
		if ( t == '\b' ) {
			if ( c ) {
				printf(" \b");
				c--; p--;
			} else
				putchar(' ');
			continue;
		}
		if (c < n - 1) {
			*p++ = t; 
			c++;
		}
	}
	*p = '\0';

	return(c);
}


/*
 * bfgets():	Sort of like fgets(), but not quite.
 *		Read data from the open inode starting at offset, 
 *		stopping when a	newline is encounted, or n-1 characters 
 *		have been read, or EOF is reached. The string is then
 *		null terminated. 
 *		N.B.: Returns the *number of characters in str*.
 */
 
int
bfgets( str, n, offset)
char	*str;
int	n;
int	offset;
{
	unsigned long	count, i;
	unsigned long	stat;
	extern off_t	disk_file_offset;

	disk_file_offset = offset;

	BL_file_read( str, 0, n, &count, &stat );

	if ( count == 0 )
		return(0);

	for ( i = 0; i < count; i++ )
		if ( str[i] == '\n' ) {
			str[i] = '\0';
			break;
		}

	if ( i >= count )
		str[count] = '\0';

	return(i);
}
