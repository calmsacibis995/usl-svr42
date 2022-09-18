/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_NFS_H	/* wrapper symbol for kernel use */
#define _FS_NFS_NFS_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/nfs/nfs.h	1.7.4.4"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _NET_RPC_TYPES_H
#include <net/rpc/types.h>	/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>		/* REQUIRED */
#endif

#ifndef _ACC_DAC_ACL_H
#include <acc/dac/acl.h>	/* REQUIRED */
#endif

#ifndef _NET_RPC_TOKEN_H
#include <net/rpc/token.h>	/* REQUIRED */
#endif

#ifndef _NET_RPC_XDR_H
#include <net/rpc/xdr.h>	/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>
#endif

#ifndef _FS_NFS_EXPORT_H
#include <fs/nfs/export.h>
#endif

#ifndef _FS_NFS_NFS_CLNT_H
#include <fs/nfs/nfs_clnt.h>
#endif

#ifndef _PROC_CRED_H
#include <proc/cred.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/acl.h>		/* REQUIRED */
#include <rpc/token.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */
#include <sys/vnode.h>
#include <nfs/export.h>
#include <nfs/nfs_clnt.h>
#include <sys/cred.h>

#else

#include <rpc/types.h>
#include <rpc/token.h>

#endif /* _KERNEL_HEADERS */

/* Maximum size of data portion of a remote request */
#define	NFS_MAXDATA	8192
#define	NFS_MAXNAMLEN	255
#define	NFS_MAXPATHLEN	1024

/*
 * Rpc retransmission parameters
 */
#define	NFS_TIMEO	11	/* initial timeout in tenths of a sec. */
#define	NFS_RETRIES	5	/* times to retry request */

/*
 * maximum transfer size for different interfaces
 */
#define	ECTSIZE	2048
#define	IETSIZE	8192

/*
 * Error status
 * Should include all possible net errors.
 * For now we just cast errno into an enum nfsstat.
 */
enum nfsstat {
	NFS_OK = 0,			/* no error */
	NFSERR_PERM=1,			/* Not owner */
	NFSERR_NOENT=2,			/* No such file or directory */
	NFSERR_IO=5,			/* I/O error */
	NFSERR_NXIO=6,			/* No such device or address */
	NFSERR_ACCES=13,		/* Permission denied */
	NFSERR_EXIST=17,		/* File exists */
	NFSERR_NODEV=19,		/* No such device */
	NFSERR_NOTDIR=20,		/* Not a directory */
	NFSERR_ISDIR=21,		/* Is a directory */
	NFSERR_FBIG=27,			/* File too large */
	NFSERR_NOSPC=28,		/* No space left on device */
	NFSERR_ROFS=30,			/* Read-only file system */
	NFSERR_NAMETOOLONG=63,		/* File name too long */
	NFSERR_NOTEMPTY=66,		/* Directory not empty */
	NFSERR_DQUOT=69,		/* Disc quota exceeded */
	NFSERR_STALE=70,		/* Stale NFS file handle */
	NFSERR_WFLUSH			/* write cache flushed */
};
#define	puterrno(error)		((enum nfsstat)error)
/*
 * errno 45 is BSD's EOPNOTSUPP - NFS expects it even though it's not in the
 * protocol. We can probably get away with it since EDEADLK probably won't
 * be returned by an NFS request.
 */
#define	geterrno(status)	((int)status == NFSERR_STALE ? ESTALE : ((int)status == 45 ? EOPNOTSUPP : (int)status))

/*
 * File types
 */
enum nfsftype {
	NFNON,
	NFREG,		/* regular file */
	NFDIR,		/* directory */
	NFBLK,		/* block special */
	NFCHR,		/* character special */
	NFLNK		/* symbolic link */
};

