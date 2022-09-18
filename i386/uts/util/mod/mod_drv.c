/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/mod/mod_drv.c	1.10"
#ident	"$Header: $"

#include	<util/debug.h>
#include	<util/types.h>
#include	<util/param.h>
#include	<svc/errno.h>
#include	<util/cmn_err.h>
#include	<mem/kmem.h>
#include	<util/mod/mod_k.h>
#include	<util/mod/moddrv.h>
#include	<io/conf.h>
#include	<io/stream.h>
#include	<proc/cred.h>
#include	<proc/user.h>

#if RESTRICTED_DMA
#include	<mem/rdma.h>
#endif

/*
 * Dynamically loadable device driver support.
 */

extern	int	nodev(), nxio();
extern	int	nulldev();

extern	clock_t	lbolt;
extern	struct	modctl	*mod_shadowbsw[];
extern	struct	modctl	*mod_shadowcsw[];

extern	struct	cdevsw	shadowcsw[];
extern	struct	bdevsw	shadowbsw[];

extern	int	cdevswsz;
extern	int	bdevswsz;

STATIC	int	mod_drvinstall();
STATIC	int	mod_drvremove();
STATIC	int	mod_drvinfo();

/*
 * Autoload routine for block devices. Called through bdevsw[].
 */
STATIC int
mod_bdev_open(dev_t *devp, int flags, int otyp, struct cred *credp)
{
	struct	modctl	*modctlp;
	struct	bdevsw	*bdevp;
	int	maj, err;

	maj = getmajor(*devp);

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_bdev_open() major = %d", maj));

	ASSERT(mod_shadowbsw[maj] == NULL);

	bdevp = &bdevsw[maj];

	/*
	 * Load the driver registered to this major number.
	 *
	 * If the load is successful, this bdevsw[] table entry
	 * will be re-populated with values for the newly loaded
	 * device driver.
	 */
	if(err = modld(bdevp->d_name, sys_cred, &modctlp))	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    load failed errno = %d", err));
		return(err);
	}
	MOD_HOLD(modctlp);


	/* Check to see if bdevsw is still pointing to this function
	 * and fail if it is. This can occur if the driver module was
	 * previously loaded at a differnt major device range.

	if (bdevp->d_open == mod_bdev_open) {
		MOD_RELE(modctlp);
		return(ENXIO);
	}

	/*
	 * Call the driver's open routine.
	 */
	if(err = (*bdevp->d_open)(devp, flags, otyp, credp))	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    open failed errno = %d", err));
		MOD_RELE(modctlp);
	}

	return(err);
}

/*
 * Autoload routine for character devices. Called through cdevsw[].
 */
STATIC int
mod_cdev_open(dev_t *devp, int flags, int otyp, struct cred *credp)
{
	struct	modctl	*modctlp;
	struct	cdevsw	*cdevp;
	int	maj, err;

	maj = getmajor(*devp);

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_cdev_open() major = %d", maj));

	ASSERT(mod_shadowcsw[maj] == NULL);

	cdevp = &cdevsw[maj];

	/*
	 * Load the driver registered to this major number.
	 *
	 * If the load is successful, this cdevsw[] table entry
	 * will be re-populated with values for the newly loaded
	 * device driver.
	 */
	if(err = modld(cdevp->d_name, sys_cred, &modctlp))	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    load failed errno = %d", err));
		return(err);
	}
	MOD_HOLD(modctlp);


	/* Check to see if cdevsw is still pointing to this function
	 * and fail if it is. This can occur if the driver module was
	 * previously loaded at a differnt major device range.
	 */
	if (cdevp->d_open == mod_cdev_open) {
		MOD_RELE(modctlp);
		return(ENXIO);
	}

	/*
	 * Call the driver's open routine.
	 */
	if(err = (*cdevp->d_open)(devp, flags, otyp, credp))	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    open failed errno = %d", err));
		MOD_RELE(modctlp);
	}

	return(err);
}

static	struct	streamtab	modsdinfo;

/*
 * Autoload routine for STREAMS devices. Called through cdevsw[].
 */
