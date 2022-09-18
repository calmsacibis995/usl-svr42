/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_K_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_K_H	/* subject to change without notice */

#ident	"@(#)uts-comm:util/mod/mod_k.h	1.14"
#ident	"$Header: $"

/* 
   This file is available only for use by crash(1M).  The contents
   are subject to change without notice.
*/
#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h>
#endif

#ifndef _PROC_CRED_H
#include <proc/cred.h>
#endif

#ifndef _UTIL_MOD_MOD_H
#include <util/mod/mod.h>
#endif

#ifndef _UTIL_MOD_MODDEFS_H
#include <util/mod/moddefs.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cred.h>
#include <sys/mod.h>
#include <sys/moddefs.h>

#else

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cred.h>
#include <sys/mod.h>
#include <sys/moddefs.h>

#endif /* _KERNEL_HEADERS */

typedef char	sv_t;

struct modctl {
	struct modctl *mod_next, *mod_prev;	/* loaded module list pointers */
	struct modctl *mod_n_uld, *mod_p_uld;	/* unload list pointers */
	unsigned mod_id;			/* module id */
	void *mod_mp;				/* pointer to relocation info */
	void *mod_inprogress_thread;		/* pointer to loading process */
	struct modwrapper *mod_mdw;		/* pointer to the wrapper */
	char *mod_name;				/* module name */
	unsigned short mod_flags;		/* flags */
	int mod_depcnt;				/* number of loaded dependents */
	int mod_refcnt;				/* module reference count */
	clock_t mod_unload_time;		/* the lbolt value when the last
						   reference is completed */
	clock_t mod_delay_tick;			/* minimum number of ticks before
						   module can be auto-unloaded */
	sv_t mod_sv;				/* synchronization variable */
	lock_t mod_lock;			/* module lock (MP only) */
};

/* list of pointers to modctl structures used to maintain dependent list */
struct modctl_list {
	struct modctl *mcl_mcp;
	struct modctl_list *mcl_next;
};

/* bit masks for mod_flags */
#define	MOD_PRF		0x1
#define	MOD_LOADING	0x2
#define	MOD_UNLOADING	0x4
#define	MOD_DEMAND	0x8
#define	MOD_SYMTABOK	0x10
#define MOD_TRANS	(MOD_LOADING | MOD_UNLOADING)

#ifdef _KERNEL
#define MODBUSY(mcp)	(mcp->mod_depcnt || mcp->mod_refcnt || mcp->mod_flags & MOD_PRF)

#define RECOVER(mp)	{ \
				(mp)->mod_p_uld->mod_n_uld = (mp)->mod_n_uld; \
				(mp)->mod_n_uld->mod_p_uld = (mp)->mod_p_uld; \
				(mp)->mod_p_uld = (mp)->mod_n_uld = NULL; \
			}

#define	MOD_HOLD(mcp)	{ if((mcp)->mod_refcnt++ == 0 && (mcp)->mod_n_uld != NULL) \
				RECOVER(mcp) \
			  moddebug(cmn_err(CE_NOTE, "!MOD: MOD_HOLD(): RC = %d", (mcp)->mod_refcnt)); }

#define UNLOAD(mp)	{ \
				(mp)->mod_unload_time = lbolt; \
				UNLOAD_LIST(mp); \
			}

#define UNLOAD_LIST(mp) { \
				(mp)->mod_n_uld = &modunload; \
				(mp)->mod_p_uld = modunload.mod_p_uld; \
				modunload.mod_p_uld->mod_n_uld = (mp); \
				modunload.mod_p_uld = (mp); \
			}

#define	MOD_RELE(mcp)	{ if(--((mcp)->mod_refcnt) == 0 && (mcp)->mod_depcnt == 0) \
				UNLOAD(mcp) \
			moddebug(cmn_err(CE_NOTE, "!MOD: MOD_RELE(): RC = %d", (mcp)->mod_refcnt)); }

extern struct modctl modhead;
extern struct modctl modunload;

/* data structures for stubs mechanism */
struct mod_stub_info {
	caddr_t mods_func_adr;
	struct mod_stub_modinfo *mods_modinfo;
	caddr_t mods_stub_adr;
	int (*mods_errfcn)();
};

struct mod_stub_modinfo {
	char *modm_module_name;
	struct mod_stub_info modm_stubs[1];	/* variable length */
};


#define MOD_DEFPATH	"/etc/conf/mod.d"
#define CUR_CONTEXT	((void *)u.u_procp)

extern int modld(const char *, const cred_t *, struct modctl **);
extern int unload_modules(boolean_t, int);

extern int mod_none_adm(unsigned int, void *);
extern int mod_cdev_adm(unsigned int, void *);
extern int mod_bdev_adm(unsigned int, void *);
extern int mod_str_adm(unsigned int, void *);
extern int mod_fs_adm(unsigned int, void *);
extern int mod_sdev_adm(unsigned int, void *);
extern int mod_misc_adm(unsigned int, void *);

/* tunable and definitions */
#define MOD_NOPAGE	0
#define MOD_PAGE	1
#define MOD_FPAGE	2
extern int mod_obj_pagesymtab;

extern struct module * mod_obj_kern;
extern unsigned int mod_obj_size;


/* macros to vector to the appropriate module specific routine */
#define	MODL_INSTALL(MODL, MODP) \
	(*(MODL)->ml_ops->modm_install)((MODL)->ml_type_data, MODP)
#define	MODL_REMOVE(MODL) \
	(*(MODL)->ml_ops->modm_remove)((MODL)->ml_type_data)
#define	MODL_INFO(MODL, P0, P1, TYPE)	\
	(*(MODL)->ml_ops->modm_info)((MODL)->ml_type_data, P0, P1, TYPE)
#define	MODL_LOCK(MODL) \
	(*(MODL)->ml_ops->modm_lock)(MODL)
#define	MODL_UNLOCK(MODL) \
	(*(MODL)->ml_ops->modm_unlock)(MODL)

/* For internal debug purpose only. */
#ifdef MODDEBUG
#define moddebug(x)	x
#else
#define moddebug(x)	/* undef */
#endif

#endif /* _KERNEL */
#endif /* _UTIL_MOD_MOD_K_H */
