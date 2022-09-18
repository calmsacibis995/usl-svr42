/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_proto.h	1.10 16 May 1992 04:40:25 -  */
#ident	"@(#)uts-comm:fs/vxfs/vx_proto.h	1.4"

/*
 * Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054
 */

#ifndef	_FS_VXFS_VX_PROTO_H
#define	_FS_VXFS_VX_PROTO_H

#ifdef	__STDC__


/*
 * Functions from vx_ioctl.c
 */

extern int	vx_ioctl(struct vnode *, int, int, int, struct cred *, int *);
extern void	vx_timethaw(struct fs *);

/*
 * Functions from vx_alloc.c
 */

extern int	vx_extalloc(struct fs *, struct extalloc *,
			struct vx_tran *);
extern int	vx_extsearchodd(struct fs *, long, struct extalloc *,
				daddr_t, daddr_t);
extern int	vx_extsearchanyodd(struct fs *, long, struct extalloc *,
				daddr_t, daddr_t);
extern void	vx_extfree(struct inode *, daddr_t, daddr_t,
			struct vx_tran *);
extern int	vx_extmapupd(struct fs *, long, daddr_t, long, int, int);
extern void	vx_exttran(struct fs *, struct vx_emtran *, long,
				int, struct vx_tran *, daddr_t,
				long, struct vx_ctran **);
extern void	vx_demap(struct fs *, struct vx_tran *, int);
extern void	vx_unemap(struct fs *, struct vx_tran *, int);
extern void	vx_emapclone(struct fs *, struct vx_map *);
extern void	vx_nospace(struct fs *, int, int);

/*
 * Functions from vx_bitmaps.c
 */

extern int	vx_getmap(struct fs *, int, int, struct vx_map **);
extern void	vx_holdmap(struct fs *, struct vx_map *, struct vx_tran *,
			struct vx_ctran *, int);
extern void	vx_lockmap(struct vx_map *);
extern void	vx_unlockmap(struct vx_map *);
extern void	vx_putmap(struct fs *, struct vx_map *, 
			struct vx_mlink *, int, int, struct vx_tran *);
extern void	vx_mapbad(struct fs *, struct vx_map *);
extern void	vx_sumpush(struct fs *);
extern void	vx_mapinit(struct fs *, struct vx_map *, int, int);

/*
 * Functions from vx_vfsops.c
 */

extern void	vx_mount_fsalloc(struct vfs *, struct fs *, char *);
extern void	vx_mount_fsfree(struct fs *, struct vx_vfs *);
extern int	vx_mountroot(struct vfs *, enum whymountroot);
extern void	vx_init(struct vfssw *, int);
extern void	vx_disable(struct fs *);
extern int	vx_setfsflags(struct fs *, int, int, int);
extern int	vx_flushsuper(struct fs	*, int, int);
extern int	vx_freeze(struct fs *);
extern void	vx_thaw(struct fs *);
extern void	vx_updlock(void);
extern void	vx_updunlock(void);

/*
 * Functions from vx_log.c
 */

extern int	vx_log(struct fs *, struct vx_tran *, int);
extern void	vx_logflush(struct fs *);
extern void	vx_logbuffree(void);
extern void	vx_logbufgrow(struct fs *);

/*
 * Functions from vx_tran.c
 */

extern void	vx_traninit(struct fs *, struct vx_tran **, int);
extern int	vx_trancommit(struct fs *, struct vx_tran *);
extern void	vx_tranundo(struct fs*, struct vx_tran *, int);
extern void	vx_tranfree(struct fs *, struct vx_tran *);
extern void	vx_subtranalloc(struct vx_tran *, int, int, 
			struct vx_ctran **, union vx_subfunc *);
extern int	vx_tranflush(struct fs *, int, int);
extern void	vx_mem_unmount(void);
extern void	vx_mem_init(void);
extern void	vx_ildone(struct inode *);
extern void	vx_ilyank(struct inode *);
extern void	vx_delbuf_flush(struct inode *);
extern void	vx_bufiodone(struct buf *);
extern void	vx_mapiodone(struct buf *);
extern void	vx_mlink_done(struct fs *, struct vx_mlink *);
extern void	vx_logwrite_flush(struct inode *);
extern void	vx_logwrite_iodone(struct inode *, off_t, u_int, int);
extern void	vx_bufstrategy(struct buf *);
extern void	vx_bufstrategy1(struct buf *);
extern void	vx_mapstrategy(struct buf *);
extern void	vx_mapstrategy1(struct buf *);

