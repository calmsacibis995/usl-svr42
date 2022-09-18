/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)spell:hashmake.c	1.2.1.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/spell/hashmake.c,v 1.1 91/02/28 20:10:10 ccs Exp $"
#include <stdio.h>
#include "hash.h"

main()
{
	char word[30];
	long h;
	hashinit();
	while(fgets(word,sizeof(word),stdin)) {
		word[strlen(word)-1] = '\0';
		printf("%.*lo\n",(HASHWIDTH+2)/3,hash(word));
	}
	return(0);
}
