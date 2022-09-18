/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucbcmd/prt/lib/repl.c	1.1"
#ident	"$Header: $"
/*      Portions Copyright (c) 1988, Sun Microsystems, Inc.     */ 
/*      All Rights Reserved.                                    */ 
 
/*
	Replace each occurrence of `old' with `new' in `str'.
	Return `str'.
*/

char *
repl(str,old,new)
char *str;
char old,new;
{
	char	*trnslat();
	return(trnslat(str, &old, &new, str));
}
