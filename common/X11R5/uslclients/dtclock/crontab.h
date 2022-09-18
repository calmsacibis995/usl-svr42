/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:crontab.h	1.2"
#endif

/*
 * crontab.h
 *
 */

#ifndef _crontab_h
#define _crontab_h

extern int AddCrontabEntry(char *);
extern int DeleteCrontabEntry(char *);
extern int ReplaceCrontabEntry(char *, char *);

#endif /* _crontab_h */
