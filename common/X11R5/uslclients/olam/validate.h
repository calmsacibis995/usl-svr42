/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:validate.h	1.3"
#endif

/*
** validate.h - This file contains declarations for the functions provided
** by "validate.c".
*/


#ifndef _OLAM_VALIDATE_H
#define _OLAM_VALIDATE_H


void	ValidateNoSpace();		/* Text field validation callback */
void    ValidateNoSpaceOrColon();	/* Text field validation callback */

/*
** This function does all of the work for the above two.
** A message is written to `footer' if `string' contains any illegal
** characters, is too long, or is empty.  Illegal characters are considered
** to be any of the following: any characters in `bad_chars' or
** ILLEGAL_CHARS (from "config.h"), white-space characters, and unprintable
** characters.
** 1 is returned if `string' is valid; 0 is returned otherwise.
*/
int	ValidateString();


#endif	/* _OLAM_VALIDATE_H */