/*
 * Functions from vx_inode.c
 */

extern void	vx_inoinit(void);
extern int	vx_iget(struct fs *, int, struct inode **, struct vx_tran *);
extern void	vx_idrop(struct inode *);
extern int	vx_iaccess(struct inode *, int, struct cred *);
extern void	vx_itimes(struct inode *);
extern void	vx_iinactive(struct inode *);
extern void	vx_iupdat(struct inode *);
extern void	vx_iput(struct inode *);
extern void	vx_ilisterr(struct inode *, int);
extern void	vx_freeze_iflush(struct fs *);
extern int	vx_freeze_idone(struct fs *);
extern void	vx_flushi(struct fs *, int);
extern void	vx_delxwri_sync(void);
extern int	vx_iunmount(struct fs *, int);
extern int	vx_iremount(struct fs *);
extern void	vx_mkimtran(struct vx_tran *, struct inode *,
				struct vx_ctran **, struct vx_imtran **);
extern void	vx_lock4(struct inode *, struct inode *,
				struct inode *, struct inode *);
extern void	vx_inull(struct fs *);
extern void	vx_idelxwri_flush(struct inode *, int);
extern void	vx_irwlock(struct inode *);
extern void	vx_irwunlock(struct inode *);
extern void	vx_iglock(struct inode *);
extern void	vx_igunlock(struct inode *);
extern void	vx_ilock(struct inode *);
extern void	vx_iunlock(struct inode *);
extern void	vx_markibad(struct inode *, char *);

/*
 * Functions from vx_bio.c
 */

extern struct buf	*vx_breada(struct fs *, daddr_t, long, int);
extern struct buf	*vx_getblka(dev_t, daddr_t, long, int);
extern int		vx_bio(struct fs *, int, daddr_t, long, struct buf **);
extern void		vx_bufinval(struct fs *, daddr_t, long);
extern void		vx_bufflush(struct fs *, daddr_t, long);
extern void		vx_bwrite(struct fs *, struct buf *);
extern void		vx_bawrite(struct fs *, struct buf *);
extern void		vx_bdwrite(struct fs *, struct buf *);
extern void		vx_brelse(struct fs *, struct buf *);
extern void		vx_btwrite(struct fs *, struct buf *);

/*
 * Functions from vx_bmap.c
 */

extern int	vx_bmap(struct inode *, off_t, off_t *, struct extent *,
			struct extalloc *, struct vx_tran *);
extern int	vx_bmap_validate(struct fs *, struct inode *);
extern void	vx_untatran(struct fs *, struct vx_tran *, int);
extern int	vx_badextent(struct fs *, daddr_t, off_t);

/*
 * Functions from vx_dirop.c
 */

extern int	vx_dircreate(struct fs *, struct inode *, caddr_t,
			struct vattr *, struct cred *, int *,
			struct inode **, ino_t *, struct vx_tran *);
extern int	vx_dirdelete(struct fs *, struct inode *, caddr_t,
			struct inode *, struct cred *, struct vnode *,
			int, struct vx_tran *);
extern int	vx_dirlink(struct fs *, struct inode *, struct inode *,
			caddr_t, struct cred *, struct vx_tran *);

/*
 * Functions from vx_ialloc.c
 */

extern int	vx_ialloc(struct fs *, struct inode *, int, int, dev_t,
			int, int, ino_t *, struct inode **, struct vx_tran *);
extern int	vx_imap(struct fs *, ino_t, int, struct vx_map *, int,
			struct vx_tran *, struct vx_ietran **);
extern void	vx_dimap(struct fs *, struct vx_tran *, int);
extern void	vx_unimap(struct fs *, struct vx_tran *, int);
extern void	vx_imapclone(struct fs *, struct vx_map *);
extern int	vx_extopset(struct vx_tran *, struct inode *, int);
extern int	vx_extopclr(struct vx_tran *, struct inode *, int);

/*
 * Functions from vx_itrunc.c
 */

extern int 	vx_trunc(struct fs *, struct inode *, off_t, int, int);
extern void	vx_unindtrunc(struct fs *, struct vx_tran *, int);
extern int	vx_qtrunc_check(struct inode *);
extern int	vx_iremove(struct fs *, struct inode *, int,
			struct vx_tran *);
extern int	vx_freesp(struct vnode *, struct flock *, int);

