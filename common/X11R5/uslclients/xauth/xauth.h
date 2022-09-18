/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xauth:xauth.h	1.1"
/*
 * $XConsortium: xauth.h,v 1.11 89/01/03 11:40:06 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xauth.h>

#ifndef True
typedef int Bool;
#define False 0
#define True 1
#endif

extern char *ProgramName;

extern char *copystring(), *malloc(), *realloc();
int process_command(), auth_initialize(), auth_finalize();
extern int print_help();
extern int verbose;
extern Bool ignore_locks;
extern Bool break_locks;
