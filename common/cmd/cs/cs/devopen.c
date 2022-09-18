/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*              copyright       "%c%"   */

#ident	"@(#)cs:cs/devopen.c	1.1"
#ident	"$Header: $"

#include "uucp.h"
#include <sys/stropts.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/tp.h>

static  size_t tp_errno;
static  char tp_ebuf[80];

int
dev_open(dcname,flags)
char *dcname;
int flags;
{
	int fd;
	struct tp_info	tpinfo;

	if ((fd = tp_open(dcname, flags)) < 0){
		if (errno == EINVAL) {
			/*  Check for EINVAL to determine if 
			 *  the device has a clist driver.
			 *  This check is heuristic. Typically EINVAL
			 *  is returned from tp_open() when it attempts
			 *  to link (i.e. ioctl(I_PLINK) ) the physical 
			 *  device "dcname" under a TP device and "dcname" 
			 *  is not a STREAMS based device. If EINVAL is 
			 *  returned we assume that I_PLINK failed because
			 *  "dcname" is clist based device and open it via 
			 *  a standard open(2) interface.
			 */

			if ((fd = open(dcname,flags)) < 0) {
				DEBUG(5, "connect failed: %s\n", sys_errlist[errno]);
				logent("connect failed", sys_errlist[errno]);
				DEBUG(5, "connect failed, errno %d\n", errno);
				return(-1);
			}
		} else {
			tp_geterror(tp_errno, (size_t)80, tp_ebuf);
			DEBUG(5, "%s\n", tp_ebuf);
			DEBUG(5, "connect failed: %s\n", sys_errlist[errno]);
			logent("connect failed", sys_errlist[errno]);
			DEBUG(5, "connect failed, errno %d\n", errno);
			return(-1);
		}
	}

	return(fd);
}
