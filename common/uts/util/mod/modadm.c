/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/mod/modadm.c	1.8"
#ident	"$Header: $"

#include <util/debug.h>
#include <util/types.h>
#include <util/param.h>
#include <proc/user.h>
#include <proc/cred.h>
#include <acc/priv/privilege.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/mod/mod.h>
#include <util/mod/mod_k.h>

#define MOD_ADMSWSZ	7

STATIC int (* mod_admsw[MOD_ADMSWSZ])() = {
	mod_none_adm,
	mod_cdev_adm,
	mod_bdev_adm,
	mod_str_adm,
	mod_fs_adm,
	mod_sdev_adm,
	mod_misc_adm
};


int
modadm_c(unsigned int type, unsigned int command, void *arg, boolean_t sysspace)
{
	if(type > MOD_ADMSWSZ)
		return(EINVAL);
	return(mod_admsw[type](command, arg,sysspace));
}

struct madma {
	unsigned int type;
	unsigned int command;
	void *arg;
};

int
mod_none_adm(unsigned int command, void * arg)
{
	return(EINVAL);
}

int
modadm(const struct madma *uap, rval_t *rvp)
{

	int error;

	if(pm_denied(u.u_cred, P_LOADMOD)) {
		rvp->r_val1 = -1;
		return(EPERM);
	}
	
	if((error = modadm_c(uap->type,uap->command,uap->arg, B_FALSE)) != 0) {
		rvp->r_val1 = -1;
		return(error);
	}
	rvp->r_val1 = 0;
	return(0);
}