/*
 * Special kludge for fifos (named pipes)  [to adhere to NFS Protocol Spec]
 *
 * VFIFO is not in the protocol spec (VNON will be replaced by VFIFO)
 * so the over-the-wire representation is VCHR with a '-1' device number.
 *
 * NOTE: This kludge becomes unnecessary with the Protocol Revision,
 *       but it may be necessary to support it (backwards compatibility).
 */
#define NFS_FIFO_TYPE	NFCHR
#define NFS_FIFO_MODE	S_IFCHR
#define NFS_FIFO_DEV	((u_long)-1)

/* identify fifo in nfs attributes */
#define NA_ISFIFO(NA)	(((NA)->na_type == NFS_FIFO_TYPE) && \
			    ((NA)->na_rdev == NFS_FIFO_DEV))

/* set fifo in nfs attributes */
#define NA_SETFIFO(NA)	{ \
			(NA)->na_type = NFS_FIFO_TYPE; \
			(NA)->na_rdev = NFS_FIFO_DEV; \
			(NA)->na_mode = ((NA)->na_mode&~S_IFMT)|NFS_FIFO_MODE; \
			}

/*
 * Another kludge for Multi-Level Directories (not in either the v.2 or the ESV
 * protocols. These are represented in a nfsesvsattr struct as a directory with
 * the file size field set to -2 (0xfffffffe), and in a nfsesvfattr struct as
 * a directory with the rdev field set to -2 (0xfffffffe).
 */
#define NFS_MLD_TYPE	((u_long)0xfffffffe)
#define NA_SETMLD(NA)  ((NA)->na_rdev = NFS_MLD_TYPE)
#define NA_TSTMLD(NA)  ((NA)->na_rdev == NFS_MLD_TYPE && (NA)->na_type == NFDIR)
#define SA_SETMLD(SA)  ((SA)->sa_size = NFS_MLD_TYPE)
#define SA_TSTMLD(SA)  ((SA)->sa_size == NFS_MLD_TYPE)

/*
 * Size of an fhandle in bytes
 */
#define	NFS_FHSIZE	32

/*
 * File access handle
 * This structure is the server representation of a file.
 * It is handed out by a server for the client to use in further
 * file transactions.
 */

#ifdef NFSSERVER

#ifdef _KERNEL_HEADERS

#ifndef _FS_VFS_H
#include <fs/vfs.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/vfs.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * This struct is only used to find the size of the data field in the
 * fhandle structure below.
 */
static
struct fhsize {
	fsid_t	f1;
	u_short	f2;
	char	f3[4];
	u_short	f4;
	char	f5[4];
};
#define	NFS_FHMAXDATA	((NFS_FHSIZE - sizeof (struct fhsize) + 8) / 2)

struct svcfh {
	fsid_t	fh_fsid;		/* filesystem id */
	u_short	fh_len;			/* file number length */
	char	fh_data[NFS_FHMAXDATA];	/* and data */
	u_short	fh_xlen;		/* export file number length */
	char	fh_xdata[NFS_FHMAXDATA];/* and data */
};

typedef struct svcfh fhandle_t;
#else
/*
 * This is the client view of an fhandle
 */
typedef struct {
	char	fh_data[NFS_FHSIZE];	/* opaque data */
} fhandle_t;
#endif


/*
 * Arguments to remote write and writecache
 */
struct nfswriteargs {
	fhandle_t	wa_fhandle;	/* handle for file */
	u_long		wa_begoff;	/* beginning byte offset in file */
	u_long		wa_offset;	/* current byte offset in file */
	u_long		wa_totcount;	/* total write cnt (to this offset) */
	u_long		wa_count;	/* size of this write */
	char		*wa_data;	/* data to write (up to NFS_MAXDATA) */
};


/*
 * File attributes
 */