STATIC int
mod_sdev_open(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	struct	modctl		*modctlp;
	struct	cdevsw		*cdevp;
	struct	streamtab	*stp;
	int	maj, err;

	maj = getmajor(*devp);

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_sdev_open() major = %d", maj));

	ASSERT(mod_shadowcsw[maj] == NULL);

	cdevp = &cdevsw[maj];

	/*
	 * Load the driver registered to this major number.
	 *
	 * If the load is successful, this cdevsw[] table entry
	 * will be re-populated with values for the newly loaded
	 * device driver.
	 */
	if(err = modld(cdevp->d_name, sys_cred, &modctlp))	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    load failed errno = %d", err));
		return(err);
	}

	MOD_HOLD(modctlp);

	/*
	 * The driver must be a STREAMS driver.
	 */
	stp = cdevp->d_str;


	/* if not a streams module or the module is already loaded
	 * to a different major device number range then fail the open 
	 * request.
	 */
	if (stp == NULL || stp == &modsdinfo) {
		MOD_RELE(modctlp);
		return(ENXIO);
	}

	/*
	 * Substitute the real qinit values for the current ones.
	 */
	setq(qp, stp->st_rdinit, stp->st_wrinit);

	/*
	 * Call the driver's open routine.
	 */
	if(*(cdevp->d_flag) & D_OLD)	{
		dev_t	odev;

		qp->q_flag |= QOLD;
		WR(qp)->q_flag |= QOLD;

		odev = cmpdev(*devp);
		if(odev == NODEV || (*qp->q_qinfo->qi_qopen)(qp, odev, flag, sflag) == OPENFAIL)  {
			MOD_RELE(modctlp);
			moddebug(cmn_err(CE_NOTE, "!MOD:    open failed errno = ENXIO"));
			return(u.u_error ? u.u_error : ENXIO);
		}
	}
	else	{
		if(err = (*qp->q_qinfo->qi_qopen)(qp, devp, flag, sflag, credp))  {
			MOD_RELE(modctlp);
			moddebug(cmn_err(CE_NOTE, "!MOD:    open failed errno = %d", err));
			return(err);
		}
	}

	return(0);
}

static	struct	module_info	modm_info = { 0, "MOD", 0, 0, 0, 0 };

static	struct	qinit	modrinit = {
	NULL, NULL, mod_sdev_open, NULL, NULL, &modm_info, NULL
};
static	struct	qinit	modwinit = {
	NULL, NULL, NULL, NULL, NULL, &modm_info, NULL
};

static	struct	streamtab	modsdinfo = { &modrinit, &modwinit };

static	int	moddevflag = 0;

/*
 * bdevsw[] entry for registered block device drivers.
 */
static	struct	bdevsw	nullbdev = {
		mod_bdev_open,
		nodev,
		nodev,
		nodev,
		nulldev,
		"nodev",
		(struct iobuf *)0,
		&moddevflag,
};

/*
 * cdevsw[] entry for registered character device drivers.
 */
static	struct	cdevsw	nullcdev = {
		mod_cdev_open,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		(struct tty *)0,
		(struct streamtab *)0,
		"nodev",
		&moddevflag,
};

/*
 * cdevsw[] entry for registered STREAMS device drivers.
 */
static	struct	cdevsw	nullsdev = {
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		(struct tty *)0,
		&modsdinfo,
		"nodev",
		&moddevflag,
};

struct	mod_operations	mod_drv_ops	= {
	mod_drvinstall,
	mod_drvremove,
	mod_drvinfo
};

/*
 * Install loadable character and block devices.
 */
