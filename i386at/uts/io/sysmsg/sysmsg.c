/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/sysmsg/sysmsg.c	1.7"
#ident	"$Header: $"


/*
** Non DSG sysmsg driver.
*/

#include <svc/errno.h>
#include <util/param.h>
#include <util/types.h>
#include <proc/signal.h>
#include <mem/immu.h>
#include <proc/user.h>
#include <io/conf.h>
#include <util/sysmacros.h>
#include <io/sysmsg/sysmsg.h>
#include <io/cram/cram.h>
#include <io/termio.h>
#include <svc/bootinfo.h>
#include <svc/sysi86.h>
#include <io/stream.h>
#include <io/termio.h>
#include <io/asy/asy.h>

int smsgputchar();
int smsggetchar();
int smsg_check_cmos_baud();
unsigned char CMOSread();

extern struct smsg_flags smsg_flags;

int	smsg_have_cmos;
int	smsg_init = 0;

extern struct conssw conssw;

int smsgdevflag = D_OLD;

smsgopen(dev)
dev_t dev;
{
	if (minor(dev) != 0)
	{
		return (ENXIO);
	}
}

smsgwrite()
{
	unsigned char c;
	while (u.u_count) {
		if (copyin((char *)u.u_base, &c, 1)) {
			return (EFAULT);
		}
		u.u_count--;
		u.u_base++;
		(*conssw.co)(c, conssw.co_dev);
	}
}

smsgioctl(dev, cmd, arg)
dev_t dev;
int cmd;
struct smsg_flags *arg;
{
	u.u_error = EFAULT;
	return (EFAULT);
}

int
smsgputchar(c)
unsigned char c;
{
	(*conssw.co)(c, conssw.co_dev);
}

int
smsggetchar()
{
	return ((*conssw.ci)(conssw.co_dev));
	
}

int
smsg_check_bios()
{
	if ((bootinfo.id[0] == 'I') &&
	    (bootinfo.id[1] == 'D') &&
	    (bootinfo.id[2] == 'N') &&
	    (bootinfo.id[3] == 'O'))
	{
		if ((bootinfo.id[4] == C2) ||
		    (bootinfo.id[4] == C3) ||
		    (bootinfo.id[4] == C4))
			return(1);
	}
	/* not a machine with console byte in the BIOS */
	return(0);
}

int
smsg_check_cmos_baud(port, default_baud)
int port, default_baud;
{
	return(default_baud);
}

smsg_program_asy_port(port)
int port;
{
}
