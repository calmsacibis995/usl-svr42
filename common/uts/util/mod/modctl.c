/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/mod/modctl.c	1.28"
#ident  "$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/vmsystm.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <acc/priv/privilege.h>
#include <acc/audit/audit.h>

#include <util/mod/mod.h>
#include <util/mod/mod_k.h>
#include <util/mod/moddefs.h>
#include <util/mod/mod_obj.h>
#include <util/mod/ksym.h>

/*
 * Check for circular dependencies.
 * If we are the process already loading this module, then
 * we're trying to load a dependent that we're already loading which
 * means we got a circular dependency.
 */
#define MOD_CIRCDEP(mp) \
	((mp)->mod_inprogress_thread != NULL && \
	    (mp)->mod_inprogress_thread == CUR_CONTEXT)

#define TIMEOUT(mp)	((mp)->mod_unload_time + (mp)->mod_delay_tick <= lbolt)

static int last_module_id;

/* rwlock_t modlist_lock; */

/* head of the struct modctl link list */
struct modctl modhead = {
	&modhead,
	&modhead,
	NULL, NULL, 
	0
};

/* head of the unloadable modules link list */
struct modctl modunload = {
	NULL, NULL, 
	&modunload,
	&modunload,
	0
};

STATIC const char *getmodname(const char *);
STATIC int modunld(struct modctl *);
STATIC void free_modp(struct modctl *);
STATIC int mod_install_stubs(struct modctl *);

/*
 * Structure for modload.
 */
struct modlda {
	char *pathname;
};

/*
 * int modload(struct modlda *uap, rval_t *rvp)
 *	System call to demand load a loadable module.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errno returned directly by this routine is:
 *
 *		EPERM	if the user does not have the P_LOADMOD privilege
 *
 *	The module id of the successfully loaded module is
 *	returned as rvp->r_val1. The module loaded by this system call
 *	will have the flag MOD_DEMAND set so it won't be able to be
 *	auto unload directly.
 */
int
modload(struct modlda *uap, rval_t *rvp)
{
	int retval;
	struct modctl *modp;
	char *pathname = NULL;

	if (pm_denied(u.u_cred, P_LOADMOD)) {
		retval = EPERM;
		goto out;
	}

	pathname = kmem_zalloc(MAXPATHLEN, KM_SLEEP);
	if (retval = copyinstr(uap->pathname, pathname, MAXPATHLEN-1, 0)) {
		pathname = NULL;
		goto out;
	}

	if ((retval = modld(pathname, u.u_cred, &modp)) != 0)
		goto out;

	/* avoid autounload */
	modp->mod_flags |= MOD_DEMAND;

	rvp->r_val1 = modp->mod_id;
	/* UNLOCK(&modp->mod_lock, ipl); */
	retval = 0;

out:
	if(audit_on)
		if(retval == 0)
			adt_modload(ADT_MOD_DEMAND, modp->mod_id, retval, MOD_OBJ_GETMODPATH(modp->mod_mp));
		else
			adt_modload(ADT_MOD_DEMAND, 0, retval, pathname);
	if(pathname != NULL)
		kmem_free(pathname, MAXPATHLEN);
	return(retval);
}

/*
 * Structure for moduload.
 */
struct modulda {
	int mod_id;
};

/*
 * int moduload(struct modulda *uap, rval_t *rvp)
 *	System call to demand unload a loadable module or all the
 *	unloadable modules.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *
 *		EPERM	if the user does not have the P_LOADMOD privilege
 *		EINVAL	if the specified module id is not existing
 *		EBUSY	if the specified module id is not unloadable
 *
 *	The flag MOD_DEMAND, if is set, will be turned off even if 
 *	the demand unloading failed, so the specified module(s) can 
 *	be auto unloaded later.
 */
