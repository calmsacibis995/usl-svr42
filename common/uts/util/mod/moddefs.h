/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODDEFS_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODDEFS_H	/* subject to change without notice */

#ident	"@(#)uts-comm:util/mod/moddefs.h	1.9"
#ident	"$Header: $"

#ifdef _KERNEL

#define MODREV	10

struct mod_operations {
	int	(*modm_install)();
	int	(*modm_remove)();
	int	(*modm_info)();
	void	(*modm_lock)();
	void	(*modm_unlock)();
};

extern struct mod_operations mod_drv_ops;
extern struct mod_operations mod_str_ops;
extern struct mod_operations mod_fs_ops;


struct modlink {
	struct	mod_operations	*ml_ops;
	void	*ml_type_data;
};

/*
 * Module type specific linkage structure.
 */

struct mod_type_data	{
	char 	*mtd_info;
	void	*mtd_pdata;
};

struct modwrapper {
	int	mw_rev;
	int	(*mw_load)();
	int	(*mw_unload)();
	void	(*mw_halt)();
	void	*mw_conf_data;
	struct	modlink	*mw_modlink;
};

#define	_STR(a)		#a
#define	_XSTR(a)	_STR(a)
#define	_WEAK(p, s)	asm(_XSTR(.weak p##s));

#define	MOD_DRV_WRAPPER(p, l, u, h, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_drvdata; \
		extern	void	p##_attach_info; \
		extern	struct	mod_operations	mod_drv_ops; \
		_WEAK(, mod_drv_ops) \
		_WEAK(, mod_drvattach) \
		_WEAK(, mod_drvdetach) \
		_WEAK(p, _mod_drvdata) \
		_WEAK(p, _conf_data) \
		_WEAK(p, _attach_info) \
		static	struct	mod_type_data	p##_drv_link = { \
			n, &p##_mod_drvdata \
		}; \
		static	struct	modlink	p##_mod_link[] = { \
			{ &mod_drv_ops, &p##_drv_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, h, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define MOD_HDRV_WRAPPER(p, l, u, h, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_attach_info; \
		extern	struct	mod_operations	mod_misc_ops; \
		_WEAK(, mod_misc_ops) \
		_WEAK(, mod_drvattach) \
		_WEAK(, mod_drvdetach) \
		_WEAK(p, _conf_data) \
		_WEAK(p, _attach_info) \
		static	struct	mod_type_data	p##_misc_link = { \
			n, NULL \
		}; \
		static 	struct	modlink p##_mod_link[] = { \
			{ &mod_misc_ops, &p##_misc_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, h, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define	MOD_STR_WRAPPER(p, l, u, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_strdata; \
		extern	struct	mod_operations	mod_str_ops; \
		_WEAK(, mod_str_ops) \
		_WEAK(p, _mod_strdata) \
		_WEAK(p, _conf_data) \
		static	struct	mod_type_data	p##_str_link = { \
			n, &p##_mod_strdata \
		}; \
		static	struct	modlink	p##_mod_link[] = { \
			{ &mod_str_ops, &p##_str_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, \
			(void (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define	MOD_FS_WRAPPER(p, l, u, n)	\
		extern	void	*p##_conf_data;	\
		extern	void	p##_mod_fsdata; \
		extern	struct	mod_operations	mod_fs_ops; \
		_WEAK(, mod_fs_ops) \
		_WEAK(p, _mod_fsdata) \
		_WEAK(p, _conf_data) \
		static	struct	mod_type_data	p##_fs_link = { \
			n, &p##_mod_fsdata \
		}; \
		static	struct	modlink	p##_mod_link[] = { \
			{ &mod_fs_ops, &p##_fs_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, \
			(void (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#define MOD_MISC_WRAPPER(p, l, u, n)	\
		extern	void	*p##_conf_data;	\
		extern	struct	mod_operations	mod_misc_ops; \
		_WEAK(p, _conf_data) \
		_WEAK(, mod_misc_ops) \
		static	struct	mod_type_data	p##_misc_link = { \
			n, NULL \
		}; \
		static 	struct	modlink p##_mod_link[] = { \
			{ &mod_misc_ops, &p##_misc_link }, \
			{ NULL, NULL } \
		}; \
		struct	modwrapper	p##_wrapper = { \
			MODREV, l, u, \
			(void (*)())0, \
			&p##_conf_data, \
			p##_mod_link \
		}

#endif	/*_KERNEL */
#endif /* _UTIL_MOD_MODDEFS_H */