struct nfsfattr {
	enum nfsftype	na_type;	/* file type */
	u_long		na_mode;	/* protection mode bits */
	u_long		na_nlink;	/* # hard links */
	u_long		na_uid;		/* owner user id */
	u_long		na_gid;		/* owner group id */
	u_long		na_size;	/* file size in bytes */
	u_long		na_blocksize;	/* prefered block size */
	u_long		na_rdev;	/* special device # */
	u_long		na_blocks;	/* Kb of disk used by file */
	u_long		na_fsid;	/* device # */
	u_long		na_nodeid;	/* inode # */
	struct timeval	na_atime;	/* time of last access */
	struct timeval	na_mtime;	/* time of last modification */
	struct timeval	na_ctime;	/* time of last change */
};

struct nfsesvfattr {
	enum nfsftype	na_type;	/* file type */
	u_long		na_mode;	/* protection mode bits */
	u_long		na_nlink;	/* # hard links */
	u_long		na_uid;		/* owner user id */
	u_long		na_gid;		/* owner group id */
	u_long		na_size;	/* file size in bytes */
	u_long		na_blocksize;	/* prefered block size */
	u_long		na_rdev;	/* special device # */
	u_long		na_blocks;	/* Kb of disk used by file */
	u_long		na_fsid;	/* device # */
	u_long		na_nodeid;	/* inode # */
	struct timeval	na_atime;	/* time of last access */
	struct timeval	na_mtime;	/* time of last modification */
	struct timeval	na_ctime;	/* time of last change */
	s_token		na_privs;	/* privileges token */
	s_token		na_sens;	/* sensitivity token */
	s_token		na_info;	/* information token */
	s_token		na_integ;	/* integrity token */
	s_token		na_ncs;		/* nationality caveat set token */
	s_token		na_acl;		/* access control list token */
};

#define n2v_type(x)	(NA_ISFIFO(x) ? VFIFO : (enum vtype)((x)->na_type))
#define n2v_rdev(x)	(NA_ISFIFO(x) ? 0 : (x)->na_rdev)

/*
 * Arguments to remote read
 */
struct nfsreadargs {
	fhandle_t	ra_fhandle;	/* handle for file */
	u_long		ra_offset;	/* byte offset in file */
	u_long		ra_count;	/* immediate read count */
	u_long		ra_totcount;	/* total read cnt (from this offset) */
};

/*
 * Status OK portion of remote read reply
 */
struct nfsrrok {
	struct nfsfattr	rrok_attr;	/* attributes, need for pagin */
	u_long		rrok_count;	/* bytes of data */
	char		*rrok_data;	/* data (up to NFS_MAXDATA bytes) */
	char		*rrok_map;	/* pointer to mapped data */
	struct vnode	*rrok_vp;	/* vnode assoc. with mapping */
};

/*
 * Extended status OK portion of remote read reply
 */
struct nfsesvrrok {
	struct nfsesvfattr	rrok_attr;	/* attributes, need for pagin */
	u_long			rrok_count;	/* bytes of data */
	char			*rrok_data;	/* data (up to NFS_MAXDATA bytes) */
	char			*rrok_map;	/* pointer to mapped data */
	struct vnode		*rrok_vp;	/* vnode assoc. with mapping */
};

/*
 * Reply from remote read
 */
struct nfsrdresult {
	enum nfsstat	rr_status;		/* status of read */
	union {
		struct nfsrrok	rr_ok_u;	/* attributes, need for pagin */
	} rr_u;
};
/*
 * Reply from remote read - extended (ESV) protocol
 */
struct nfsesvrdresult {
	enum nfsstat	rr_status;		/* status of read */
	union {
		struct nfsesvrrok  rr_ok_u;	/* attributes, need for pagin */
	} rr_u;
};
#define	rr_ok		rr_u.rr_ok_u
#define	rr_attr		rr_u.rr_ok_u.rrok_attr
#define	rr_count	rr_u.rr_ok_u.rrok_count
#define	rr_data		rr_u.rr_ok_u.rrok_data
#define rr_map		rr_u.rr_ok_u.rrok_map
#define rr_vp		rr_u.rr_ok_u.rrok_vp


/*
 * File attributes which can be set
 */