int
moduload(struct modulda *uap, rval_t *rvp)
{
	struct modctl *modp;
	int retval;

	if (pm_denied(u.u_cred, P_LOADMOD)) {
		retval = EPERM;
		goto uload_out;
	}
	/* if no module is loaded, return immediately */

	/* oipl = RW_RDLOCK(&modlist_lock, ipl); */
	if (modhead.mod_next == &modhead) {
		/* RW_UNLOCK(&modlist_lock, oipl); */
		retval = EINVAL;
		goto uload_out;
	}
	/* RW_UNLOCK(&modlist_lock, oipl); */

	if (uap->mod_id == 0)
		if(unload_modules(B_FALSE, 0) == 0) {
			/* Make audit call only if unsuccessful. Successful unloads
			   are reported from within unload_modules */
			if(audit_on)
				adt_moduload(ADT_MOD_DEMAND,0,EBUSY);
			return(EBUSY);
		}
		else
			return(0);

	/* RW_RDLOCK(&modlist_lock, ipl); */
	for (modp = modhead.mod_next; modp != &modhead; modp = modp->mod_next)
		if (modp->mod_id == uap->mod_id)
			break;

	if (modp == &modhead) {
		/* RW_UNLOCK(&modlist_lock, oipl); */
		retval = EINVAL;
		goto uload_out;
	}

	/* LOCK(&modp->mod_lock, ipl); */
	/* RW_UNLOCK(&modlist_lock, oipl); */


	/* turn off demand load flag if it is set */
	modp->mod_flags &= ~MOD_DEMAND;
	if ((modp->mod_flags & MOD_TRANS) ||
		MODBUSY(modp)) {
		/* UNLOCK(&modp->mod_lock, oipl); */
		retval = EBUSY;
		goto uload_out;

	} else {
		modp->mod_flags |= MOD_UNLOADING;
		/* UNLOCK(&modp->mod_lock, oipl); */
	}
		
	retval = modunld(modp);
uload_out:
	if(audit_on)
		adt_moduload(ADT_MOD_DEMAND, uap->mod_id, retval);
	return(retval);
}

/*
 * Structure for modstat.
 */
struct modstata {
	int mod_id;
	struct modstatus *stbuf;
	boolean_t get_next_mod;
};

/*
 * int modstat(struct modstata *uap, rval_t *rvp)
 *	System call to get information of a loadable module.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *
 *		EPERM	if the user does not have the P_LOADMOD privilege
 *		EINVAL	if the specified module id is not existing and
 *			get_next_mod is set to B_FALSE; or if there is
 *			no module id greater than the module id specified
 *			when get_next_mod is set to B_TRUE.
 */
int
modstat(struct modstata *uap, rval_t *rvp)
{
	struct modstatus *modstatusp;
	struct modctl *modp;
	int retval;

	if (pm_denied(u.u_cred, P_LOADMOD))
		return(EPERM);

	modstatusp = (struct modstatus *)kmem_zalloc(sizeof(struct modstatus), 
		KM_SLEEP);

	/* oipl = RW_RDLOCK(&modlist_lock, ipl); */
	for (modp = modhead.mod_next; modp != &modhead; modp = modp->mod_next) {
		if (modp->mod_id == uap->mod_id)
			break;
		if (uap->get_next_mod && (modp->mod_id > uap->mod_id))
			break;
	}
	if (modp == &modhead || (modp->mod_flags & MOD_LOADING)) {
		/* RW_UNLOCK(&modlist_lock, oipl); */
		retval = EINVAL;
		goto out3;
	}
	/* oipl2 = LOCK(&modp->mod_lock, ipl); */
	/* RW_UNLOCK(&modlist_lock, oipl); */
			
	modstatusp->ms_id = modp->mod_id;
	modstatusp->ms_unload_delay = modp->mod_delay_tick / HZ;
	modstatusp->ms_refcnt = modp->mod_refcnt;
	modstatusp->ms_depcnt = modp->mod_depcnt;

	mod_obj_getmodstatus(modp->mod_mp, modstatusp);

	if((retval = mod_info(modp, modstatusp)) != 0)
		goto out2;

	retval = copyout((caddr_t)modstatusp, (caddr_t)uap->stbuf,
			sizeof(struct modstatus));

out2:
	/* UNLOCK(&modp->mod_lock, oipl2); */
out3:
	kmem_free(modstatusp, sizeof(struct modstatus));
	return(retval);
}

/*
 * int modld(const char *module_name, const cred_t *credentials, 
 *		struct modctl **retmodp)
 *
 *	Common kernel interface for autoload and demand load of a 
 *	loadable module.
 *
 * Calling/Exit State:
 *	The module_name argument gives a module name or a full path 
 *	of a module. The credentials argument gives a credential 
 *	struct of the calling process. The retmodp argument is 
 *	used for returning a pointer to the pointer of modctl 
 *	structure of the loaded module.
 *
 *	No locks should be held upon entry.
 *
 *	If the loading process succeed or the module is already 
 *	loaded, the retmodp argument will be set to a pointer to 
 *	the struct modctl of the specified module with the mod_lock 
 *	of the module held. The calling process is responsible 
 *	for releasing the lock.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *
 *		EINVAL	if circular dependency is detected
 *		EBADVER	if the version number between the kernel loadable
 *			module mechanism and the wrapper of the module to
 *			be loaded does not match		
 */
