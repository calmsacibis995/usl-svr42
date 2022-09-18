/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cfgintf:common/cmd/cfgintf/summary/maxcol.c	1.1.4.2"
#ident "$Header: maxcol.c 2.0 91/07/11 $"

#include <stdio.h>

main(argc,argv)
int argc;
char **argv;
{
	int maxcol = 0;
	int linecol = 0;
 	int c;
	while ((c = getchar()) != EOF) {
	   if (c == '\n') {
	      if (linecol > maxcol)
		 maxcol = linecol;
	      linecol = 0;
	      continue;
	   }
	   linecol++;
	}
	printf("%d\n", maxcol);
}
