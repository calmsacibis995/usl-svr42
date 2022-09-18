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

#ident	"@(#)shl:stat.c	1.4.5.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/shl/stat.c,v 1.1 91/02/28 20:09:39 ccs Exp $"
#include	"defs.h"

static struct sxtblock status;

all_layers(flag)
	int flag;
{
	int i;

	status.input = 0;
	status.output = 0;

	if (ioctl(cntl_chan_fd, SXTIOCSTAT, &status) == SYSERROR)
		pfmt(stderr, MM_WARNING,
			":48:ioctl() SXTIOCSTAT failed: %s\n",
			strerror (errno));

	for (i = 1; i <= max_index; ++i)
		if (layers[i])
			stat_layer(i, flag);
}


one_layer(name, flag)
	char *name;
	int flag;
{
	int i;

	if (i = lookup_layer(name))
		stat_layer(i, flag);
}


stat_layer(i, flag)
	int i;
	int flag;
{
	pfmt(stdout, MM_INFO|MM_NOGET, "%s (%d)", layers[i]->name, layers[i]->p_grp);

	if (status.input & (1 << i))
		pfmt(stdout, MM_NOSTD, ":49: blocked on input\n");
	else if (status.output & (1 << i))
		pfmt(stdout, MM_NOSTD, ":50: blocked on output\n");
	else
		pfmt(stdout, MM_NOSTD, ":51: executing or awaiting input\n");

	fflush(stdout);

	if (flag)
	{
		char buff[50];

		sprintf(buff, "%d", layers[i]->p_grp);
		system(buff);
		printf("\n");
	}
}