struct nfssattr {
	u_long		sa_mode;	/* protection mode bits */
	u_long		sa_uid;		/* owner user id */
	u_long		sa_gid;		/* owner group id */
	u_long		sa_size;	/* file size in bytes */
	struct timeval	sa_atime;	/* time of last access */
	struct timeval	sa_mtime;	/* time of last modification */
};

struct nfsesvsattr {
	u_long		sa_mode;	/* protection mode bits */
	u_long		sa_uid;		/* owner user id */
	u_long		sa_gid;		/* owner group id */
	u_long		sa_size;	/* file size in bytes */
	struct timeval	sa_atime;	/* time of last access */
	struct timeval	sa_mtime;	/* time of last modification */
	s_token		sa_privs;	/* privileges token */
	s_token		sa_sens;	/* sensitivity token */
	s_token		sa_info;	/* information token */
	s_token		sa_integ;	/* integrity token */
	s_token		sa_ncs;		/* nationality caveat set token */
	s_token		sa_acl;		/* access control list token */
};

/*
 * Reply status with file attributes
 */
struct nfsattrstat {
	enum nfsstat	ns_status;		/* reply status */
	union {
		struct nfsfattr ns_attr_u;	/* NFS_OK: file attributes */
	} ns_u;
};
/*
 * Reply status with extended (ESV) file attributes
 */
struct nfsesvattrstat {
	enum nfsstat	ns_status;		/* reply status */
	union {
		struct nfsesvfattr	ns_attr_u;	/* NFS_OK: file attributes */
	} ns_u;
};
#define	ns_attr	ns_u.ns_attr_u

/*
 * NFS_OK part of read sym link reply union
 */
struct nfssrok {
	u_long	srok_count;	/* size of string */
	char	*srok_data;	/* string (up to NFS_MAXPATHLEN bytes) */
};

/*
 * Result of reading symbolic link
 */
struct nfsrdlnres {
	enum nfsstat	rl_status;		/* status of symlink read */
	union {
		struct nfssrok	rl_srok_u;	/* name of linked to */
	} rl_u;
};
#define	rl_srok		rl_u.rl_srok_u
#define	rl_count	rl_u.rl_srok_u.srok_count
#define	rl_data		rl_u.rl_srok_u.srok_data

/*
 * NFS_OK part of readlink for extended protocol
 */
struct nfsesvsrok {
	u_long	srok_count;
	char	*srok_data;
	struct nfsesvfattr srok_attr;
};

/*
 * Result of reading symbolic link in extended protocol
 */
struct nfsesvrdlnres {
	enum nfsstat		rl_status;	/* status of symlink read */
	union {
		struct nfsesvsrok	rl_srok_u;
	} rl_u;
};
#define rl_attr		rl_u.rl_srok_u.srok_attr

/*
 * Arguments to readdir
 */
struct nfsrddirargs {
	fhandle_t rda_fh;	/* directory handle */
	u_long rda_offset;	/* offset in directory (opaque) */
	u_long rda_count;	/* number of directory bytes to read */
};

/*
 * NFS_OK part of readdir result
 */
struct nfsrdok {
	u_long	rdok_offset;		/* next offset (opaque) */
	u_long	rdok_size;		/* size in bytes of entries */
	bool_t	rdok_eof;		/* true if last entry is in result */
	struct dirent *rdok_entries;	/* variable number of entries */
};

/*
 * Readdir result
 */
struct nfsrddirres {
	enum nfsstat	rd_status;	/* result status */
	u_long		rd_bufsize;	/* client request size (not xdr'ed) */
	u_long		rd_origreqsize;	/* client request size */
	union {
		struct nfsrdok rd_rdok_u;
	} rd_u;
};
#define	rd_rdok		rd_u.rd_rdok_u
#define	rd_offset	rd_u.rd_rdok_u.rdok_offset
#define	rd_size		rd_u.rd_rdok_u.rdok_size
#define	rd_eof		rd_u.rd_rdok_u.rdok_eof
#define	rd_entries	rd_u.rd_rdok_u.rdok_entries

