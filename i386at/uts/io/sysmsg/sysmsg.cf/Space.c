/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/sysmsg/sysmsg.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/termio.h>
#include <sys/sysmsg.h>
#include <sys/conf.h>

struct smsg_flags smsg_flags = 
{
	CMF,	/* console message flag - tunable */
	CMF,	/* dynamic console message flag - tunable */
	RCMF,	/* remote console message flag - tunable */
	RCMF,	/* dynamic remote console message flag - tunable */
	0,	/* default for current alternate console setting */
	1,	/* default for remote console setting */
	0,	/* default for CMOS alternate console setting */
	B9600,	/* default alternate console baud rate */
	B1200	/* default remote console baud rate */
};

int com2cons = COM2CONS;

extern int kdputchar(), kdgetchar();
extern int asyputchar(), asygetchar();
extern int asyputchar2(), asygetchar2();

struct conssw conssw =
{
	kdputchar,
	0,
	kdgetchar
};
