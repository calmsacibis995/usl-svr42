/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_OBJ_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_OBJ_H	/* subject to change without notice */

#ident	"@(#)uts-comm:util/mod/mod_obj.h	1.9"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>
#endif

#ifndef _PROC_CRED_H
#include <proc/cred.h>
#endif

#ifndef _PROC_OBJ_ELF_H
#include <proc/obj/elf.h>
#endif

#ifndef _UTIL_MOD_MOD_K_H
#include <util/mod/mod_k.h>
#endif

#ifndef _UTIL_MOD_MOD_H
#include <util/mod/mod.h>
#endif

#ifndef _UTIL_MOD_MODULE_H
#include <util/mod/module.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/cred.h>
#include <sys/elf.h>
#include <sys/mod_k.h>
#include <sys/mod.h>
#include <sys/module.h>

#endif /* _KERNEL_HEADERS */


extern int mod_obj_load(const char *, const cred_t *, struct module**);
extern void mod_obj_unload(struct module *);
extern void mod_obj_getmodstatus(struct module *, struct modstatus *);
extern unsigned long mod_obj_lookup(const struct module *, const char *);

#define MOD_OBJ_GETMODWRAP(x)	((struct modwrapper *)((struct module *)(x))->md_mw)
#define MOD_OBJ_GETMODPATH(x)	(((struct module *)(x))->md_path)


/* Below this line defined in mod_ksym.c in module modksym */
extern struct module *mod_obj_kern;
extern size_t mod_obj_size;

extern void modinit(const char*);
extern boolean_t mod_obj_validaddr(unsigned long);
extern unsigned long mod_obj_getsymvalue(const char *, boolean_t, boolean_t);
extern char *mod_obj_getsymname(unsigned long, unsigned long *, boolean_t, char *);
extern int mod_obj_ioksym(const char *, char *, size_t , boolean_t , int);
extern boolean_t mod_obj_validaddr(unsigned long);

/* The following interfaces are only for use in mod_obj.c */
extern char *mod_obj_getspace(size_t, boolean_t *);
extern int mod_obj_lookupone(const struct module *, const char *, boolean_t, Elf32_Sym *);
extern unsigned long mod_obj_hashname(const char *);

#endif /* _UTIL_MOD_MOD_OBJ_H */
