/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:cs.h	1.1.2.2"
#ident  "$Header: cs.h 1.3 91/06/21 $"

struct csopts {
      struct netconfig *nc_p;
      int nd_opt;
      struct netbuf *nb_p;
};

int
cs_connect(char *, char *, struct csopts *, int *);

void
cs_perror(char *, int);
