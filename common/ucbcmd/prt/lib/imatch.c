/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucbcmd/prt/lib/imatch.c	1.1"
#ident	"$Header: $"
/*      Portions Copyright (c) 1988, Sun Microsystems, Inc.     */ 
/*      All Rights Reserved.                                    */ 
 

/*
	initial match
	if `prefix' is a prefix of `string' return 1
	else return 0
*/

int
imatch(prefix,string)
register char *prefix, *string;
{
	while (*prefix++ == *string++)
		if (*prefix == 0)
			return(1);
	return(0);
}
