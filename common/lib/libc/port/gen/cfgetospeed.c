/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/cfgetospeed.c	1.2"

#ifdef __STDC__
	#pragma weak cfgetospeed = _cfgetospeed
#else
#define const
#endif
#include "synonyms.h"
#include <sys/termios.h>

/*
 * returns output baud rate stored in c_cflag pointed by termios_p
 */

speed_t cfgetospeed(termios_p)
const struct termios *termios_p;
{
	return (termios_p->c_cflag & CBAUD);
}
