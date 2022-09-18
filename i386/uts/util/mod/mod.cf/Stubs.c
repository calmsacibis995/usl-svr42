/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/mod/mod.cf/Stubs.c	1.5"
#ident	"$Header: $"


#include	<config.h>
#include 	<sys/errno.h>
#include	<sys/cred.h>
#include	<sys/cmn_err.h>
#include	<sys/mod_k.h>
#include	<sys/mod_intr.h>

struct modctl modhead = {
	&modhead, &modhead,
	NULL,
	NULL,
	0
};
struct modctl modunload = {
	NULL, NULL,
	&modunload,
	&modunload,
	0
};

int mod_einval() { return(EINVAL); }
int mod_zero() { return(0); }
int mod_enoload() { return(ENOLOAD); }
int
mod_stub_load(struct mod_stub_info *stub)
{

	if (stub->mods_errfcn == NULL)
		cmn_err(CE_PANIC, "MOD: install_stub: can't load module %s",
			stub->mods_modinfo->modm_module_name);
	else
		return(stub->mods_errfcn());
}
const char *mod_fsname() {return(0);}
const char *mod_drvname() { return(0);}
int mod_strld() { return(ENOSYS); }
int unload_modules(boolean_t a, int h) { return(0); }

int modld(const char *a, const cred_t *b, struct modctl **c) {return(ENOSYS);}
void autounload_all() {}

int modstat(){return(ENOSYS);}
int modload(){return(ENOSYS);}
int moduload(){return(ENOSYS);}
int modpath(){return(ENOSYS);}
int modadm(){return(ENOSYS);}

/*
 * For now...
 */
#define	MOD_MAX_INTR	20


struct	mod_shr_v	*mod_shr_ivect[MOD_MAX_INTR];
char	mod_iv_locks[MOD_MAX_INTR];

struct	modctl	*mod_shadowcsw[CDEVSWSZ];
struct	modctl	*mod_shadowbsw[BDEVSWSZ];
struct	modctl	*mod_shadowfsw[FMODSWSZ];
struct	modctl	*mod_shadowvfssw[VFSSWSZ];
