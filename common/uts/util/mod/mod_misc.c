/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/mod/mod_misc.c	1.2"
#ident	"$Header: $"
#include <util/debug.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod.h>
#include <util/mod/moddefs.h>

STATIC int mod_miscdonothing();
STATIC int mod_miscinfo(struct mod_type_data *p, int *p0, int *p1, int *type);

struct	mod_operations	mod_misc_ops	= {
	mod_miscdonothing,
	mod_miscdonothing,
	mod_miscinfo
};

int
mod_misc_adm(unsigned int command, void * arg)
{
	return(0); /* nothing to register */
}


STATIC int
mod_miscdonothing()
{
	return(0);
}

STATIC int 
mod_miscinfo(struct mod_type_data *p, int *p0, int *p1, int *type)
{
	*type = MOD_TY_MISC;
	return(0);
}
