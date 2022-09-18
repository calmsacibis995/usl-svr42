/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_RFS_DU_H	/* wrapper symbol for kernel use */
#define _FS_RFS_DU_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/rfs/du.h	1.6.2.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _FS_RFS_RF_CIRMGR_H
#include <fs/rfs/rf_cirmgr.h>	/* REQUIRED */
#endif

#ifndef _FS_RFS_RF_COMM_H
#include <fs/rfs/rf_comm.h>	/* REQUIRED */
#endif

#ifndef _FS_STAT_H
#include <fs/stat.h>		/* REQUIRED */
#endif

#ifndef _FS_STATFS_H
#include <fs/statfs.h>		/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef _IO_STREAM_H
#include <io/stream.h>		/* REQUIRED */
#endif

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/rf_cirmgr.h>	/* REQUIRED */
#include <sys/rf_comm.h>	/* REQUIRED */
#include <sys/stat.h>		/* REQUIRED */
#include <sys/statfs.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/stream.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Entry points for RFS into the SVR3.X system call protocol
 */

/* Entry points specific to the client */
extern int	du_lookup();
extern int	du_access();
extern int	du_create();
extern int	du_fcntl();
extern int	du_getattr();
extern int	du_mkdir();
extern int	du_open();
extern int	du_remove();
extern int	du_rename();
extern int	du_rmdir();
extern int	du_setattr();
extern int	du_fstatfs();

/* Entry points specific to the server */
extern int	dusr_saccess();
extern int	dusr_chdirec();
extern int	dusr_chmod();
extern int	dusr_iupdate();
extern int	dusr_chown();
extern int	dusr_coredump();
extern int	dusr_creat();
extern int	dusr_exec();
extern int	dusr_fcntl();
extern int	dusr_fstat();
extern int	dusr_fstatfs();
extern int	dusr_link();
extern int	dusr_link1();
extern int	dusr_mkdir();
extern int	dusr_mknod();
extern int	dusr_open();
extern int	dusr_rmdir();
extern int	dusr_rmount();
extern int	dusr_seek();
extern int	dusr_stat();
extern int	dusr_statfs();
extern int	dusr_unlink();
extern int	dusr_utime();

/* --- */


struct exec_ids {
	uid_t ex_uid;
	gid_t ex_gid;
};


/*
 * The ersatz vnode mechanism is added to cases
 * that arise with ES that cannot be handled by stashing
 * information under the current directory and returning a pointer
 * to that given vnode as the out paramter. 
 * (The code path has multiple stash references when audit_on.)
 *
 * This mechanism provides a new, unique vnode that has
 * proper type, etc. and its own ``stash'' of information
 * that is created at lookup time.
 * Minimally, the ersatz vnode stash has stat and vattr information.
 *
 * Unlike dustash, this information will persist until
 * the vnode is de-allocated by rf_inactive().
 */

extern int	du_evn_cnt;	/* external, so rf_inactive() can decrement */

typedef enum evn_cont_types {
	EVN_CONT_EMPTY	= 0x0,
	EVN_CONT_STAT	= 0x1,
	EVN_CONT_VATTR	= 0x2,
	EVN_CONT_STATFS	= 0x4,
	EVN_CONT_EXEC	= 0x8,
	EVN_CONT_VP	= 0x10
} evn_contype_t;

typedef union evn_contents {
	struct statfs	evn_statfs;
	struct exec_ids	evn_exec;
	vnode_t 	*evn_vp;
} evn_cont_t;

typedef struct evn_stash {
	struct stat	evn_stat;
	vattr_t		evn_vattr;
	evn_contype_t	evn_type;
	evn_cont_t	evn_u;
} evn_st_t;

#define EVN_STAT	evn_stat
#define EVN_VATTR	evn_vattr
#define EVN_STATFS	evn_u.evn_statfs
#define EVN_EXEC	evn_u.evn_exec
#define EVN_VP		evn_u.evn_vp

typedef struct ersatz_vnode { /* with additional  infra-structure */
	sndd_t		e_sndd;	/* includes vnode */
	struct queue	e_queue;
	gdp_t		e_gdp;
	evn_st_t	e_stash;
} evn_t;

#define	EVNTOV(p)	(&((p)->e_sndd.sd_vn))
#define	EVNTOSD(p)	(&((p)->e_sndd))
#define	EVNTOQP(p)	((struct queue *)(EVNTOSD(p) + 1))
#define	EVNTOGP(p)	((gdp_t *)	 (EVNTOQP(p) + 1))
#define	EVNTOST(p)	((evn_st_t *)	 (EVNTOGP(p) + 1))

#define VTOEVN(p)	((evn_t *)(p)->v_data)
#define VTOST(p)	EVNTOST(VTOEVN(p))

#endif /* _FS_RFS_DU_H */
