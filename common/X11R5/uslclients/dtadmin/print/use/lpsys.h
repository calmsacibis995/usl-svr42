/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/lpsys.h	1.2"
#endif

#ifndef LPSYS_H
#define LPSYS_H

#include <lp.h>
#include <requests.h>

typedef struct {
    char	*id;
    char	*user;
    long	size;
    long	date;
    short	outcome;
    char	*printer;
    char	*form;
    char	*character_set;
    long	level;
} PrintJob;

extern PrintJob	*LpJobs (char *);
extern REQUEST	*LpInquire (char *);
extern int	LpChangeRequest (char *, REQUEST *);
extern char	*LpAllocFiles (int);
extern int	LpRequest (char *, char **, long *);
extern int	LpCancel (char *);

#endif LPSYS_H
