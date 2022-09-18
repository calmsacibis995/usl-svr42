/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/mod/mod_fs.c	1.9"
#ident	"$Header: $"

#include	<util/debug.h>
#include	<util/types.h>
#include	<mem/kmem.h>
#include	<fs/vfs.h>
#include	<fs/vnode.h>
#include	<fs/fstyp.h>
#include	<svc/errno.h>
#include	<util/param.h>
#include	<proc/cred.h>
#include	<util/mod/mod_k.h>
#include	<util/mod/mod.h>
#include	<util/mod/moddefs.h>
#include	<util/mod/modfs.h>
#include	<util/cmn_err.h>

extern int vfsswsz, nfstype;
STATIC char **mod_fs_nametab = NULL;

/*
 *	STATIC int
 *	mod_fs_init(const vfssw_t *vswp, int fstype)
 *
 *	FS initialization stub used for all loadable file systems.  
 *
 *	Calling/Exit State:  It should never be called and panics if it is.
 */
STATIC int
mod_fs_init(const vfssw_t *vswp, int fstype)
{
	cmn_err(CE_PANIC,"MOD: Init routine called for dynamically loadable fs type %d.\n", fstype);
}

/*
 *	STATIC int
 *	mod_fs_null()
 *
 *	Stub routine used for all vfsops routines (except mount) for loadable
 *	filesystems after they are registered when they are not loaded.
 *
 *	Calling/Exit State: The routine is only called when the filesystem
 *	is not loaded and always returns 0.
 */
STATIC int
mod_fs_null()
{
	return(0);
}

/*
 *	STATIC int
 *	mod_fs_mount(const struct vfs *vfsp, const struct vnode *mvp, 
 *		const struct mounta *uap, const struct cred *cr)
 *
 *	Mount routine for loadable filesystems in the vfsops structure 
 *	after the filesystem type is registered and before it is loaded.
 *	Called indirectly from mount.
 *	
 *	Calling/Exit State: By always returning ENOLOAD, this routine signals
 *	the mount code to load the module for this file system type.
 */
STATIC int
mod_fs_mount(const struct vfs *vfsp, const struct vnode *mvp, const struct mounta *uap, const struct cred *cr)
{
	/* ENOLOAD indicates to mount(2) that it should try to modld the module */
	return(ENOLOAD);
}

STATIC struct vfsops mod_fs_vfsops = {
	mod_fs_mount,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,	
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
	mod_fs_null,
};

static char vfssw_lock;
#define	LOCKED		1
#define	SLEEPING	2

#define	VFSSW_LOCK()	{ \
				while(vfssw_lock & LOCKED)	{ \
					moddebug(cmn_err(CE_NOTE, "!MOD: vfssw locked, sleeping")); \
					vfssw_lock |= SLEEPING; \
					sleep(&vfssw_lock, PZERO); \
				} \
				moddebug(cmn_err(CE_NOTE, "!MOD: locking vfssw")); \
				vfssw_lock |= LOCKED; \
			}

#define	VFSSW_UNLOCK()	{ \
				moddebug(cmn_err(CE_NOTE, "!MOD: unlocking vfssw")); \
				vfssw_lock &= ~LOCKED; \
				if(vfssw_lock & SLEEPING) { vfssw_lock&=~SLEEPING; wakeup(&vfssw_lock); } \
			}

/*
 *	int
 *	mod_fs_adm(unsigned int cmd, void *arg)
 *
 * 	Name registration for file system types
 *
 *	Calling/Exit State: cmd contains the administrative command to perform given
 *	the information in arg.  Currently, the only valid command is module
 *	registration.  If this is successful, the routine returns 0, else the 
 *	appropriate errno.
 */
