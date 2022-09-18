/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/common/stdlib.h	1.2"
/*ident	"@(#)cfront:incl/stdlib.h	1.7"*/


#ifndef STDLIBH
#define STDLIBH

extern int     abort();
extern int     close(int);
extern double  atof (const char*);
extern int     atoi (const char*);
extern long    atol (const char*);
extern void    exit (int);
extern char*   getenv (const char*);
extern int     kill(int, int);
extern int     open(const char *, int, ...);
extern int     rand ();
extern void    srand  (unsigned);
extern double  strtod (const char*, char**);
extern long    strtol (const char*, char**, int);
extern unsigned long  strtoul(const char *, char **, int);
extern int     system (const char*);
extern int     wait(int *);

#endif