/*
 * NFS_OK part of readdir result for extended protocol
 */
struct nfsesvrdok {
	struct nfsesvfattr	rdok_attr;	/* attributes of dir */
	u_long			rdok_offset;	/* next offset (opaque) */
	u_long			rdok_size;	/* size in bytes of entries */
	bool_t			rdok_eof;	/* true if have last entry */
	struct dirent		*rdok_entries;	/* variable number of entries */
};

/*
 * ESV readdir result
 */
struct nfsesvrddirres {
	enum nfsstat	rd_status;
	u_long		rd_bufsize;
	u_long		rd_origreqsize;
	union {
		struct nfsesvrdok	rd_rdok_u;
	} rd_u;
};
#define rd_attr		rd_u.rd_rdok_u.rdok_attr

/*
 * Arguments for directory operations
 */
struct nfsdiropargs {
	fhandle_t	da_fhandle;	/* directory file handle */
	char		*da_name;	/* name (up to NFS_MAXNAMLEN bytes) */
};

/*
 * NFS_OK part of directory operation result
 */
struct  nfsdrok {
	fhandle_t	drok_fhandle;	/* result file handle */
	struct nfsfattr	drok_attr;	/* result file attributes */
};

/*
 * Results from directory operation
 */
struct  nfsdiropres {
	enum nfsstat	dr_status;	/* result status */
	union {
		struct  nfsdrok	dr_drok_u;	/* NFS_OK result */
	} dr_u;
};
#define	dr_drok		dr_u.dr_drok_u
#define	dr_fhandle	dr_u.dr_drok_u.drok_fhandle
#define	dr_attr		dr_u.dr_drok_u.drok_attr

/*
 * NFS_OK part of directory operation result for extended protocol
 */
struct nfsesvdrok {
	fhandle_t		drok_fhandle;
	struct nfsesvfattr	drok_attr;
};

/*
 * Results from directory operations for extended (ESV) protocol
 */
 struct nfsesvdiropres {
	enum nfsstat	dr_status;
	union {
		struct nfsesvdrok dr_drok_u;
	} dr_u;
};

/*
 * arguments to setattr
 */
struct nfssaargs {
	fhandle_t	saa_fh;		/* fhandle of file to be set */
	struct nfssattr	saa_sa;		/* new attributes */
};

/*
 * arguments to extended (ESV) setattr
 */
struct nfsesvsaargs {
	fhandle_t		saa_fh;
	struct nfsesvsattr	saa_sa;
};

/*
 * arguments to create and mkdir
 */
struct nfscreatargs {
	struct nfsdiropargs	ca_da;	/* file name to create and parent dir */
	struct nfssattr		ca_sa;	/* initial attributes */
};

/*
 * arguments to extended protocol create and mkdir
 */
struct nfsesvcreatargs {
	struct nfsdiropargs	ca_da;	/* file name to create and parent dir */
	struct nfsesvsattr	ca_sa;	/* initial attributes */
};

/*
 * arguments to link
 */
struct nfslinkargs {
	fhandle_t		la_from;	/* old file */
	struct nfsdiropargs	la_to;		/* new file and parent dir */
};

/*
 * arguments to rename
 */
struct nfsrnmargs {
	struct nfsdiropargs rna_from;	/* old file and parent dir */
	struct nfsdiropargs rna_to;	/* new file and parent dir */
};

/*
 * arguments to symlink
 */
struct nfsslargs {
	struct nfsdiropargs	sla_from;	/* old file and parent dir */
	char			*sla_tnm;	/* new name */
	struct nfssattr		sla_sa;		/* attributes */
};

/*
 * aguments to extended (ESV) protocol symlink
 */
struct nfsesvslargs {
	struct nfsdiropargs	sla_from;	/* old file and parent dir */
	char			*sla_tnm;	/* new name */
	struct nfsesvsattr	sla_sa;		/* attributes */
};

