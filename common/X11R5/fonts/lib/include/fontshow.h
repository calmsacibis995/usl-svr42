/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontinc:lib/include/fontshow.h	1.4"
/*

 * $XConsortium: fontshow.h,v 1.1 91/05/11 09:12:05 rws Exp $
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#define FONT_SHOW_INFO	    (1<<0)
#define FONT_SHOW_PROPS	    (1<<1)
#define FONT_SHOW_METRICS   (1<<2)
#define FONT_SHOW_GLYPHS    (1<<3)
#define FONT_SHOW_ALL	    (FONT_SHOW_INFO|FONT_SHOW_PROPS|FONT_SHOW_GLYPHS)
