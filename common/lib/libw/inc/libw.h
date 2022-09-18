/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:inc/libw.h	1.1.2.2"
#ident  "$Header: libw.h 1.2 91/06/26 $"

#include	<stdlib.h>
#include	<sys/euc.h>

#ifdef __STDC__
void getwidth(eucwidth_t *);
int mbftowc(char *, wchar_t *, int (*)(), int *);
int scrwidth(wchar_t);
int wisprint(wchar_t);
#else
void getwidth();
int mbftowc();
int scrwidth();
int wisprint();
#endif /* __STDC__ */
