/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/config.c	1.4.6.3"
#ident "@(#)config.c	1.6 'attmail mail(1) command'"
#include "libmail.h"
/*
 * These are configurable parameters for system aliases
 */
const char libdir[] = "/etc/mail";
const char sysalias[] = "/etc/mail/namefiles";
const char useralias[] = "/lib/names";
const char maildir[] = MAILDIR;		/* directory for mail files */
const char mailsave[] = SAVEDIR;	/* dir for save files */
const char mailfwrd[] = FWRDDIR;	/* dir for forward files */
const char spoolsmtpq[] = SPOOLSMTPQ;	/* dir for smtp files */