int
modld(const char *module_name, const cred_t *credentials, struct modctl **retmodp)
{
	extern	int	mod_initialized;

	struct modctl *modctlp, *modp;
	const char *modname;
	int retval;
	int (*func)();

	modname = getmodname(module_name);

	if(!mod_initialized)	{
		cmn_err(CE_WARN,
		   "Attempt to load module: %s, prior to DLM initialization",
		   modname);

		retval = ENOSYS;
		goto errout3;
	}

	/*
	 * Check to see if the module is statically configured.
	 */
	if(mod_static(modname))	{
		retval = EINVAL;
		goto errout3;
	}

	modp = (struct modctl *)kmem_zalloc(sizeof (*modp), KM_SLEEP);

	modp->mod_name = kmem_zalloc(strlen(modname) + 1, KM_SLEEP);
	strcpy(modp->mod_name, modname);

	/* LOCK_INIT(modp->mod_lock, hier, min_ipl, lkinfop, KM_SLEEP); */
	modp->mod_flags |= MOD_LOADING;
	modp->mod_inprogress_thread = CUR_CONTEXT;

	/* check the existence of the requested module */
again:
	/* oipl = RW_WRLOCK(&modlist_lock, ipl); */
	for (modctlp = modhead.mod_next; modctlp != &modhead; 
		modctlp = modctlp->mod_next)
		if (strcmp(modctlp->mod_name, modp->mod_name) == 0) {
			/* oipl2 = LOCK(&modctlp->mod_lock, ipl); */
			/* RW_UNLOCK(&modlist_lock, oipl); */

			/* Check for circular dependency. */
			if (MOD_CIRCDEP(modctlp)) {
				/* UNLOCK(&modctlp->mod_lock, oipl2); */
				cmn_err(CE_NOTE, "!MOD: Circular dependency in module %s.", modctlp->mod_name);
				retval = EINVAL;
				goto errout;
			}

			if (modctlp->mod_flags & MOD_TRANS) {
				/* SV_WAIT(&modctlp->mod_sv, PZERO, &modctlp->mod_lock); */
				sleep(&modctlp->mod_sv, PZERO);
				goto again;
			} else {
				free_modp(modp);
				/* return with modctlp->mod_lock held */
				*retmodp = modctlp;
				goto goodout;
			}
		
		}

	/* add to the end of the modhead list */
	modp->mod_next = &modhead;
	modp->mod_prev = modhead.mod_prev;
	modhead.mod_prev->mod_next = modp;
	modhead.mod_prev = modp;

	modp->mod_id = ++last_module_id;

	/* RW_UNLOCK(&modlist_lock, oipl); */

	moddebug(cmn_err(CE_NOTE, "!MOD: loading %s, module id %d.\n", 
		module_name, modp->mod_id));

	if ((retval = mod_obj_load(module_name, credentials, 
		(struct module **)&modp->mod_mp)) != 0) {
		moddebug(cmn_err(CE_NOTE, "!MOD: error loading %s, program is %s\n", 
			module_name, u.u_comm));
		goto errout4;
	}
	modp->mod_flags |= MOD_SYMTABOK;

	/* set up the wrapper information */
	modp->mod_mdw = MOD_OBJ_GETMODWRAP(modp->mod_mp);
	if(modp->mod_mdw->mw_rev != MODREV) {
		cmn_err(CE_NOTE, "!MOD: %s: revision number does not match",
			modp->mod_name);
		retval = EBADVER;
		goto errout2;
	}
	modp->mod_delay_tick = (clock_t)*(time_t *)modp->mod_mdw->mw_conf_data * HZ;

	/* call the _load() entry point in the module if the routine exists;
	   if something failed in _load(), it should free up all the
	   resource it allocated before return */
	
	func = (int (*)())(*modp->mod_mdw->mw_load);
	if (func != NULL) {
		retval = (*func)();	/* call _load */

		moddebug(cmn_err(CE_NOTE, "!MOD: Returned from _load, retval = %x\n", 
			retval));

		if (retval != 0) {
			moddebug(cmn_err(CE_NOTE, 
				"!MOD: modld:%s(%s): _load returned error %d.\n",
				module_name, modp->mod_name, retval));
			goto errout2;
		}
	}

	if ((retval = mod_connect(modp)) != 0) {
		moddebug(cmn_err(CE_NOTE, 
			"!MOD: modld:%s(%s): mod_connect returned error %d.\n",
			module_name, modp->mod_name, retval));
		/* undo whatever done in _load() */
		if ((func = (int (*)())(*modp->mod_mdw->mw_unload)) != NULL) {
			(void)(*func)();
		}
		goto errout2;
	}

	/*
	 * install stubs if any
	 */
	if ((retval = mod_install_stubs(modp)) != 0) {
		modp->mod_flags &= ~MOD_LOADING;
		modp->mod_flags |= MOD_UNLOADING;
		if (modunld(modp) != 0)
			cmn_err(CE_PANIC,
			"MOD: can't install stub functions for the module %s",
				modp->mod_name);
		*retmodp = NULL;
		goto errout3;
	}

	/* success, tell anyone waiting for this module */
	/* LOCK(&modp->mod_lock, ipl); */
	modp->mod_inprogress_thread = NULL;
	modp->mod_flags &= ~MOD_LOADING;
	/* SV_BROADCAST(&modp->mod_sv, CL_PRMPT); */
	wakeup(&modp->mod_sv);
	/* return with the modp->mod_lock held */
	*retmodp = modp;
goodout:
	/* make auditing call if autoload,
	   if demand load, the call is made in modload */
	if(audit_on && credentials == sys_cred)
		adt_modload(ADT_MOD_AUTO,(*retmodp)->mod_id,0,MOD_OBJ_GETMODPATH((*retmodp)->mod_mp));
	return (0);

errout2:
	/* free up the memory allocated for the module */
	modp->mod_flags &= ~MOD_SYMTABOK;
	mod_obj_unload(modp->mod_mp);
errout4:
	/* oipl = RW_WRLOCK(&modlist_lock, ipl); */
	modp->mod_next->mod_prev = modp->mod_prev;
	modp->mod_prev->mod_next = modp->mod_next;
	if(modp->mod_n_uld != NULL)
		RECOVER(modp);
	/* RW_UNLOCK(&modlist_lock, oipl); */
	/* SV_BROADCAST(modp->mod_sv, CL_PRMPT); */
	wakeup(&modp->mod_sv);
errout:
	free_modp(modp);
errout3:
	*retmodp = NULL;
	/* make the auditing call here in autoload case.  It is made
	   in modload for the demand load case */
	if(audit_on && credentials == sys_cred)
		adt_modload(ADT_MOD_AUTO,0,retval, (char *) module_name);
	return (retval);
}