/*
 * NFS_OK part of statfs operation
 */
struct nfsstatfsok {
	u_long fsok_tsize;	/* preferred transfer size in bytes */
	u_long fsok_bsize;	/* fundamental file system block size */
	u_long fsok_blocks;	/* total blocks in file system */
	u_long fsok_bfree;	/* free blocks in fs */
	u_long fsok_bavail;	/* free blocks avail to the non-privileged */
};

/*
 * Results of statfs operation
 */
struct nfsstatfs {
	enum nfsstat	fs_status;	/* result status */
	union {
		struct	nfsstatfsok fs_fsok_u;	/* NFS_OK result */
	} fs_u;
};
#define	fs_fsok		fs_u.fs_fsok_u
#define	fs_tsize	fs_u.fs_fsok_u.fsok_tsize
#define	fs_bsize	fs_u.fs_fsok_u.fsok_bsize
#define	fs_blocks	fs_u.fs_fsok_u.fsok_blocks
#define	fs_bfree	fs_u.fs_fsok_u.fsok_bfree
#define	fs_bavail	fs_u.fs_fsok_u.fsok_bavail

/*
 * Arguments for access operation
 */
struct nfsaccessargs {
	fhandle_t	acc_fhandle;
	u_long		acc_flag;
};
/* Access types */
#define ACCESS_READ	0x001
#define ACCESS_WRITE	0x002
#define ACCESS_EXEC	0x004
#define ACCESS_SEARCH	0x008
#define ACCESS_APPEND	0x010

/*
 * NFS_OK part of access operation
 */
struct nfsaccessok {
	bool_t			accok_status;
	struct nfsesvfattr	accok_attr;
};

/*
 * Results of access operation
 */
struct nfsaccessres {
	enum nfsstat	acc_status;
	union {
		struct nfsaccessok acc_accok_u;
	} acc_u;
};
#define acc_accok	acc_u.acc_accok_u
#define acc_stat	acc_u.acc_accok_u.accok_status
#define acc_attr	acc_u.acc_accok_u.accok_attr

#ifdef _KERNEL

/*
 * XDR routines for handling structures defined above
 */