/*
 * Functions from vx_dira.c
 */

extern int	vx_diradd(struct fs *, struct inode *, caddr_t, int, off_t,
			ino_t, struct vx_imtran *, struct vx_tran *);
extern void	vx_undiradd(struct fs *, struct vx_tran *, int);
extern void	vx_ddiradd(struct fs *, struct vx_tran *, int);
extern int	vx_dirrem(struct fs *, struct inode *, int, off_t,
			ino_t, struct vx_tran *);
extern void	vx_undirrem(struct fs *, struct vx_tran *, int);
extern int	vx_direrr(struct fs *, struct inode *, daddr_t, int);

/*
 * Functions from vx_dirl.c
 */

extern int	vx_dirlook(struct inode *, char *, struct inode **,
			struct cred *, int *);
extern int	vx_dirscan(struct inode *, char *, int, ino_t *, daddr_t *,
			off_t *, struct vx_tran *);
extern int	vx_dirbread(struct inode *, off_t, struct buf **,
			off_t *, off_t *, daddr_t *,
			struct vx_ctran **, struct vx_tran *);
extern int	vx_dirhash(off_t, char *, int);
extern int	vx_readdir(struct vnode *, struct uio *, struct cred *, int *);
extern int	vx_dirempty(struct fs *, struct inode *, struct vx_tran *);
extern int	vx_dirloop(struct fs *, struct inode *, struct inode *);

/*
 * Functions from vx_map.c
 */

extern int	vx_map(struct vnode *, u_int, struct as *, caddr_t *, 
			u_int, u_int, u_int, u_int, struct cred *);
extern int	vx_addmap(struct vnode *, u_int, struct as *, addr_t, 
			u_int, u_int, u_int, u_int, struct cred *);
extern int 	vx_delmap(struct vnode *, u_int, struct as *, addr_t,
			u_int, u_int, u_int, u_int, struct cred *);

/*
 * Functions from vx_dio.c
 */

extern int	vx_dio_check(struct inode *, struct uio	*, u_int);
extern int	vx_dio_writei(struct inode *, struct uio *, u_int);
extern int	vx_dio_readi(struct inode *, struct uio *, u_int);
extern int	vx_logged_write(struct inode *, struct uio *, u_int, int);

/*
 * Functions from vx_rdwri.c
 */

extern int	vx_read(struct vnode *, struct uio *, int, struct cred *);
extern int	vx_readnomap(struct inode *, struct uio *);
extern void	vx_do_read_ahead(struct inode *, u_int, u_int);
extern int	vx_write(struct vnode *, struct uio *, int, struct cred *);
extern int	vx_write_realloc(struct inode *, struct extalloc *,
			struct vx_tran *);
extern void	vx_idelxwri_realloc(struct inode *, int);
extern void	vx_async_shorten(struct inode *);
extern int	vx_alloc_clear(struct inode *, u_int, u_int, u_int *);
extern int	vx_clear_ext(struct inode *, daddr_t, u_int, u_int, u_int);
extern void	vx_mklwrtran(struct vx_tran *, struct inode *,
			u_int, u_int, caddr_t);
extern void	vx_dlwrtran(struct fs *, struct vx_tran *, int);
extern void	vx_logwrite_donetran(struct inode *);

/*
 * Functions from vx_getpage.c
 */

extern int	vx_getpage(struct vnode *, u_int, u_int, u_int *,
			struct page *[], u_int, struct seg *, addr_t,
			enum seg_rw, struct cred *);
extern int	vx_do_getpage(struct vnode *, ulong_t, ulong_t, u_int *,
			struct page *[], u_int, struct seg *, addr_t,
			enum seg_rw, int, char [], struct cred *);

/*
 * Functions from vx_putpage.c
 */

extern int	vx_putpage(struct vnode *, u_int, u_int, int,
			struct cred *);
extern int	vx_do_putpage(struct vnode *, ulong_t, ulong_t, int, int);
extern int	vx_io_ext(struct inode *, daddr_t, off_t,
			struct page *, off_t, int, int);

/*
 * Functions from vx_vnops.c
 */

extern void	vx_unsymlink(struct fs *, struct vx_tran *, int);

/*
 * Functions from vx_snap.c
 */

extern int	vx_snap_mount(struct vfs *, struct vnode *, struct cred *,
			      char *, struct vnode *, daddr_t);