/*
 * static const char *getmodname(const char *module_name)
 *	Find the module name from a full path.
 *
 * Calling/Exit State:
 *	Return a pointer to the module name.	
 */
STATIC const char *
getmodname(const char *module_name)
{
	int i;

	for (i = strlen(module_name) - 1; i >= 0 && module_name[i] != '/'; i--);

	if (i >= 0)
		return(&module_name[++i]);
	else
		return(module_name);
}

/*
 * int unload_modules(boolean_t autounload, int highmem)
 *	Unload loadable modules that are on the unload list.
 *	Common interface for auto unload and demand unload.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	The autounload argument indicate the calling process
 *	is auto unload or demand unload. The highmem argument
 *	is used by auto unload as a high water mark for freemem.
 *	The demand unload will ignore the argument.
 *	The auto unload will stop and return 1 when freemem >= highmem.
 *
 *	The demand unload will return the number of modules unloaded.
 *	The auto unload will return 0 if freemem < highmem at the end
 *	of the routine.
 */
int
unload_modules(boolean_t autounload, int highmem)
{
	struct modctl *modp;
	int n, saveid;

	n = 0;
again1:
	/* oipl = RW_RDLOCK(&modlist_lock, ipl); */
	if (autounload)
		modp = modunload.mod_n_uld;
	else
		modp = modhead.mod_next;

	while (modp != &modunload && modp != &modhead) {

		/* oipl2 = LOCK(&modp->mod_lock, ipl); */

		/* turn off demand load flag if it is demand unload */
		if (!autounload)
			modp->mod_flags &= ~MOD_DEMAND;

		if ((modp->mod_flags & MOD_UNLOADING) 
			|| (autounload && !TIMEOUT(modp))) {
			/* UNLOCK(&modp->mod_lock, oipl2); */
			moddebug(cmn_err(CE_NOTE, 
				"!MOD: unload_modules(): skip module %s", 
				modp->mod_name));
		} else {

			/* RW_UNLOCK(&modlist_lock, oipl); */
			modp->mod_flags |= MOD_UNLOADING;
			/* UNLOCK(&modp->mod_lock, oipl2); */
			saveid = modp->mod_id;
			if (modunld(modp) == 0) {
				if (autounload) {
					moddebug(cmn_err(CE_NOTE, 
						"!MOD: freemem=0x%x\thighmem=0x%x",
						freemem, highmem));
					if (freemem >= highmem)
						return(1);
				} else
					n++;
				if(audit_on)
					adt_moduload(autounload ? ADT_MOD_AUTO : ADT_MOD_DEMAND,
						saveid, 0);
				goto again1;
			} else {
				cmn_err(CE_NOTE, "!MOD: can't unload module %s", 
					modp->mod_name);
				/* oipl = RW_RDLOCK(&modlist_lock, ipl); */
			}
		}
		if (autounload)
			modp = modp->mod_n_uld;
		else
			modp = modp->mod_next;
	}

	/* RW_UNLOCK(&modlist_lock, oipl); */
	return(n);
}

