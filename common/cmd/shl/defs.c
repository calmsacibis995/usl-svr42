/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)shl:defs.c	1.5.7.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/shl/defs.c,v 1.1 91/02/28 20:09:24 ccs Exp $"

#include	"defs.h"

struct layer 	*layers[MAX_LAYERS];

int max_index = 0;
int chan = 0;
int fildes[2];
int real_tty_fd;
	
char *cntlf;
char cntl_fl[]	= "/dev/sxt/000";

int	cntl_chan_fd;
	
struct utmp	  *u_entry;
struct termio ttysave;

uid_t uid;
gid_t gid;

const char
	badswitch[] = ":7:Switch failed: %s\n",
	resuming[] = ":8:Resuming %s\n",
	badchown[] = ":9:chown() on %s failed: %s\n",
	badioctl[] = ":10:Virtual tty ioctl failed: %s\n";