int
mod_fs_adm(unsigned int cmd, void *arg)
{
	struct	mod_mreg	mr, *mrp;
	vfssw_t *vswp;
	int index;
	char *fsname;
	int error;
	char	**namep;

	if(cmd != MOD_C_MREG)	{
		cmn_err(CE_NOTE, "!MOD:    cmd != MOD_C_MREG: %d", cmd);
		return(EINVAL);
	}

	/* must include this lock so that if the kmem_allocs sleep and
		another file system registration is processed in the meantime,
		no collision takes place.
	*/
	VFSSW_LOCK();
	if(mod_fs_nametab == NULL)
		mod_fs_nametab = (char **)
			kmem_zalloc(vfsswsz * sizeof(char*), KM_SLEEP);

	mrp = &mr;
	if(copyin(arg, mrp, sizeof(struct mod_mreg)) != 0) {
		error=EFAULT;
		goto out;
	}

	fsname = kmem_zalloc(FSTYPSZ,KM_SLEEP);
	if((error = copyinstr(mrp->md_typedata, 
		              fsname, FSTYPSZ, NULL)) != 0) {
		goto out;
	}


	if((vswp = vfs_getvfssw(fsname)) == NULL) {
		/* new registration */
		if(nfstype >= vfsswsz) {
			error=ECONFIG;
			goto out;
		}
		index = nfstype;
		nfstype++;
		vfssw[index].vsw_name = fsname;
		vfssw[index].vsw_init = mod_fs_init;
		vfssw[index].vsw_vfsops = &mod_fs_vfsops;
	}
	else {
		/* reregistration */
		index = vswp - vfssw;

		/* don't allow static modules to be overridden with dynamics */
		if(vfssw[index].vsw_init != mod_fs_init) {
			error=EEXIST;
			goto out;
		}
	}
	/*
	 *	Save the file name of the module to be loaded.
	 *	This is often the same as the name added to the vfssw table 
	 *	(the argument to mount) but can be different.
	 */
	namep = &mod_fs_nametab[index];

	if(*namep == NULL)	{
		*namep = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);
	}

	strncpy(*namep, mrp->md_modname, MODMAXNAMELEN);

out:
	VFSSW_UNLOCK();
	return(error);
}


/*
 *	const char *
 *	mod_fsname(unsigned int index)
 *
 * 	Return the name registered to a given vfssw index. Called from mount.
 *
 *	Calling/Exit State: index should be an index into the vfssw
 *	table for a registered loadable file system type. The routine returns
 *	a pointer to the file name of containing the file system module
 *	assocaited with that vfssw entry as most recently registered
 *	through modadm.  If the index is invalid or no registration has
 *	taken place, the routine panics NULL.
 */
const char *
mod_fsname(unsigned int index)
{


	if(index >= nfstype || mod_fs_nametab == NULL)
		cmn_err(CE_PANIC,
		     "MOD: Invalid index %d into vfssw given to mod_fsname.\n",
				index);

	return(mod_fs_nametab[index]);
}




STATIC int
mod_fs_install(const struct mod_type_data *fsdata, struct modctl *mcp)
{
	struct mod_fs_data *mfp;
	vfssw_t *vswp;

	mfp = (struct mod_fs_data *) fsdata->mtd_pdata;

	if((vswp = vfs_getvfssw(mfp->mfd_name)) == NULL)
		return(EINVAL);

	if(mod_shadowvfssw[vswp-vfssw] != NULL)
		cmn_err(CE_PANIC,"MOD: Trying to install already installed fs type %s.\n", vswp->vsw_name);

	vswp->vsw_vfsops = mfp->mfd_vfsops;
	vswp->vsw_flag = *mfp->mfd_fsflags;
	mod_shadowvfssw[vswp- vfssw] = mcp;
	return(0);
}

STATIC int
mod_fs_remove(const struct mod_type_data *fsdata)
{

	struct mod_fs_data *mfp;
	vfssw_t *vswp;

	mfp = (struct mod_fs_data *) fsdata->mtd_pdata;

	if((vswp = vfs_getvfssw(mfp->mfd_name)) == NULL)
		cmn_err(CE_PANIC,"MOD: Trying to remove unregistered fs type %s.\n", mfp->mfd_name);

	if(mod_shadowvfssw[vswp-vfssw] == NULL)
		cmn_err(CE_PANIC,"MOD: Trying to remove uninstalled fs type %s.\n", vswp->vsw_name);
	vswp->vsw_vfsops = &mod_fs_vfsops;
	vswp->vsw_flag = 0;
	mod_shadowvfssw[vswp- vfssw] = NULL;
	return(0);
}

	
	
STATIC	int
mod_fs_info(struct mod_type_data *fsdata, int *p0, int *p1, int *type)
{
	struct	mod_fs_data	*mfp;
	vfssw_t *vswp;


	mfp = (struct mod_fs_data *) fsdata->mtd_pdata;
	if((vswp = vfs_getvfssw(mfp->mfd_name)) == NULL)
		cmn_err(CE_PANIC, "MOD: Trying to get information on uninstalled fs module %s.\n", mfp->mfd_name);

	*type = MOD_TY_FS;
	*p0 = vswp- vfssw;

	return(0);
}



struct mod_operations mod_fs_ops = {
	mod_fs_install,
	mod_fs_remove, 
	mod_fs_info,
	NULL,
	NULL
};