STATIC	int
mod_drvinstall(struct mod_type_data *drvdatap, struct modctl *modctlp)
{
	register struct	mod_drv_data	*dta;
	register int	major, nm;
	char	*name;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_drvinstall()"));

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	if((nm = dta->bmajors) && dta->bmajor_0 + nm >= bdevswsz)	{
		return(ECONFIG);
	}
	if((nm = dta->cmajors) && dta->cmajor_0 + nm >= cdevswsz)	{
		return(ECONFIG);
	}

	/*
	 * Populate bdevsw[] for all supported block majors.
	 */
	if(nm = dta->bmajors)	{
		register struct	bdevsw	*bdevp;

		major = dta->bmajor_0;
		moddebug(cmn_err(CE_NOTE, "!MOD:    Block majors:"));

		while(nm--)	{
			moddebug(cmn_err(CE_NOTE, "!MOD:        %d", major));

			bdevp = &bdevsw[major];
			/*
			 * If the driver isn't registered for any of the
			 * configured major numbers, or any major has a
			 * static entry, fail the load.
			 */
			if(bdevp->d_open != mod_bdev_open)	{
				(void)mod_drvremove(drvdatap);	/* clean up */
				return(EINVAL);
			}
			name = bdevp->d_name;
			*bdevp = dta->drv_bdevsw;
			bdevp->d_name = name;
			mod_shadowbsw[major] = modctlp;
			fix_bswtbl(major);
#if RESTRICTED_DMA
			if (rdma_enabled)
				rdma_fix_bswtbl(major);
#endif
			major++;
		}
	}

	/*
	 * Populate cdevsw[] for all supported character majors.
	 */
	if(nm = dta->cmajors)	{
		register struct	cdevsw	*cdevp;

		major = dta->cmajor_0;
		moddebug(cmn_err(CE_NOTE, "!MOD:    Character majors:"));

		while(nm--)	{
			moddebug(cmn_err(CE_NOTE, "!MOD:        %d", major));

			cdevp = &cdevsw[major];
			/*
			 * If the driver isn't registered for any of the
			 * configured major numbers, or any major has a
			 * static entry, fail the load.
			 */
			if(cdevp->d_open != mod_cdev_open && cdevp->d_str != &modsdinfo)	{
				(void)mod_drvremove(drvdatap);	/* clean up */
				return(EINVAL);
			}
			name = cdevp->d_name;
			*cdevp = dta->drv_cdevsw;
			cdevp->d_name = name;
			mod_shadowcsw[major] = modctlp;
			fix_cswtbl(major);
			major++;
		}
	}

	return(0);
}

/*
 * Disconnect loadable character and block devices.
 */
STATIC	int
mod_drvremove(struct mod_type_data *drvdatap)
{
	struct	mod_drv_data	*dta;
	struct	modctl		**modctlpp;
	register int	major, nm;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_drvremove()"));

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	/*
	 * Reset bdevsw[] for all supported block majors.
	 */
	if(nm = dta->bmajors)	{
		register struct	bdevsw	*bdevp;

		major = dta->bmajor_0;
		moddebug(cmn_err(CE_NOTE, "!MOD:    Block majors:"));

		while(nm--)	{
			moddebug(cmn_err(CE_NOTE, "!MOD:        %d", major));

			/*
			 * If the next condition is true, it means
			 * that mod_drvinstall() failed at the same point, so
			 * there's no need to go on.
			 */
			if(*(modctlpp = &mod_shadowbsw[major]) == (struct modctl *)NULL)	{
				return(0);
			}

			*modctlpp = (struct modctl *)NULL;
			bdevp = &bdevsw[major];
			/*
			 * So the next open will trigger autoload.
			 */
			bdevp->d_open = mod_bdev_open;
			bdevp->d_flag = &moddevflag;
			bdevp->d_size = nulldev;
			major++;
		}
	}

	/*
	 * Reset cdevsw[] for all supported character majors.
	 */
	if(nm = dta->cmajors)	{
		register struct	cdevsw	*cdevp;

		major = dta->cmajor_0;
		moddebug(cmn_err(CE_NOTE, "!MOD:    Character majors:"));

		while(nm--)	{
			moddebug(cmn_err(CE_NOTE, "!MOD:        %d", major));

			/*
			 * If either of the next 2 conditions is true, it means
			 * that mod_drvinstall() failed at the same point, so
			 * there's no need to go on.
			 */
			if(major >= cdevswsz)	{
				return(0);
			}
			if(*(modctlpp = &mod_shadowcsw[major]) == (struct modctl *)NULL)	{
				return(0);
			}

			*modctlpp = (struct modctl *)NULL;
			cdevp = &cdevsw[major];
			/*
			 * So the next open will trigger autoload.
			 */
			if(cdevp->d_str)	{
				cdevp->d_str = &modsdinfo;
			}
			else	{
				cdevp->d_open = mod_cdev_open;
			}
			cdevp->d_flag = &moddevflag;
			major++;
		}
	}

	return(0);
}

