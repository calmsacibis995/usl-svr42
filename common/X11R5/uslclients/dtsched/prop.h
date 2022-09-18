/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtsched:prop.h	1.2"
#endif

/*
 * prop.h
 *
 */

#ifndef _prop_h
#define _prop_h

extern void   CreatePropertyWindow(Widget w, int i);
extern void   CreateInputPropertyWindow(Widget w, char * task);
extern char * BasenameOf(char * command);
extern char * DayOrDate(char * month, char * date, char * day);
extern char * TimeOf(char * hour, char * min);

#endif
