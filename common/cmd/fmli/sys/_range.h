/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fmli:sys/_range.h	1.1"
static int
valid_range(c1, c2)
wchar_t c1, c2;
{
	wchar_t mask;
	if(MB_CUR_MAX > 3 || eucw1 > 2)
		mask = EUCMASK;
	else
		mask = H_EUCMASK;
	return (c1 & mask) == (c2 & mask) && 
	(c1 > 0377 || !iscntrl(c1)) && 
	(c2 > 0377 || !iscntrl(c2));
}