#ifdef __STDC__
bool_t xdr_attrstat(XDR *, struct nfsattrstat *);
bool_t xdr_esvattrstat(XDR *, struct nfsesvattrstat *);
bool_t xdr_creatargs(XDR *, struct nfscreatargs *);
bool_t xdr_esvcreatargs(XDR *, struct nfsesvcreatargs *);
bool_t xdr_diropargs(XDR *, struct nfsdiropargs *);
bool_t xdr_diropres(XDR *, struct nfsdiropres *);
bool_t xdr_esvdiropres(XDR *, struct nfsesvdiropres *);
bool_t xdr_drok(XDR *, struct nfsdrok *);
bool_t xdr_esvdrok(XDR *, struct nfsesvdrok *);
bool_t xdr_fattr(XDR *, struct nfsfattr *);
bool_t xdr_esvfattr(XDR *, struct nfsesvfattr *);
bool_t xdr_fhandle(XDR *, fhandle_t *);
bool_t xdr_linkargs(XDR *, struct nfslinkargs *);
bool_t xdr_rddirargs(XDR *, struct nfsrddirargs *);
bool_t xdr_putrddirres(XDR *, struct nfsrddirres *);
bool_t xdr_esvputrddirres(XDR *, struct nfsesvrddirres *);
bool_t xdr_getrddirres(XDR *, struct nfsrddirres *);
bool_t xdr_esvgetrddirres(XDR *, struct nfsesvrddirres *);
bool_t xdr_rdlnres(XDR *, struct nfsrdlnres *);
bool_t xdr_esvrdlnres(XDR *, struct nfsesvrdlnres *);
bool_t xdr_rdresult(XDR *, struct nfsrdresult *);
bool_t xdr_esvrdresult(XDR *, struct nfsesvrdresult *);
bool_t xdr_readargs(XDR *, struct nfsreadargs *);
bool_t xdr_rnmargs(XDR *, struct nfsrnmargs *);
bool_t xdr_rrok(XDR *, struct nfsrrok *);
bool_t xdr_esvrrok(XDR *, struct nfsesvrrok *);
bool_t xdr_saargs(XDR *, struct nfssaargs *);
bool_t xdr_esvsaargs(XDR *, struct nfsesvsaargs *);
bool_t xdr_sattr(XDR *, struct nfssattr *);
bool_t xdr_esvsattr(XDR *, struct nfsesvsattr *);
bool_t xdr_slargs(XDR *, struct nfsslargs *);
bool_t xdr_esvslargs(XDR *, struct nfsesvslargs *);
bool_t xdr_srok(XDR *, struct nfssrok *);
bool_t xdr_esvsrok(XDR *, struct nfsesvsrok *);
bool_t xdr_timeval(XDR *, struct timeval *);
bool_t xdr_writeargs(XDR *, struct nfswriteargs *);
bool_t xdr_fsok(XDR *, struct nfsstatfsok *);
bool_t xdr_statfs(XDR *, struct nfsstatfs *);
bool_t xdr_accessargs(XDR *, struct nfsaccessargs *);
bool_t xdr_accessok(XDR *, struct nfsaccessok *);
bool_t xdr_accessres(XDR *, struct nfsaccessres *);
#else
bool_t xdr_attrstat();
bool_t xdr_esvattrstat();
bool_t xdr_creatargs();
bool_t xdr_esvcreatargs();
bool_t xdr_diropargs();
bool_t xdr_diropres();
bool_t xdr_esvdiropres();
bool_t xdr_drok();
bool_t xdr_esvdrok();
bool_t xdr_fattr();
bool_t xdr_esvfattr();
bool_t xdr_fhandle();
bool_t xdr_linkargs();
bool_t xdr_rddirargs();
bool_t xdr_putrddirres();
bool_t xdr_esvputrddirres();
bool_t xdr_getrddirres();
bool_t xdr_esvgetrddirres();
bool_t xdr_rdlnres();
bool_t xdr_esvrdlnres();
bool_t xdr_rdresult();
bool_t xdr_esvrdresult();
bool_t xdr_readargs();
bool_t xdr_rnmargs();
bool_t xdr_rrok();
bool_t xdr_esvrrok();
bool_t xdr_saargs();
bool_t xdr_esvsaargs();
bool_t xdr_sattr();
bool_t xdr_esvsattr();
bool_t xdr_slargs();
bool_t xdr_esvslargs();
bool_t xdr_srok();
bool_t xdr_esvsrok();
bool_t xdr_timeval();
bool_t xdr_writeargs();
bool_t xdr_fsok();
bool_t xdr_statfs();
bool_t xdr_accessargs();
bool_t xdr_accessok();
bool_t xdr_accessres();
#endif /* __STDC__ */

#endif /* _KERNEL */

/*
 * Remote file service routines
 */
#define	RFS_NULL	0
#define	RFS_GETATTR	1
#define	RFS_SETATTR	2
#define	RFS_ROOT	3
#define	RFS_LOOKUP	4
#define	RFS_READLINK	5
#define	RFS_READ	6
#define	RFS_WRITECACHE	7
#define	RFS_WRITE	8
#define	RFS_CREATE	9
#define	RFS_REMOVE	10
#define	RFS_RENAME	11
#define	RFS_LINK	12
#define	RFS_SYMLINK	13
#define	RFS_MKDIR	14
#define	RFS_RMDIR	15
#define	RFS_READDIR	16
#define	RFS_STATFS	17
#define RFS_NPROC	18
#define RFS_ACCESS	18
#define RFS_ESVNPROC	19

/*
 * remote file service numbers
 */
