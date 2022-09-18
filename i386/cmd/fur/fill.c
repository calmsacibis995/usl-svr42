/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fur:i386/cmd/fur/fill.c	1.1"

void
#ifdef __STDC__
filltext(char *start, char *end)
#else
filltext(start, end)
char *start;
char *end;
#endif
{
	while( start != end) {
		*(start) = 0x90; /* NOP */
		start++;
	}
}