extern int	vx_snap_unmount(struct vfs *, struct cred *);
extern void	vx_snap_strategy(struct fs *, struct buf *);
extern void	vx_snap_copy(struct fs *, struct buf *);
extern int	vx_snapread(struct fs *, struct vx_snapread *);
extern void	vx_copylock(struct fs *, int);
extern void	vx_copyunlock(struct fs *, int);
extern int	vx_mount_args(struct mounta *, struct vx_mountargs3 *,
			      int, int, int *);
extern void	vx_dev_init(void);
extern dev_t	vx_dev_alloc(void);
extern void	vx_dev_free(dev_t);
extern void	vx_cluster_init(void);

#else	/* not __STDC__ */


/*
 * Functions from vx_ioctl.c
 */

extern int	vx_ioctl();
extern void	vx_timethaw();

/*
 * Functions from vx_alloc.c
 */

extern int	vx_extalloc();
extern int	vx_extsearchodd();
extern int	vx_extsearchanyodd();
extern void	vx_extfree();
extern int	vx_extmapupd();
extern void	vx_exttran();
extern void	vx_demap();
extern void	vx_unemap();
extern void	vx_emapclone();
extern void	vx_nospace();

/*
 * Functions from vx_bitmaps.c
 */

extern int	vx_getmap();
extern void	vx_holdmap();
extern void	vx_lockmap();
extern void	vx_unlockmap();
extern void	vx_putmap();
extern void	vx_mapbad();
extern void	vx_sumpush();
extern void	vx_mapinit();

/*
 * Functions from vx_dirop.c
 */

extern int	vx_dircreate();
extern int	vx_dirdelete();
extern int	vx_dirlink();

/*
 * Functions from vx_vfsops.c
 */

extern void	vx_mount_fsalloc();
extern void	vx_mount_fsfree();
extern int	vx_mountroot();
extern void	vx_init();
extern void	vx_disable();
extern int	vx_setfsflags();
extern int	vx_flushsuper();
extern int	vx_freeze();
extern void	vx_thaw();
extern void	vx_updlock();
extern void	vx_updunlock();

/*
 * Functions from vx_log.c
 */

extern int	vx_log();
extern void	vx_logflush();
extern void	vx_logbuffree();
extern void	vx_logbufgrow();

/*
 * Functions from vx_tran.c
 */

extern void	vx_traninit();
extern int	vx_trancommit();
extern void	vx_tranundo();
extern void	vx_tranfree();
extern void	vx_subtranalloc();
extern int	vx_tranflush();
extern void	vx_mem_unmount();
extern void	vx_mem_init();
extern void	vx_ildone();
extern void	vx_ilyank();
extern void	vx_delbuf_flush();
extern void	vx_bufiodone();
extern void	vx_mapiodone();
extern void	vx_mlink_done();
extern void	vx_logwrite_flush();
extern void	vx_logwrite_iodone();
extern void	vx_bufstrategy();
extern void	vx_bufstrategy1();
extern void	vx_mapstrategy();
extern void	vx_mapstrategy1();

/*
 * Functions from vx_inode.c
 */

extern void	vx_inoinit();
extern int	vx_iget();
extern void	vx_idrop();
extern int	vx_iaccess();
extern void	vx_itimes();
extern void	vx_iinactive();
extern void	vx_iupdat();
extern void	vx_iput();
extern void	vx_ilisterr();
extern void	vx_freeze_iflush();
extern int	vx_freeze_idone();
extern void	vx_flushi();
extern void	vx_delxwri_sync();
extern int	vx_iunmount();
extern int	vx_iremount();
extern void	vx_mkimtran();
extern void	vx_lock4();
extern void	vx_inull();
extern void	vx_idelxwri_flush();
extern void	vx_irwlock();
extern void	vx_irwunlock();
extern void	vx_iglock();
extern void	vx_igunlock();
extern void	vx_ilock();
extern void	vx_iunlock();
extern void	vx_markibad();

/*
 * Functions from vx_itrunc.c
 */

extern int 	vx_trunc();
extern void	vx_unindtrunc();
extern int	vx_qtrunc_check();
extern int	vx_iremove();
extern int	vx_freesp();

/*
 * Functions from vx_dira.c
 */

extern int	vx_diradd();
extern void	vx_undiradd();
extern void	vx_ddiradd();
extern int	vx_dirrem();
extern void	vx_undirrem();
extern int	vx_direrr();