#define	NFS_PROGRAM	((u_long)100003)
#define	NFS_VERSION	((u_long)2)
#define NFS_ESVPROG	((u_long)200012)
#define NFS_ESVVERS	((u_long)1)
#define	NFS_PORT	2049

#ifdef _KERNEL

/*	function defs for NFS kernel */
#ifdef __STDC__
int	nfs_validate_caches(struct vnode *, struct cred *);
void	nfs_purge_caches(struct vnode *);
void 	nfs_cache_check(struct vnode *, timestruc_t);
void	nfs_attrcache(struct vnode *, struct nfsfattr *);
void	nfs_esvattrcache(struct vnode *, struct nfsesvfattr *);
void	nfs_attrcache_va(struct vnode *, struct vattr *);
void	set_attrcache_time(struct vnode *);
void	vattr_to_nattr(struct vattr *, struct nfsfattr *);
void	vattr_to_esvnattr(struct vattr *, struct nfsesvfattr *, struct netbuf *, lid_t *, struct acl *, u_int);
void	vattr_to_sattr(struct vattr *, struct nfssattr *);
void	vattr_to_esvsattr(struct vattr *, struct nfsesvsattr *, struct netbuf *, lid_t *, struct acl *, u_int);
void	sattr_to_vattr(struct nfssattr *, struct vattr *);
void	esvsattr_to_vattr(struct nfsesvsattr *, struct vattr *, lid_t *, struct acl *, u_int *);
void	nattr_to_vattr(struct vnode *, struct nfsfattr *, struct vattr *);
void	esvnattr_to_vattr(struct vnode *, struct nfsesvfattr *, struct vattr *, lid_t *, struct acl *, u_int *);
int	nfsgetattr(struct vnode *, struct vattr *, struct cred *);
int	nfsgetesvattr(struct vnode *, struct vattr *, struct cred *);
int	nfs_getattr_otw(struct vnode *, struct vattr *, struct cred *);
int	nfs_getattr_cache(struct vnode *, struct vattr *);
int	rfscall(struct mntinfo *, int, int, xdrproc_t, caddr_t, xdrproc_t, caddr_t, struct cred *);
void	setdiropargs(struct nfsdiropargs *, char *, struct vnode *);
int	setdirgid(struct vnode *);
u_int	setdirmode(struct vnode *, u_int);
struct vnode *makenfsnode(fhandle_t *, struct nfsfattr *, struct vfs *);
struct vnode *makeesvnfsnode(fhandle_t *, struct nfsesvfattr *, struct vfs *);
char	*newname(void);
int	makefh(fhandle_t *, struct vnode *, struct exportinfo *);
int	eqaddr(struct netbuf *, struct netbuf *, struct netbuf *);
int	nfstsize(void);
#else /* __STDC__ */
int	nfs_validate_caches();
void	nfs_purge_caches();
void 	nfs_cache_check();
void	nfs_attrcache();
void	nfs_attrcache_va();
void	set_attrcache_time();
void	vattr_to_nattr();
void	vattr_to_esvnattr();
void	vattr_to_sattr();
void	vattr_to_esvsattr();
void	sattr_to_vattr();
void	esvsattr_to_vattr();
void	nattr_to_vattr();
void	esvnattr_to_vattr();
int	nfsgetattr();
int	nfsgetesvattr();
int	nfs_getattr_otw();
int	nfs_getattr_cache();
int	rfscall();
void	setdiropargs();
int	setdirgid();
u_int	setdirmode();
struct vnode *makenfsnode();
char	*newname();
int	makefh();
int	eqaddr();
int	nfstsize();
#endif /* __STDC__ */

#ifdef DEBUG
extern int nfslog;
extern int nfs_log();

#define	NFSLOG(A, B, C, D)  ((void)((nfslog) && nfs_log((A), (B), (C), (D))))
#else
#define NFSLOG(A, B, C, D)
#endif
#endif	/* _KERNEL */

#endif	/* _FS_NFS_NFS_H */