/*
 * static int modunld(struct modctl *modp)
 *	Common kernel interface for auto unload and demand unload of a 
 *	loadable module.
 *
 * Calling/Exit State:
 *	The modp argument gives a pointer to the modctl structure
 *	of the module to be unloaded.
 *
 *	The routine is called with the MOD_UNLOADING flag set for
 *	the module which modp points to.
 *	If the module unloaded successfully, the routine returns 0;
 *	otherwise the routine returns a valid errno and unset the 
 *	MOD_UNLOADING flag.
 *
 *	If this routine is called by demand unload, the flag MOD_DEMAND should
 *	be unset already in the moduload() before entering this routine. 
 *	In case the demand unload failed, the module will be available for
 *	auto unload.
 *	So, if the MOD_DEMAND flag is still set when entering this routine 
 *	it must be auto unloading a demand loaded module, the routine will 
 *	return EBUSY.
 */
STATIC int
modunld(struct modctl *modp)
{
	int retval;
	int (*func)();

	/* LOCK(&modp->mod_lock, ipl); */
	if (modp->mod_flags & MOD_DEMAND) {
		modp->mod_flags &= ~MOD_UNLOADING;
		/* UNLOCK(&modp->mod_lock, ipl); */
		return(EBUSY);
	}
	/* UNLOCK(&modp->mod_lock, ipl); */

	if ((retval = mod_disconn(modp)) != 0) {
		/* LOCK(&modp->mod_lock, ipl); */
		modp->mod_flags &= ~MOD_UNLOADING;
		/* UNLOCK(&modp->mod_lock, ipl); */
		return (retval);
	}

	if ((func = modp->mod_mdw->mw_unload) != NULL) {

		/* call _unload */
		if ((retval = (*func)(modp)) != 0) {
			/* try to restore the switch tables if the _unload() failed */
			if (mod_connect(modp) != 0)
				cmn_err(CE_PANIC, "MOD: can't reconnect the module %s back to the kernel", modp->mod_name);
			/* LOCK(&modp->mod_lock, ipl); */
			modp->mod_flags &= ~MOD_UNLOADING;
			/* UNLOCK(&modp->mod_lock, ipl); */
			return (retval);
		}
	}

	moddebug(cmn_err(CE_NOTE, "!MOD: unloading %s, module id %d.\n",
		    modp->mod_name, modp->mod_id));

	modp->mod_flags &= ~MOD_SYMTABOK;
	mod_obj_unload(modp->mod_mp); /* and finally free the memory */

	/* oipl = RW_WRLOCK(&modlist_lock, ipl); */
	modp->mod_prev->mod_next = modp->mod_next;
	modp->mod_next->mod_prev = modp->mod_prev;
	if (modp->mod_n_uld != NULL) {
		modp->mod_p_uld->mod_n_uld = modp->mod_n_uld;
		modp->mod_n_uld->mod_p_uld = modp->mod_p_uld;
	}
	/* RW_UNLOCK(&modlist_lock, oipl); */
	/* SV_BROADCAST(modp->mod_sv, CL_PRMPT); */
	wakeup(&modp->mod_sv);
	free_modp(modp);
	return (0);
}