STATIC	int
mod_drvinfo(struct mod_type_data *drvdatap, int *p0, int *p1, int *type)
{
	struct	mod_drv_data	*dta;
	int	*p;

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	*type = MOD_TY_BDEV;

	p0[0] = dta->bmajor_0;
	p0[1] = dta->bmajors;

	p1[0] = dta->cmajor_0;
	p1[1] = dta->cmajors;

	return(0);
}

static	char	cdev_lock, bdev_lock;

#define	LOCKED		1
#define	SLEEPING	2

#define	CDEV_LOCK()	{ \
				while(cdev_lock & LOCKED)	{ \
					moddebug(cmn_err(CE_NOTE, "!MOD: cdevsw locked, sleeping")); \
					cdev_lock |= SLEEPING; \
					sleep(&cdev_lock, PZERO); \
				} \
				moddebug(cmn_err(CE_NOTE, "!MOD: locking cdevsw")); \
				cdev_lock |= LOCKED; \
			}

#define	CDEV_UNLOCK()	{ \
				moddebug(cmn_err(CE_NOTE, "!MOD: unlocking cdevsw")); \
				cdev_lock &= ~LOCKED; \
				if(cdev_lock & SLEEPING) { cdev_lock&=~SLEEPING; wakeup(&cdev_lock); } \
			}

#define	BDEV_LOCK()	{ \
				while(bdev_lock & LOCKED)	{ \
					moddebug(cmn_err(CE_NOTE, "!MOD: bdevsw locked, sleeping")); \
					bdev_lock |= SLEEPING; \
					sleep(&bdev_lock, PZERO); \
				} \
				moddebug(cmn_err(CE_NOTE, "!MOD: locking bdevsw")); \
				bdev_lock |= LOCKED; \
			}

#define	BDEV_UNLOCK()	{ \
				moddebug(cmn_err(CE_NOTE, "!MOD: unlocking bdevsw")); \
				bdev_lock &= ~LOCKED; \
				if(bdev_lock & SLEEPING) { bdev_lock&=~SLEEPING; wakeup(&bdev_lock); } \
			}

/*
 * Name registration for character devices.
 */
int
mod_cdev_adm(unsigned int cmd, void *arg)
{
	struct	cdevsw	*cdevp;
	struct	mod_mreg	mr, *mrp;

	int	(*opn)();
	int	major;
	char	*name;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_cdev_adm()"));

	if(cmd != MOD_C_MREG)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    cmd != MOD_C_MREG: %d", cmd));
		return(EINVAL);
	}

	mrp = &mr;
	if(copyin(arg, mrp, sizeof(struct mod_mreg)))	{
		return(EFAULT);
	}

	if((major = (int)(mrp->md_typedata)) >= cdevswsz)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:   %d > %d", major, cdevswsz));
		return(ECONFIG);
	}

	CDEV_LOCK();

	cdevp = &cdevsw[major];

	/*
	 * Check if the module is already registered.
	 */
	if(!mod_shadowcsw[major] && cdevp->d_open != mod_cdev_open && cdevp->d_str != &modsdinfo)	{
		/*
		 * If it's not registered, fail if it's static.
		 */
		opn = (*cdevp->d_flag & D_OLD ? shadowcsw[major].d_open : cdevp->d_open);
		if (((opn != nodev) && (opn != nxio))
		   || cdevp->d_str != (struct streamtab *)0)	{
			CDEV_UNLOCK();
			return(EEXIST);
		}

		/*
		 * First time registry, allocate space for the name.
		 */
		name = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);
		*cdevp = nullcdev;
		cdevp->d_name = name;
	}
	else if(cdevp->d_str == &modsdinfo)	{
		/*
		 * It was registered, but we're changing its type.
		 */
		name = cdevp->d_name;
		*cdevp = nullcdev;
		cdevp->d_name = name;
	}

	strncpy(cdevp->d_name, mrp->md_modname, MODMAXNAMELEN);

	CDEV_UNLOCK();

	moddebug(cmn_err(CE_NOTE, "!MOD:                 %d, %s", major, cdevp->d_name));

	return(0);
}

