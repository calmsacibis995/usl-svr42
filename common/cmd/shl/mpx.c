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

#ident	"@(#)shl:mpx.c	1.7.8.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/shl/mpx.c,v 1.1 91/02/28 20:09:32 ccs Exp $"
#include	"defs.h"
#include	"unistd.h"
#include	"sys/stropts.h"

open_cntl_chan()
{
	register int fd;
	register char p;

	while ((fd = open(cntlf, O_RDWR | O_EXCL)) == SYSERROR)
	{
		if (errno == EBUSY)
		{
			chan += MAX_LAYERS;
			set_dev(chan);
		}
		else
		{
			pfmt(stderr, MM_ERROR, 
				":34:No control channels available: %s\n",
				strerror (errno));
			return(SYSERROR);
		}
	}


	{	register int j;

		for (j=0; j < MAX_LAYERS; ++j)
		{
			set_dev(chan + j);
			if (chown(cntlf, uid, gid) != SYSERROR)
			{
				chmod(cntlf, 0622);
			}
			else
			{
				pfmt(stderr, MM_ERROR, badchown, cntlf, strerror (errno));
				close(fd);
				return(SYSERROR);
			}
		}

	}

	return(fd);
}

int cook;		/* returned from I_LINK ioctl */
multiplex()
{
	extern int real_tty_fd;

	if ((cook = ioctl(cntl_chan_fd, I_LINK, real_tty_fd)) == SYSERROR)
	{
		switch (errno)
		{
			case ENXIO:
				pfmt(stderr, MM_ERROR, 
					":35:SXT driver not configured: %s\n",
					strerror (errno));
				break;

			case ENOMEM:
				pfmt(stderr, MM_ERROR, 
					":36:No memory for kernel configuration: %s\n",
					strerror (errno));
				break;

			case ENOTTY:
				pfmt(stderr, MM_ERROR, 
					":37:Not using a tty device: %s\n",
					strerror (errno));
				break;

			case EAGAIN:
				pfmt(stderr, MM_ERROR, 
					":38:Out of streams buffers: %s\n",
					strerror (errno));
				break;

			default:
				pfmt(stderr, MM_ERROR, 
					":39:Multiplex failed: %s\n", 
					strerror (errno));
		}

		return(0);
	}

	return(1);
}

close_cntl_chan()
{
	register int j;

	for (j=0; j < MAX_LAYERS; ++j)
	{
		set_dev(chan + j);
		if (chown(cntlf, ROOT_UID, SYS_GID) != SYSERROR)
		{
			chmod(cntlf, 0620);
		}
		else
		{
			pfmt(stderr, MM_ERROR, badchown, cntlf, strerror(errno));
			return(SYSERROR);
		}
	}
	return(0);
}
