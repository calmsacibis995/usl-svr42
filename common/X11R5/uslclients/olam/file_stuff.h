/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:file_stuff.h	1.3"
#endif

/*
** file_stuff.h - This file contains macro definitions and function
** definitions for "file_stuff.c".
*/


#ifndef _OLAM_FILE_STUFF_H
#define _OLAM_FILE_STUFF_H


/*
** Masks for `stat_flags' argument to OpenFile()
*/
#define	FILE_EXISTS	1
#define FILE_READABLE	2
#define FILE_WRITEABLE	4


int	ExtractDisp();			/* Used by GetLine() to extract */
					/* fields from the display */
					/* (/usr/X/lib/Xconnections) file */
int	ExtractHost();			/* Used by GetLine() to extract a */
					/* host name from the host */
					/* (/etc/X0.hosts) file */
void	FormatDispEntry();		/* Formats display fields */
char	*GetLine();			/* Get an interesting string from a */
					/* file */
FILE	*OpenFile();			/* Open file for reading */
int	ParseDispEntry();		/* Parses display line into separate */
					/* fields */


#endif	/* _OLAM_FILE_STUFF_H */
