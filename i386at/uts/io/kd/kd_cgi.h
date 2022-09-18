/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_KD_KD_CGI_H	/* wrapper symbol for kernel use */
#define _IO_KD_KD_CGI_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/kd/kd_cgi.h	1.2"
#ident	"$Header: $"

/*
 * Structure for listing valid
 * adapter IO addresses
 */
struct portrange {
	ushort first;		/* first port */
	ushort count;		/* number of valid right after 'first' */
};

#define	BLACK		0x0
#define	BLUE		0x1
#define	GREEN		0x2
#define	CYAN		0x3
#define	RED		0x4
#define	MAGENTA		0x5
#define	BROWN		0x6
#define	WHITE		0x7
#define	GRAY		0x8
#define	LT_BLUE		0x9
#define	LT_GREEN	0xA
#define	LT_CYAN		0xB
#define	LT_RED		0xC
#define	LT_MAGENTA	0xD
#define	YELLOW		0xE
#define	HI_WHITE	0xF

struct cgi_class
{
	char   *name;
	char   *text;
	long	base;
	long	size;
	struct portrange *ports;
};

#endif	/* _IO_KD_KD_CGI_H */
