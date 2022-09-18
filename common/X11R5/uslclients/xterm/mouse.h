/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:mouse.h	1.1"
#endif
/*
 mouse.h (C hdr file)
	Acc: 621885034 Fri Sep 15 13:50:34 1989
	Mod: 621885047 Fri Sep 15 13:50:47 1989
	Sta: 621885047 Fri Sep 15 13:50:47 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* definition of mouse bit-masks	*/
#define	BUTTON1_RELEASED	000000000001L
#define	BUTTON1_PRESSED		000000000002L
#define	BUTTON1_CLICKED		000000000004L
#define	BUTTON1_DOUBLE_CLICKED	000000000010L
#define	BUTTON1_TRIPLE_CLICKED	000000000020L
#define	BUTTON2_RELEASED	000000000040L
#define	BUTTON2_PRESSED		000000000100L
#define	BUTTON2_CLICKED		000000000200L
#define	BUTTON2_DOUBLE_CLICKED	000000000400L
#define	BUTTON2_TRIPLE_CLICKED	000000001000L
#define	BUTTON3_RELEASED	000000002000L
#define	BUTTON3_PRESSED		000000004000L
#define	BUTTON3_CLICKED		000000010000L
#define	BUTTON3_DOUBLE_CLICKED	000000020000L
#define	BUTTON3_TRIPLE_CLICKED	000000040000L
#define ALL_MOUSE_EVENTS	000000077777L

#define	Send_mouse_event(buff, len)	write (term->screen.respond, (buff), (len))
