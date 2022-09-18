/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:footer.h	1.3"
#endif

/*
** footer.h - This file contains declarations for the routines provided by
** "footer.c".
*/


#ifndef _OLAM_FOOTER_H
#define _OLAM_FOOTER_H


/*
** Clear message in `footer'.  `footer' is expected to be a static text widget
*/
void	ClearFooter();
/*
  Widget	footer;
*/


/*
** Write message into `footer' using printf(3)-style template (`tmpl') and
** an optional string `str' that may be referenced in `tmpl'.  `footer' is
** expected to be static text widget.
** The resulting message can be at most MAXLINE chars.
*/
void	FooterMsg();
/*
  Widget        footer;
  char          *tmpl;
  char          *str;
*/


#endif	/* _OLAM_FOOTER_H */