/*
 * Functions from vx_dirl.c
 */

extern int	vx_dirlook();
extern int	vx_dirscan();
extern int	vx_dirbread();
extern int	vx_dirhash();
extern int	vx_readdir();
extern int	vx_dirempty();
extern int	vx_dirloop();

/*
 * Functions from vx_bio.c
 */

extern struct buf	*vx_breada();
extern struct buf	*vx_getblka();
extern int		vx_bio();
extern void		vx_bufinval();
extern void		vx_bufflush();
extern void		vx_bwrite();
extern void		vx_bawrite();
extern void		vx_bdwrite();
extern void		vx_brelse();
extern void		vx_btwrite();

/*
 * Functions from vx_bmap.c
 */

extern int	vx_bmap();
extern void	vx_untatran();
extern int	vx_badextent();

/*
 * Functions from vx_ialloc.c
 */

extern int	vx_ialloc();
extern int	vx_imap();
extern void	vx_dimap();
extern void	vx_unimap();
extern void	vx_imapclone();
extern int	vx_extopset();
extern int	vx_extopclr();

/*
 * function from vx_map.c
 */
extern int	vx_map();
extern int	vx_addmap();
extern int 	vx_delmap();

/*
 * Functions from vx_dio.c
 */
extern int	vx_dio_check();
extern int	vx_dio_writei();
extern int	vx_dio_readi();
extern int	vx_logged_write();

/*
 * Functions from vx_rdwri.c
 */

extern int	vx_read();
extern int	vx_readnomap();
extern void	vx_do_read_ahead();
extern int	vx_write();
extern int	vx_write_realloc();
extern void	vx_idelxwri_realloc();
extern void	vx_async_shorten();
extern int	vx_alloc_clear();
extern int	vx_clear_ext();
extern void	vx_mklwrtran();
extern void	vx_dlwrtran();
extern void	vx_logwrite_donetran();

/*
 * Functions from vx_getpage.c
 */

extern int	vx_getpage();
extern int	vx_do_getpage();

/*
 * Functions from vx_putpage.c
 */

extern int	vx_putpage();
extern int	vx_do_putpage();
extern int	vx_io_ext();

/*
 * Functions from vx_vnops.c
 */

extern void	vx_unsymlink();

/*
 * Functions from vx_snap.c
 */

extern int	vx_snap_mount();
extern int	vx_snap_unmount();
extern void	vx_snap_strategy();
extern void	vx_snap_copy();
extern int	vx_snapread();
extern void	vx_copylock();
extern void	vx_copyunlock();
extern int	vx_mount_args();
extern void	vx_dev_init();
extern dev_t	vx_dev_alloc();
extern void	vx_dev_free();
extern void	vx_cluster_init();

#endif	/* __STDC__ */

#ifdef	__STDC__

extern int	splhi(void);
extern int	splx(int);
extern caddr_t	mappio(caddr_t, int, int);
extern int	unmappio(caddr_t, int, caddr_t);
extern int	pagezero(struct page *, u_int, u_int);
extern u_int	page_rdonly(struct page *);
extern int	pvn_vpempty(struct vnode *);
extern int	as_iolock(struct uio *, struct page **, u_int,
			struct vnode *, off_t, int *);
extern int	chklock(struct vnode *, int, off_t, int, int);
extern int	cleanlocks(struct vnode *, pid_t, sysid_t);
extern int	convoff(struct vnode *, struct flock *, int, off_t);
extern int	strlen(char *);
extern int	strcpy(char *, char *);
extern void	bp_mapin(struct buf *);
extern void	cleanup(void);
extern void	delay(long);
extern struct buf	*getfreeblk(long);
extern int	specpreval(vtype_t, dev_t, struct cred *);
extern void	clkset(time_t);

#else	/* not __STDC__ */

extern int	splhi();
extern int	splx();
extern caddr_t	mappio();
extern int	unmappio();
extern int	pagezero();
extern u_int	page_rdonly();
extern int	pvn_vpempty();
extern int	as_iolock();
extern int	chklock();
extern int	cleanlocks();
extern int	convoff();
extern int	strlen();
extern int	strcpy();
extern void	bp_mapin();
extern void	cleanup();
extern void	delay();
extern struct buf	*getfreeblk();
extern int	specpreval();
extern void	clkset();

#endif	/* __STDC__ */

#endif	/* _FS_VXFS_VX_PROTO_H */
