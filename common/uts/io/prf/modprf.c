/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*          copyright       "%c%"   */

#ident	"@(#)uts-comm:io/prf/modprf.c	1.2"
#ident	"$Header: $"

#include <util/debug.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <mem/faultcatch.h>
#include <proc/obj/elf.h>
#include <proc/obj/elftypes.h>
#include <mem/kmem.h>
#include <proc/user.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>
#include <io/prf/prf.h>

extern struct module *mod_obj_kern;	/* from mod_obj.c */




int
mod_prf(struct mprf **retprf)
{
	struct modctl *modp;
	unsigned int mcount;
	struct mprf *rpp;
	unsigned int namespcnt;
	unsigned int saveoff;
	char *namesp;

	if(mod_obj_kern == NULL)
		return(ENOLOAD);
	/* turn profiling off */
	if(retprf == NULL) {
		/*get lock on modlist */
		for(modp = modhead.mod_next; modp != &modhead; modp = modp->mod_next)
			modp->mod_flags ^= MOD_PRF;

		/* release lock */
		modhead.mod_flags ^= MOD_PRF;
		return(0);
	}

	/* cannot turn profiling on again without turning it off first */
	if(modhead.mod_flags & MOD_PRF) {
		return(EBUSY);
	}

	/* turn profiling on */

	mcount = 1;
	namespcnt = strlen(mod_obj_kern->md_path) +1;

	/* get lock on modlist */
	for(modp = modhead.mod_next; modp != &modhead; modp = modp->mod_next) {
		if(modp->mod_flags & MOD_TRANS)
			continue;
		modp->mod_flags |= MOD_PRF;
		mcount++;
		namespcnt += (strlen(((struct module *) modp->mod_mp)->md_path)+ 1);
	}
	modhead.mod_flags |= MOD_PRF;
	/* release lock */

	rpp = *retprf = (struct mprf *) kmem_alloc(
	  (mcount+1) * sizeof(struct mprf)
	  + namespcnt, KM_SLEEP);
	namesp = (char *) rpp + (mcount+1) * sizeof(struct mprf);

	rpp->mprf_addr = mcount;
	rpp->mprf_offset = namespcnt;
	rpp++;

	rpp->mprf_addr = (unsigned long) mod_obj_kern->md_space;
	rpp->mprf_offset = 0;
	strcpy(namesp,mod_obj_kern->md_path);
	saveoff = strlen(namesp) + 1;

	for(modp = modhead.mod_next; modp != &modhead; modp = modp->mod_next) {
		if(!(modp->mod_flags & MOD_PRF)) 
			continue;
		rpp++;
		rpp->mprf_addr = (unsigned long) ((struct module *) modp->mod_mp)->md_space;
		rpp->mprf_offset = saveoff;
		strcpy(namesp+saveoff,((struct module *) modp->mod_mp)->md_path);
		saveoff += strlen(((struct module *) modp->mod_mp)->md_path) + 1;
	}

	return(0);
}

