/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/mod/mod_str.c	1.4"
#ident "$Header: $"

#include	<util/debug.h>
#include	<util/types.h>
#include	<svc/errno.h>
#include	<util/cmn_err.h>
#include	<util/mod/mod_k.h>
#include	<io/stream.h>
#include	<io/strsubr.h>
#include	<io/conf.h>
#include	<proc/user.h>
#include	<io/ddi.h>

/*
 * Dynamically loadable STREAMS module support.
 */

extern	struct	modctl	*mod_shadowfsw[];
extern	int	fmodswsz;
extern	clock_t	lbolt;

STATIC	int	mod_strinstall();
STATIC	int	mod_strremove();
STATIC	int	mod_strinfo();

/*
 * Autoload routine for STREAMS modules. Called through fmodsw[].
 */
STATIC int
mod_smod_open(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	struct	modctl		*modctlp;
	struct	fmodsw		*fmodp;
	struct	streamtab	*stp;
	int	idx, err;

	idx = QU_MODIDX(qp);	/* passed from qattach() */

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_smod_open() idx = %d", idx));

	ASSERT(idx >= 0 && idx < fmodswsz);
	ASSERT(mod_shadowfsw[idx] == NULL);
	ASSERT(QU_MODCTLP(qp) == NULL);

	fmodp = &fmodsw[idx];

	/*
	 * Load the module registered to this fmodws[] index.
	 *
	 * If the load is successful, this fmodsw[] table entry
	 * will be re-populated with values for the newly loaded
	 * STREAMS module.
	 */
	if(err = modld(fmodp->f_name, sys_cred, &modctlp))	{
		moddebug(cmn_err(CE_NOTE, "!MOD:    load failed errno = %d", err));
		return(err);
	}
	MOD_HOLD(modctlp);

	stp = fmodp->f_str;
	ASSERT(stp != NULL);

	/*
	 * Substitute the real qinit values for the current ones.
	 */
	setq(qp, stp->st_rdinit, stp->st_wrinit);

	/*
	 * Call the module's open routine.
	 */
	if(*(fmodp->f_flag) & D_OLD)	{
		dev_t	odev;

		qp->q_flag |= QOLD;
		WR(qp)->q_flag |= QOLD;

		if((odev=cmpdev(*devp)) == NODEV ||
		   ((*qp->q_qinfo->qi_qopen)(qp, odev, flag, sflag) == OPENFAIL))  {
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

	QU_MODCTLP(qp) = modctlp;
	return(0);
}

static	struct	module_info	smodm_info = { 0, "MOD", 0, 0, 0, 0 };

static	struct	qinit	smodrinit = {
	putnext, NULL, mod_smod_open, NULL, NULL, &smodm_info, NULL
};
static	struct	qinit	smodwinit = {
	putnext, NULL, NULL, NULL, NULL, &smodm_info, NULL
};

static	struct	streamtab	modsminfo = { &smodrinit, &smodwinit };

static	int	modmodflag = 0;

struct	mod_operations	mod_str_ops	= {
	mod_strinstall,
	mod_strremove,
	mod_strinfo
};

/*
 * Install lodable streams module.
 */
STATIC	int
mod_strinstall(struct mod_type_data *strdatap, struct modctl *modctlp)
{
	register struct	fmodsw	*dta;
	register struct	fmodsw	*fmodp;
	int	idx;
	char	*name;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_strinstall()"));

	dta = (struct fmodsw *)strdatap->mtd_pdata;
	name = dta->f_name;

	/*
	 * Find the fmodsw[] table entry for the module.
	 */
	if((idx = findmod(name)) < 0)	{
		moddebug(cmn_err(CE_NOTE, "!MOD: module %s, not in fmodsw", name));
		return(EINVAL);
	}

	moddebug(cmn_err(CE_NOTE, "!MOD:    f_name = %s, idx = %d", name, idx));

	/*
	 * Populate the fmodsw[] entry with the module's information.
	 */
	fmodp = &fmodsw[idx];
	fmodp->f_str = dta->f_str;
	fmodp->f_flag = dta->f_flag;

	/*
	 * Save the address of the module's fmodsw[] entry.
	 */ 
	strdatap->mtd_pdata = (void *)fmodp;
	mod_shadowfsw[idx] = modctlp;

	return(0);
}

/*
 * Remove a loaded STREAMS module.
 */
STATIC	int
mod_strremove(struct mod_type_data *strdatap)
{
	register struct	fmodsw	*dta;
	int	idx;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_strremove()"));

	/*
	 * Saved in mod_strinstall().
	 */
	dta = (struct fmodsw *)strdatap->mtd_pdata;

	idx = dta - fmodsw;

	moddebug(cmn_err(CE_NOTE, "!MOD:    f_name = %s, idx = %d", dta->f_name, idx));

	/*
	 * Reset for next load.
	 */
	dta->f_str = &modsminfo;
	dta->f_flag = &modmodflag;
	mod_shadowfsw[idx] = NULL;

	return(0);
}

STATIC	int
mod_strinfo(struct mod_type_data *strdatap, int *p0, int *p1, int *type)
{
	struct	fmodsw	*dta;
	int	i;
	int	*p;

	/*
	 * Saved in mod_strinstall().
	 */
	dta = (struct fmodsw *)strdatap->mtd_pdata;

	*type = MOD_TY_STR;

	*p0 = dta - fmodsw;
	*p1 = -1;

	return(0);
}

/*
 * Name registration for STREAMS modules.
 */
int
mod_str_adm(unsigned int cmd, void *arg)
{
	struct	mod_mreg	mr, *mrp;
	register struct	fmodsw	*fmodp;

	int	nlen, i;
	char	*sname, *tname, nbuf[FMNAMESZ+1];

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_str_adm()"));

	if(cmd != MOD_C_MREG)	{
		return(EINVAL);
	}

	mrp = &mr;
	if(copyin(arg, mrp, sizeof(struct mod_mreg)))	{
		return(EFAULT);
	}

	sname = mrp->md_modname;

	tname = NULL;
	fmodp = fmodsw;
	for(i=0; i < fmodswsz; i++)	{
		if(*fmodp->f_name == '\0')	{
			tname = fmodp->f_name;
			break;
		}
		if(strcmp(fmodp->f_name, sname) == 0)	{
			if(fmodp->f_str != &modsminfo)	{
				moddebug(cmn_err(CE_NOTE, "!MOD: mod_str_adm(): EEXIST"));
				return(EEXIST);
			}
			else	{
				moddebug(cmn_err(CE_NOTE, "!MOD: %s, reregistered", sname));
				return(0);
			}
		}
		fmodp++;
	}
	if(!tname)	{
		moddebug(cmn_err(CE_NOTE, "!MOD: mod_str_adm(): EINVAL"));
		return(ECONFIG);
	}

	strncpy(tname, sname, FMNAMESZ);
	fmodp->f_str = &modsminfo;
	fmodp->f_flag = &modmodflag;

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_str_adm(): %s, %d", tname, i));

	return(0);
}
