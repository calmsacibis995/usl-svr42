/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/count.c	1.2.3.2"
#ident  "$Header: count.c 2.0 91/07/12 $"

#include <stdio.h>

FILE *fp=stdin;

char *strchr();

main(argc, argv)
int argc;
char **argv;
{
	int count = 0;
	char entry[BUFSIZ];
	char *eptr=entry;
	char *ptr;

	fgets(entry, BUFSIZ, fp);
	while((ptr=strchr(eptr, ':')) != NULL) {
		eptr=++ptr;
		count++;
	}
	printf("%d\n",count);

}
