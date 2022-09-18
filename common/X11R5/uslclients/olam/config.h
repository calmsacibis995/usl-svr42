/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:config.h	1.7"
#endif

/*
** config.h - This file contains macro definitions that are used all over.
*/


#ifndef _OLAM_CONFIG_H
#define _OLAM_CONFIG_H


#define AVERAGE_BEEP_VOLUME	0	/* Beep volume to use in call to */
					/* XBell() if no value has been */
					/* supplied in the application */
					/* resources */
#define DISPFIELDLEN		14	/* Length for one field in Outgoing */
					/* Remote Displays */
#define DISPFILE		"/usr/X/lib/Xconnections"
/*
** Allow DISPHELPFILE to be defined at run time
*/
#ifndef DISPHELPFILE
#define DISPHELPFILE	"display"
#endif
/* #define DISPHELPTAG		"Outgoing Remote Displays" */
#define DISPVIEWHEIGHT		5	/* Height (in entries) of scrolling */
					/* list for Outgoing Remote Displays */
#define DISPVIEWWIDTH		(3 * DISPFIELDLEN + 2)
					/* Width (in characters) of */
					/* scrolling list for Outgoing */
					/* Remote Displays */
#define HOSTFILE		"/etc/X0.hosts"
#define HOSTVIEWHEIGHT		5	/* Height (in entries) of scrolling */
					/* list for Accepted Remote Hosts */
#define HOSTVIEWWIDTH		14	/* Width (in characters) of */
					/* scrolling list for Accepted */
					/* Remote Hosts */
/*
** Allow HOSTHELPFILE to be defined at compile time
*/
#ifndef HOSTHELPFILE
#define HOSTHELPFILE	"host_name"
#endif
#define HOSTHELPTAG		"Accepted Remote Hosts"
#define ILLEGAL_CHARS		"#"	/* Chars. that are illegal for all */
					/* of the text fields */
#define LISTFONT		"fixed"	/* Font for scrolling list items; */
					/* must be fixed-width so that */
					/* Outgoing Remote Displays fields */
					/* line up */
#define MAXLINE			1024	/* Magic number that controls the */
					/* size of various */
					/* statically-allocated char. arrays */
#define MAXPATHLEN		1024	/* Max. path len. for `file_name' */
					/* member of CommonStuff struct */
#define MAXMSG			1024	/* Not used @ */


#endif	/* _OLAM_CONFIG_H */
