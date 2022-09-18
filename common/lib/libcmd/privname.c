/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libcmd:common/lib/libcmd/privname.c	1.2.2.3"

/*
 * These routines are useful for programs that work with privileges.
*/

#include	<string.h>
#include	<sys/types.h>
#include	<sys/privilege.h>

extern	char	*strcpy();
extern	int	strcmp();

struct	privdef {
	const char	*name;
	int	number;
};

const static	struct	privdef	pdefs[] = {
	{	"allprivs",	P_ALLPRIVS	},
	{	"audit",	P_AUDIT		},
	{	"auditwr",	P_AUDITWR	},
	{	"compat",	P_COMPAT	},
	{	"dacread",	P_DACREAD	},
	{	"dacwrite",	P_DACWRITE	},
	{	"dev",		P_DEV     	},
	{	"driver",	P_DRIVER	},
	{	"filesys",	P_FILESYS	},
	{	"fsysrange",	P_FSYSRANGE	},
	{	"macread",	P_MACREAD	},
	{	"macwrite",	P_MACWRITE	},
	{	"macupgrade",	P_MACUPGRADE	},
	{	"mount",	P_MOUNT		},
	{	"multidir",	P_MULTIDIR	},
	{	"owner",	P_OWNER		},
	{	"plock",	P_PLOCK		},
	{	"rtime",	P_RTIME		},
	{	"setflevel",	P_SETFLEVEL	},
	{	"setplevel",	P_SETPLEVEL	},
	{	"setspriv",	P_SETSPRIV	},
	{	"setuid",	P_SETUID	},
	{	"setupriv",	P_SETUPRIV	},
	{	"sysops",	P_SYSOPS	},
	{	"tshar",	P_TSHAR		},
	{	"core",		P_CORE		},
	{	"loadmod",	P_LOADMOD	},
};

#define	NPRVS	(sizeof(pdefs)/sizeof(struct privdef))

/*
 * privname():
 * Lookup the privilege numbered 'p' and copy the name of that privilege
 * into 'buf'.  Return a pointer to 'buf'.
 * If there is no privilege 'p' defined, return a NULL pointer.
*/
char	*
privname(buf, p)
	char	*buf;
	int	p;
{
	register int	i;

	for(i = 0; i < NPRVS; ++i) {
		if (pdefs[i].number == p) {
			return strcpy(buf, pdefs[i].name);
		}
	}
	*buf = 0;
	return (char *)0;
}

/*
 * privnum():
 * Lookup the privilege named 'name' in the list of privilege names and
 * return the privilege number for that name.
 * If the privilege is not found, return -1.
*/
int
privnum(name)
	char	*name;
{
	register int	i;

	if (!name)
		return -1;
	for(i = 0; i < NPRVS; ++i){
		if (!strcmp(name, pdefs[i].name)) {
			return pdefs[i].number;
		}
	}
	return -1;
}