/*
 * Name registration for STREAMS devices.
 */
int
mod_sdev_adm(unsigned int cmd, void *arg)
{
	struct	cdevsw	*cdevp;
	struct	mod_mreg	mr, *mrp;

	int	(*opn)();
	int	major;
	char	*name;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_sdev_adm()"));

	if(cmd != MOD_C_MREG)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    cmd != MOD_C_MREG: %d", cmd));
		return(EINVAL);
	}

	mrp = &mr;
	if(copyin(arg, mrp, sizeof(struct mod_mreg)))	{
		return(EFAULT);
	}

	if((major = (int)(mrp->md_typedata)) >= cdevswsz)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:   %d > %d", major, cdevswsz));
		return(ECONFIG);
	}

	CDEV_LOCK();

	cdevp = &cdevsw[major];

	/*
	 * Check if the module is already registered.
	 */
	if(!mod_shadowcsw[major] && cdevp->d_open != mod_cdev_open && cdevp->d_str != &modsdinfo)	{
		/*
		 * Fail if there's a static driver configured here.
		 */
		opn = (*cdevp->d_flag & D_OLD ? shadowcsw[major].d_open : cdevp->d_open);
		if ((opn != nodev && opn != nxio)
		 || cdevp->d_str != (struct streamtab *)0)	{
			CDEV_UNLOCK();
			return(EEXIST);
		}

		/*
		 * First time registry, allocate space for the name.
		 */
		name = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);
		*cdevp = nullsdev;
		cdevp->d_name = name;
	}
	else if(cdevp->d_open == mod_cdev_open)	{
		/*
		 * It was registered, but we're changing its type.
		 */
		name = cdevp->d_name;
		*cdevp = nullsdev;
		cdevp->d_name = name;
	}

	strncpy(cdevp->d_name, mrp->md_modname, MODMAXNAMELEN);

	CDEV_UNLOCK();

	moddebug(cmn_err(CE_NOTE, "!MOD:                 %d, %s", major, cdevp->d_name));

	return(0);
}

/*
 * Name registration for block devices.
 */
int
mod_bdev_adm(unsigned int cmd, void *arg)
{
	struct	bdevsw	*bdevp;
	struct	mod_mreg	mr, *mrp;

	int	(*opn)();
	int	major;
	char	*name;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_bdev_adm()"));

	if(cmd != MOD_C_MREG)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    cmd != MOD_C_MREG: %d", cmd));
		return(EINVAL);
	}

	mrp = &mr;
	if(copyin(arg, mrp, sizeof(struct mod_mreg)))	{
		return(EFAULT);
	}

	if((major = (int)(mrp->md_typedata)) > bdevswsz)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:   %d > %d", major, bdevswsz));
		return(ECONFIG);
	}

	BDEV_LOCK();

	bdevp = &bdevsw[major];

	/*
	 * Check if the module is already registered.
	 */
	if(!mod_shadowbsw[major] && bdevp->d_open != mod_bdev_open)	{
		/*
		 * If it's not registered, fail if it's static.
		 */
		opn = (*bdevp->d_flag & D_OLD ? shadowbsw[major].d_open : bdevp->d_open);
		if ((opn != nodev) && (opn != nxio))	{
			BDEV_UNLOCK();
			return(EEXIST);
		}

		/*
		 * First time registry, allocate space for the name.
		 */
		name = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);
		*bdevp = nullbdev;
		bdevp->d_name = name;
	}

	strncpy(bdevp->d_name, mrp->md_modname, MODMAXNAMELEN);

	BDEV_UNLOCK();

	moddebug(cmn_err(CE_NOTE, "!MOD:                 %d, %s", major, bdevp->d_name));

	return(0);
}