/*
 * static void free_modp(struct modctl *modp)
 *	Free the memory allocated for the modctl related data structures.
 *
 * Calling/Exit State:
 *	Always success, no return value.
 */
STATIC void
free_modp(struct modctl *modp)
{
	if (modp->mod_name)
		kmem_free(modp->mod_name, strlen(modp->mod_name) + 1);
	kmem_free(modp, sizeof(*modp));
}

/*
 * int mod_connect(struct modctl *modp)
 *	Connect a module to the running kernel.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 */
int
mod_connect(struct modctl *modp)
{
	register int retval = 0;
	register struct modlink *linkp, *linkp1;

	linkp = modp->mod_mdw->mw_modlink;

	while (linkp->ml_ops != NULL) {
		/* MODL_LOCK(linkp); */
		if ((retval = MODL_INSTALL(linkp, modp)) != 0) {

			/* if one of the connection failed,
			   remove all the installed linkage */
			/* MODL_UNLOCK(linkp); */
			linkp1 = modp->mod_mdw->mw_modlink;

			while (linkp1 != linkp) {
				/* MODL_LOCK(linkp1); */
				/* MODL_REMOVE should not fail */
				MODL_REMOVE(linkp1); /* clean up */
				/* MODL_UNLOCK(linkp1); */
				linkp1++;
			}
			break;
		}
		/* MODL_UNLOCK(linkp); */
		linkp++;
	}

	return (retval);
}

/*
 * int mod_disconn(struct modctl *modp)
 *	Disconnect a module from the running kernel.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 * Remarks: 
 *	The hierarchy value of mod_swt_lock and locks for all the switch
 *	tables should be the same, since we can't force a special order
 *	in ml_linkage array.
 */
int
mod_disconn(struct modctl *modp)
{
	register int retval;
	register struct modlink *linkp;

	/* acquire the mod_swt_lock to avoid dead lock when there are more
	   than one unload process try to acquire switch table locks */
	/* oipl = LOCK_SH(&mod_swt_lock, ipl); */

	/* loop through ml_linkage to invoke MODL_LOCK to lock switch tables */
	/*
	linkp = modp->mod_mdw->mw_modlink;
	while (linkp->ml_ops != NULL) {
		MODL_LOCK(linkp);
		linkp++;
	}
	*/

	/* UNLOCK(&mod_swt_lock, oipl); */

	/* oipl = LOCK(&modp->mod_lock, ipl); */

	if (MODBUSY(modp)) {
		retval = EBUSY;
		goto out5;
	}

	linkp = modp->mod_mdw->mw_modlink;

	retval = 0;
	while (linkp->ml_ops != NULL) {
		if ((retval = MODL_REMOVE(linkp)) != 0)
			break;
		linkp++;
	}

out5:
	/* UNLOCK(&modp->mod_lock, oipl); */

	/* loop through ml_linkage to invoke MODL_UNLOCK to unlock switch tables */
	/*
	linkp = modp->mod_mdw->mw_modlink;
	while (linkp->ml_ops != NULL) {
		MODL_UNLOCK(linkp);
		linkp++;
	}
	*/

	return (retval);
}

/*
 * int mod_info(struct modctl *modp, struct modstatus *modstatusp)
 *	Get module status.
 *
 * Calling/Exit State:
 *	The mod_lock of the specified module is held upon entry and exit.
 */
int
mod_info(struct modctl *modp, struct modstatus *modstatusp)
{
	register int i;
	register int retval = 0;
	register struct modspecific_stat *mssp;
	register struct modlink *linkp;

	modstatusp->ms_rev = modp->mod_mdw->mw_rev;

	linkp = modp->mod_mdw->mw_modlink;
	mssp = &modstatusp->ms_msinfo[0];

	for (i = 0; i < MODMAXLINK; i++) {
		if (linkp->ml_ops == NULL)
			break;
		strncpy(mssp->mss_linkinfo, ((struct mod_type_data *)linkp->ml_type_data)->mtd_info,
		    MODMAXLINKINFOLEN);
		if((retval = MODL_INFO(linkp, mssp->mss_p0, mssp->mss_p1, &mssp->mss_type)) != 0)
			return(retval);
		linkp++;
		mssp++;
	}
	return (0);
}

/*
 * int mod_stub_load(struct mod_stub_info *stub)
 *	Install a loadable module through the stub function.
 *
 * Calling/Exit State:
 *	No locks held upon entry and exit.
 *
 *	The system will panic if the load failed and there is no
 *	error function specified for the stub function.
 */
int
mod_stub_load(struct mod_stub_info *stub)
{
	struct modctl *modp;

	if (modld(stub->mods_modinfo->modm_module_name, sys_cred, &modp) == 0) {
		/* currently, a stub loaded module is not unloadable */
		modp->mod_refcnt++;
		/* UNLOCK(&modp->mod_lock, ipl); */
		return(0);
	}

	if (stub->mods_errfcn == NULL)
		cmn_err(CE_PANIC, "MOD: install_stub: can't load module %s",
			stub->mods_modinfo->modm_module_name);
	else
		return(1);
}

/*
 * static int mod_install_stubs(struct modctl *modp)
 *	Replace all the stub functions in the module with real functions.
 *
 * Calling/Exit State:
 *	No locks held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errno returned directly by this routine is:
 *
 *		ERELOC	if any stub function is not properly defined
 *
 * Remarks:
 *	The module auto loaded through a well defined interface should
 *	not have any stub function.
 */
STATIC int
mod_install_stubs(struct modctl *modp)
{
	struct mod_stub_modinfo *mp;
	struct mod_stub_info *sp;
	unsigned long offset, funcadr;
	char *funcname;
	char namebuf[MODMAXNAMELEN + 9];
	int i;
	char fnamebuf[MAXSYMNMLEN];

	strcpy(namebuf, modp->mod_name);
	strcat(namebuf, "_modinfo");

	if ((mp = (struct mod_stub_modinfo *)mod_obj_getsymvalue(namebuf, 
		B_FALSE, B_TRUE)) == 0)
		return(0);

	for (i = 0, sp = mp->modm_stubs; sp->mods_func_adr; i++, sp++) {
		funcname = (caddr_t)mod_obj_getsymname(
			(unsigned long)sp->mods_stub_adr, &offset, B_TRUE,fnamebuf);
		if (funcname == NULL) {
			cmn_err(CE_NOTE, 
		"!MOD: mod_install_stubs: can't find symbol name for 0x%x",
				sp->mods_stub_adr);
			return(ERELOC);
		}
		funcadr = mod_obj_lookup((struct module *)modp->mod_mp, 
			funcname);
		if (funcadr == 0) {
			cmn_err(CE_NOTE, 
		"!MOD: mod_install_stubs: %s() not defined properly in module %s",
				funcname, modp->mod_name);
			return(ERELOC);
		}
		sp->mods_func_adr = (caddr_t)funcadr;
	}
	return(0);
}

/*
 * mod_static() returns 1 if the module whose name
 * is pointed to by m is statically configured in
 * the running kernel, 0 otherwise.
 */
int
mod_static(const char *m)
{
	register const	char	*mp;
	register char	*smp, **sm;
	extern	char	*static_modules[];

	sm = static_modules;

	while((smp = *sm) != NULL)	{
		mp = m;

		while(*smp == *mp)	{
			if(*smp == '\0')	{
				return(1);
			}
			smp++;
			mp++;
		}
		sm++;
	}

	return(0);
}

/*
 * modhalt() is called on system shutdown. It loops through
 * the modhead list calling the halt(D2D) routines of currently
 * loaded modules.
 */
void
modhalt()
{
	struct	modctl	*modp;
	void	(*func)();

	moddebug(cmn_err(CE_NOTE, "MOD: modhalt()"));

	for(modp = modhead.mod_next; modp != &modhead; modp = modp->mod_next)  {
		moddebug(cmn_err(CE_NOTE, "MOD: %s, halt = 0x%x",
			modp->mod_name, modp->mod_mdw->mw_halt));
		if((func = modp->mod_mdw->mw_halt) != NULL)	{
			moddebug(cmn_err(CE_NOTE, "MOD:    halt called"));
			(*func)();
		}
	}
}

/*
 * int mod_zero(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return 0.
 */
int
mod_zero(void)
{
	return(0);
}

/*
 * int mod_minus(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return -1.
 */
int
mod_minus(void)
{
	return(-1);
}

/*
 * int mod_einval(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return EINVAL.
 */
int
mod_einval()
{
	return(EINVAL);
}

/*
 * int mod_enoload(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return ENOLOAD.
 */
int
mod_enoload()
{
	return(ENOLOAD);
}
