/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/gendev.c	1.17"
#ident	"$Header: $"

/*
 * INTERACTIVE UNIX Generic Driver Low-Level Library Routines.
 * This includes initialization and interrupt routines called directly
 * from the kernel
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1986, 1989 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <fs/buf.h>
#include <mem/kmem.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <io/iobuf.h>
#include <io/ioctl.h>
#include <util/cmn_err.h>
#include <io/vtoc.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#include <io/target/dynstructs.h>
#include <io/hba/dcd.h>
#include <io/hba/gendev.h>
#include <io/hba/gentape.h>
#include <io/target/alttbl.h>
#include <io/ddi.h>

extern struct head	sm_poolhead; 	/* head of pool of small dynamic */
				 	/* structs (28 bytes) for drq's */
int debugflag = 0;
int drq_needed = 0;     /* flag to indicate somebody is sleeping waiting for
			   an available drq entry (also what is slept on) */

/*
 * getdrq()  -- get a drq_entry.  Returns a pointer at an available
 *              drq_entry.  Will sleep until one is available, if necessary.
 *              Returned block is set to all 0's.
 */

struct  drq_entry  *
getdrq()
{
	register struct drq_entry  *drqp;

	drqp= (struct drq_entry *)sdi_get(&sm_poolhead, KM_SLEEP);
	drqp->drq_link = NULL;		
	drqp->drq_word1.drq_flgint = 0L;
	drqp->drq_addr1 = 0L;
	drqp->drq_addr2 = 0L;
	drqp->drq_vaddr = 0L;
	return (drqp);
}


/*
 * reldrq() -- return a previously-gotten drq_entry to the free list
 *             value is the drq_link field of the entry being released.
 */

struct drq_entry *
reldrq(drqp)
struct drq_entry  *drqp;
{
	struct drq_entry  *linkval = drqp->drq_link;

	sdi_free(&sm_poolhead, (jpool_t *)drqp);
	return (linkval);
}

/*
 * gdev_busy_drives() --  Checks if there are any busy drives on a controller 
 *                   If there are then it returns TRUE (1) if there are no
 *                   busy drives on the controller then it returns FALSE (0) 
 *                   Any drive busy on controller = return 1;
 *                   No drives busy on controller = return 0;
 *                   Code must be running at SPLDISK !!!
 */
int
gdev_busy_drives(dcbp)
register struct gdev_ctl_block *dcbp;
{
	ushort driver = 0;        /* Keep track of each driver on this controller */
	int common_drive_num = 0; /* Drives are counted in by the indivdual driver*/
	/* fields, but the pointers to the dpbp's are   */
	/* maintained in the controller structure using */
	/* a shared index space.                        */

	for (driver = 0; driver < dcbp->dcb_drivers ; driver++ )
	{
		int drives_under_driver = dcbp->dcb_drv[driver].drv_drives;
		while (drives_under_driver--)
		{
			if (dcbp->dcb_dpbs[common_drive_num++]->dpb_flags & DFLG_BUSY)
				return 1; /* A drive is busy on this controller */
		}
	}
	return 0; /* No drives are busy on this controller */
}



/*
 * gdev_getexcl() -- Get exclusive access to a controller.  This routine will
 *                   return with CFLG_EXCL set in dcb_flags of the passed
 *                   gdev_ctl_block when exclusive access is obtained.  This
 *                   exclusivity will obtain until gdev_relexcl is called.
 *                   NOTE:  THIS ROUTINE SHOULD ONLY BE CALLED FROM CODE WHICH
 *                          IS RUNNING AT 'SPLDISK' !!!
 */

void
gdev_getexcl(dcbp)
register struct gdev_ctl_block *dcbp;
{
	int oldpri;
	int oldidx = drvidx(dcbp);

	oldpri = SPLGDEV;
	while (dcbp->dcb_flags & (CFLG_BUSY | CFLG_EXCL) || gdev_busy_drives(dcbp))
	{
		dcbp->dcb_flags |= CFLG_EXCLREQ;
		sleep((char *)dcbp, PRIBIO);
	}
	dcbp->dcb_driveridx = oldidx;
	/* We won the scheduler!  Grant exclusivity and let our caller run! */
	dcbp->dcb_flags |= CFLG_EXCL;
	dcbp->dcb_flags &= ~CFLG_EXCLREQ;
	splx(oldpri);
}

/*
 * gdev_wakeup_drives()
 *                   Checks if there are any drives on this controller 
 *                   that are requesting DFLG_EXCLREQ drive accesss.  It  
 *                   wakes up all the drives that do have such a request to  
 *                   see if they can gain access.   In effect this routine
 *                   wakes up gdev_reserve_drive so that it can check if
 *                   DFLG_BUSY is currently set.
 *                   Code must be running at SPLDISK !!!
 */

int gdev_wakeup_drives(dcbp)
register struct gdev_ctl_block *dcbp;
{
	ushort driver = 0;        /* Keep track of each driver on this contrler */
	int common_drive_num = 0; /* Drives are counted in by each driver.      */
	/* But the pointers to them are kept in the   */
	/* controller structure.  The controller uses */
	/* a common area for the drives of all of its */
	/* controllers.                               */

	for (driver = 0; driver < dcbp->dcb_drivers ; driver++ )
	{
		int drives_under_driver = dcbp->dcb_drv[driver].drv_drives;
		while (drives_under_driver--)
		{
			register struct gdev_parm_block *dpbp = dcbp->dcb_dpbs[common_drive_num++];
			if (dpbp->dpb_flags & DFLG_EXCLREQ)
				wakeup((char *)dpbp); /* A drive needs to check if it is still busy */
		}
	}
}




/*
 * gdev_relexcl() -- Release exclusive controller access.  This routine will
 *                   release exclusive access to the controller indicated by
 *                   the passed gdev_ctl_block.  If there is another pending
 *                   exclusive request, it will be granted at this time.  If
 *                   not, normal I/O will be restarted.
 */

void
gdev_relexcl(dcbp)
register struct gdev_ctl_block *dcbp;
{
	int oldpri;
	oldpri = SPLGDEV;
	dcbp->dcb_flags &= ~CFLG_EXCL;
	if (dcbp->dcb_flags & CFLG_EXCLREQ)
	{       /* Somebody else wants exclusivity.  Give it to them. */
		wakeup((char *)dcbp);
		splx(oldpri);
		return;
	}
	/* No exclusive requests pending.  Restart normal I/O */
	dcbp->dcb_driveridx = 0;        /* Start with first driver on controller */
	(dcb_START(dcbp))(dcbp);
	splx(oldpri);

	/* Check for individual drive exclusive requests */
	gdev_wakeup_drives(dcbp);

}


/*
 * gdev_reserve_drive() 
 *                   -- Get exclusive access to a drive.  This routine will
 *                   return with DFLG_BUSY set in dpb_flags of the passed
 *                   gdev_parm_block when access to the drive is obtained.
 *                   This routine will wait until the drive is free befor it
 *                   obtains rights to the drive.   Rights to the drive are
 *                   released by calling gdev_release_drive().
 *                   NOTE:  THIS ROUTINE SHOULD ONLY BE CALLED FROM CODE WHICH
 *                          IS RUNNING AT 'SPLDISK' !!!
 */

void
gdev_reserve_drive(dcbp, dpbp)
register struct gdev_ctl_block *dcbp;
register struct gdev_parm_block *dpbp;
{
	int oldpri;
	int oldidx = drvidx(dcbp);
	oldpri = SPLGDEV;
	while ((dpbp->dpb_flags & DFLG_BUSY) || (dcbp->dcb_flags & CFLG_EXCL))
	{
		dpbp->dpb_flags |= DFLG_EXCLREQ;
		sleep((char *)dpbp, PRIBIO);
	}
	drvidx(dcbp) = oldidx;
	/* We won the scheduler!  Grant exclusivity and let our caller run! */
	dpbp->dpb_flags |= DFLG_BUSY;
	dpbp->dpb_flags &= ~DFLG_EXCLREQ;
	splx(oldpri);
}


/*
 * gdev_release_drive() 
 *                   Release access to a drive.  This routine releases access
 *                   to a drive.  It should only be called from code which 
 *                   has previously done a gdev_reserve_drive().  Upon return
 *                   it will have either granted another gdev_reserve_drive
 *                   request, or it will have released the drive by marking 
 *                   ~DFLG_BUSY.
 *                   NOTE:  THIS ROUTINE SHOULD ONLY BE CALLED FROM CODE WHICH
 *                          IS RUNNING AT 'SPLDISK' !!!
 */


void
gdev_release_drive(dcbp, dpbp)
register struct gdev_ctl_block *dcbp;
register struct gdev_parm_block *dpbp;
{
	register int cur_driveridx = dcbp->dcb_driveridx;
	int oldpri;
	oldpri = SPLGDEV;

	dpbp->dpb_flags &= ~DFLG_BUSY;
	if (dpbp->dpb_flags & DFLG_EXCLREQ)
	{
		/* Someone else wants drive access, so give it to them */
		wakeup((char *)dpbp);
		splx(oldpri);
		return;
	}

	/* No reserve_drive requests pending.  Restart normal I/O */
	dcbp->dcb_driveridx = 0;        /* Start with first driver on controller */
	(dcb_START(dcbp))(dcbp);
	dcbp->dcb_driveridx = cur_driveridx;
	splx(oldpri);
}


/*
 * gdev_xferok -- used by drivers that can detect partial transfer success
 *                to indicate that some number of sectors have been moved
 *                successfully between memory and the drive.  This routine
 *                updates the retry-point markers (dpb_memsect, dpb_curaddr,
 *                and dpb_memsect's dpb_count field) as well as dpb_cursect,
 *                dpb_sectcount, and dpb_totcount.  Any membreak entries
 *                between dpb_memsect and dpb_newdrq are examined for 'buf'
 *                pointers.  If found, the buffer headers are passed to
 *                'iodone' and their b_bcount values are subtracted from
 *                totcount.  This function also sets dpb_intret depending on
 *                on dpb_sectcount and dpb_req.
 */
void
gdev_xferok(dpbp,nsect)
register struct gdev_parm_block *dpbp;
int nsect;
{

	register struct drq_entry *drqp = dpbp->dpb_memsect;
	register dcdblk_t *bp;

	dpbp->dpb_cursect += nsect;
	dpbp->dpb_totcount += (nsect * dpbp->dpb_secsiz);

	if (drqp != NULL)
	{       /* actually have valid running-value fields.  clean up */

		while (drqp != dpbp->dpb_newdrq)
		{       /* release any used memory sections */
			if (drq_bufp(drqp))
			{       /* Have buffer that finished. */
				bp = drq_bufp(drqp);
				if (dpbp->dpb_flags & DFLG_FIXREC)
					dpbp->dpb_totcount -= 
					    bp->sb->sb.SCB.sc_datasz;
				else
					dpbp->dpb_totcount -= bp->sb->sb.SCB.sc_datasz;
				bp->sb->sb.SCB.sc_comp_code = SDI_ASW;
				bp->av_forw = (dcdblk_t *)dpbp->dpb_cbque;
				dpbp->dpb_cbque = (char *)bp;
			}
			drqp = reldrq(drqp);
		}

		/* NOTE: drqp now has dpbp->dpb_newdrq's value */
		if (dpbp->dpb_newcount == 0)
		{       /* used up dpb_newdrq's memsect, too */
			if ((bp = drq_bufp(drqp)) != NULL)
			{       /* clean up buffer */
				if (dpbp->dpb_flags & DFLG_FIXREC)
					dpbp->dpb_totcount -= bp->sb->sb.SCB.sc_datasz;
				else
					dpbp->dpb_totcount -= bp->sb->sb.SCB.sc_datasz;
				bp->sb->sb.SCB.sc_comp_code = SDI_ASW;
				bp->av_forw = (dcdblk_t *)dpbp->dpb_cbque;
				dpbp->dpb_cbque = (char *)bp;
			}
			dpbp->dpb_newaddr = dpbp->dpb_curaddr = drq_memaddr(drqp);
			dpbp->dpb_newvaddr = dpbp->dpb_virtaddr = drq_vaddr(drqp);
			drqp = reldrq(drqp);
			if ((drqp != NULL) && (drqp->drq_type == DRQT_MEMBREAK))
			{       /* have new memory section, use it */
				dpbp->dpb_newcount = drqp->drq_count;
				dpbp->dpb_newdrq = dpbp->dpb_memsect = drqp;
				dpbp->dpb_req = drqp->drq_link;
			}
			else {       /* No memsects left */
				dpbp->dpb_memsect = NULL;
				dpbp->dpb_req = drqp;
			}
		}
		else {       /* set restart-point values to running values */
			dpbp->dpb_memsect = drqp;
			dpbp->dpb_req = drqp->drq_link;
			drqp->drq_count = dpbp->dpb_newcount;
			dpbp->dpb_curaddr = dpbp->dpb_newaddr;
			dpbp->dpb_virtaddr = dpbp->dpb_newvaddr;
		}
	}

	/* set dpb_intret depending on what's left... */
	if ((dpbp->dpb_sectcount -= nsect) != 0)
	{       /* Not done with this disk section -- continue */
		dpbp->dpb_intret = DINT_CONTINUE;
		return;
	}
	else {       /* done with disk section -- all done? */
		while (dpbp->dpb_req)
		{       /* clean up any MEMXFERs and see what's left */
			drqp=dpbp->dpb_req;
			if (drqp->drq_type == DRQT_MEMXFER)
			{
				bcopy((char *)drq_srcaddr(drqp)+KVBASE,
				    (char *)drq_dstaddr(drqp)+KVBASE,
				    drqp->drq_count);
				dpbp->dpb_req = reldrq(drqp);
				continue;
			}

			if (drqp->drq_type == DRQT_MEMCLEAR)
			{
				bzero ((char *)drq_dstaddr(drqp)+KVBASE, drqp->drq_count);
				dpbp->dpb_req = reldrq(drqp);
				continue;
			}

			if (drqp->drq_type == DRQT_IGNORE)
			{
				dpbp->dpb_req = reldrq(drqp);
				continue;
			}

			if (drqp->drq_type == DRQT_MEMBREAK)
			{       /* this section complete */
				dpbp->dpb_intret = DINT_COMPLETE;
				return;
			}
			dpbp->dpb_intret = DINT_NEWSECT;        /* new section left */
			return;
		}
		dpbp->dpb_intret = DINT_COMPLETE;       /* Nothing left -- all done */
	}
}


/*
 * gdev_shrdcb -- look for an existing dcb to share.  If found, its
 *                dcb_drivers field is incremented and a pointer at
 *                it is returned.  If not, a pointer at the next avail
 *                dcb in the free array is returned.  In this case, the
 *                entire dcb has been zeroed except for dcb_drivers, which
 *                will be set to 1.
 */

gdev_dcbp
gdev_shrdcb(cfgp)
register gdev_cfgp cfgp;
{
	register gdev_dcbp dcbp = gdev_ctl_blocks;
	register int i = gdev_nextctl;

	if (cfgp->cfg_capab & CCAP_SHARED)
	{      /* can share this one -- see if any exist yet */
		for (; (i--)!=0; dcbp++)
		{
			if ((dcbp->dcb_capab & CCAP_SHARED) == 0)
				continue;       /* can't share this one */
			if (strcmp(cfgp->cfg_name,dcbp->dcb_name) != 0)
				continue;       /* names don't match */
			if ((cfgp->cfg_memaddr1 == dcbp->dcb_memaddr1) &&
			    (cfgp->cfg_memaddr2 == dcbp->dcb_memaddr2) &&
			    (cfgp->cfg_ioaddr1 == dcbp->dcb_ioaddr1) &&
			    (cfgp->cfg_ioaddr2 == dcbp->dcb_ioaddr2) &&
			    (cfgp->cfg_dmachan1 == dcbp->dcb_dmachan1) &&
			    (cfgp->cfg_dmachan2 == dcbp->dcb_dmachan2))
			{       /* found a match! */
				dcbp->dcb_driveridx = dcbp->dcb_drivers++;
				return (dcbp);
			}
		}
	}

	/* allocate and zero a new dcb */
	dcbp = &gdev_ctl_blocks[gdev_nextctl++];
	bzero ((char *)dcbp, sizeof(struct gdev_ctl_block));
	dcbp->dcb_drivers = 1;
	return (dcbp);
}


/*
 * gdev_filldcb -- fill up the fields of a dcb from a cfg_entry.  If the
 *                 dcb_drivers field is 1, ALL fields are filled in, on
 *                 a copy-by-name basis.  Otherwise, only the driver-specific
 *                 fields for the proper driver are stuffed.  NOTE: the
 *                 dcb_driveridx field MUST BE SET BY THE CALLER!
 *                 The 'start' argument is the function to start I/O for this
 *                 driver and 'intr' is the completion/error entry point for
 *                 the driver.
 */
void
gdev_filldcb(dcbp, cfgp, start, intr)
register gdev_dcbp dcbp;
register gdev_cfgp cfgp;
int (*start)();
int (*intr)();
{
	register struct gdev_driver *drvp = &dcbp->dcb_drv[drvidx(dcbp)];
	int i;

	dcbp->dcb_xferok = gdev_xferok;
	drvp->drv_CMD = cfgp->cfg_CMD;
	drvp->drv_OPEN = cfgp->cfg_OPEN;
	drvp->drv_CLOSE = cfgp->cfg_CLOSE;
	drvp->drv_START = start;
	drvp->drv_drvint = intr;
	drvp->drv_ioctls = &cfgp->cfg_ioctls;
	drvp->drv_drives = drvp->drv_curdriv = 0;
	drvp->drv_baseminor = cfgp->cfg_baseminor;
	for (i=GDEV_CTL_INT-1; i>=0; i--)
	{       /* copy interrupt routine pointers from cfg to drv */
		drvp->drv_INT[i] = cfgp->cfg_ints[i].cfg_INT;
	}

	if (dcbp->dcb_drivers != 1)
	{       /* We're sharing an existing dcb */
		drvp->drv_firstdriv = dcbp->dcb_drv[drvidx(dcbp)-1].drv_firstdriv +
		    dcbp->dcb_drv[drvidx(dcbp)-1].drv_drives;
	}
	else {       /* New dcb, fill in the rest of the stuff */
		dcbp->dcb_name = cfgp->cfg_name;
		dcbp->dcb_capab = cfgp->cfg_capab;
		dcbp->dcb_memaddr1 = cfgp->cfg_memaddr1;
		dcbp->dcb_memaddr2 = cfgp->cfg_memaddr2;
		dcbp->dcb_ioaddr1 = cfgp->cfg_ioaddr1;
		dcbp->dcb_ioaddr2 = cfgp->cfg_ioaddr2;
		dcbp->dcb_dmachan1 = cfgp->cfg_dmachan1;
		dcbp->dcb_dmachan2 = cfgp->cfg_dmachan2;
		dcbp->dcb_multint = cfgp->cfg_multint;
		dcbp->dcb_maxsec = cfgp->cfg_maxsec;
		dcbp->dcb_delay = cfgp->cfg_delay;
		dcbp->dcb_defsecsiz = cfgp->cfg_defsecsiz;
	}
}


gdev_setintr(dcbp, cfgp)
register gdev_dcbp dcbp;
register gdev_cfgp cfgp;
{
	register ushort level;
	int	 i;

	if (dcbp->dcb_drivers != 1)
		return;

	for (i=GDEV_CTL_INT-1; i>=0; i--) {       /* copy interrupt levels */
		level = cfgp->cfg_ints[i].cfg_intlev;

		if (level != 0) {  /* make a gdev_int_entry for this level */
			gdev_int_entries[gdev_next_int].int_ctl = dcbp;
			gdev_int_entries[gdev_next_int].int_idx = i;
			gdev_int_entries[gdev_next_int].int_link =
			    gdev_int_routines[level];
			gdev_int_routines[level] = &gdev_int_entries[gdev_next_int++];
		}
	}
}

/*
 * gdev_majorset -- allocate a majormap entry and a set of minormap ones.
 *                  value is the index of the first minormap entry to be
 *                  used for this major.  If not enough, panic.
 */

gdev_majorset(maj, nminor)
register int maj, nminor;
{
	register ushort retval;

	retval = gdev_nextminor;
	majormap[maj].cnf_valid = 1;
	majormap[maj].cnf_minorcnt = nminor;
	majormap[maj].cnf_minorp = &minormap[gdev_nextminor];

	gdev_nextminor += nminor;
	return (retval);
}


/*
 * gdev_count_breakup -- make sure no device section is larger than the
 *                       maximum count allowed by the controller.  Our
 *                       'head' argument is a pointer at a drq_entry which
 *                       is actually the head of a list, not part of it.
 */
gdev_count_breakup(dcbp,head,secsiz)
register gdev_dcbp dcbp;
struct drq_entry *head;		/* points to the drq list */
ushort secsiz;
{
	register struct drq_entry *drqp;
	register struct drq_entry *tail;
	int driveridx = drvidx(dcbp);
	/* We call getdrq, which might sleep, and
                                  * if it sleeps, the value of 
                                  * dcbp->dcb_driveridx could change.  If it
                                  * changes, then we need to put it back 
                                  */

	/* start at the first entry in the list */
	for (drqp=head->drq_link; drqp; drqp=drqp->drq_link)
	{
		while (drqp->drq_count > dcbp->dcb_maxsec)
		{       /* Break this one up */
			tail = getdrq();		 /* get another drq entry */
			dcbp->dcb_driveridx = driveridx;
			tail->drq_type = DRQT_CONT;	 /* continuation of last req */
			tail->drq_link = drqp->drq_link; /* put new entry in right */
			drqp->drq_link = tail;		 /* after entry that current */
			/* drqp points to */
			set_drq_memaddr (tail,
			    (ulong)(drq_memaddr(drqp)) +
			    (dcbp->dcb_maxsec * secsiz));
			set_drq_vaddr (tail,
			    (ulong)(drq_memaddr(drqp)) +
			    (dcbp->dcb_maxsec * secsiz));
			set_drq_daddr (tail, drq_daddr(drqp) +
			    dcbp->dcb_maxsec);
			tail->drq_count = drqp->drq_count - dcbp->dcb_maxsec;
			drqp->drq_count = dcbp->dcb_maxsec;
			drqp = tail;
		}
	}
}


/*
 * gdev_mem_breakup -- make sure no memory section spans an address
 *                     space discontinuity.  Also cause breaks at 64K
 *                     physical boundaries if CCAP_64K is set.
 *                     'flags' argument comes from buffer's b_flags field,
 *                     'head' argument is a pointer at a drq_entry which
 *                     is actually the head of a list, not part of it.
 */
gdev_mem_breakup(flags, procp, dcbp, head)
int flags;
struct   proc	*procp;
gdev_dcbp dcbp;
register struct drq_entry *head;
{
	register struct drq_entry *drqp;
	int driveridx;
	struct drq_entry *prevmem = head;
	driveridx = drvidx(dcbp);
	drqp = head->drq_link;

	while (drqp) {       /* loop for each current MEMBREAK entry */
		register struct  drq_entry *sparedrq;
		paddr_t curaddr;
		ulong	curpaddr; /* current page of xfer */
		ulong	lastpaddr; /* last page of xfer */
		ulong	tmppaddr; /* tmp page of xfer */
		ulong   curoffset;
		ulong   curcount;
	
		/*
		 * If there is no count on this section, it is just here
		 * to point at a buffer.  Skip it.
		 */

		if (drqp->drq_count == 0) {
			drqp = drqp->drq_link;
			continue;
		}

		/*
		 * Get logical addr of start of transfer from previous
		 * memory section entry.  We leave ALL disk blocks
		 * in virtual space 'til we get them fully broken up.
		 */

		curaddr = drq_memaddr (prevmem);
		if (!drq_bufp (prevmem))
		{       /* no buffer associated w/prev break */
			set_drq_bufp (prevmem, curaddr); /* save virt addr */
			prevmem->drq_virt = 1;  /* let later code know */
		}
		curpaddr = btop(vtop((caddr_t)curaddr,procp)); /*cur phy page*/
		curoffset = curaddr - ptob(btop((caddr_t)curaddr)); /*offset in page*/
		curcount = 0;
		set_drq_memaddr (prevmem, curpaddr+curoffset);
		set_drq_vaddr (prevmem, curaddr);

		/* Now hop 'prevmem' over any 0-count nodes... */
		while (prevmem->drq_link != drqp) {
			prevmem = prevmem->drq_link;
		}

		/*
		 * Get ending logical address of transfer.  For any
		 * pages spanned by the request, check for breaks
		 * and add MEMBREAK entries (prior to the one which
		 * actually points at the buffer & disk seg) as
		 * necessary.
		 */

		lastpaddr = btop(vtop((caddr_t)curaddr + (drqp->drq_count - 1),procp));
		while (curpaddr != lastpaddr)/*until at end of segment to xfer*/
		{    /* loop for each page checking for discontinuity */
			curcount += ptob(1) - curoffset;
			curaddr += ptob(1) - curoffset;
			curoffset = 0;

			tmppaddr=btop(vtop((caddr_t)curaddr,procp));
			if ((++curpaddr == tmppaddr) &&
		   	   (!((dcbp->dcb_capab & CCAP_16BIT) &&
	 	   	   (((uint)curpaddr & 0xf) == 0)))) {
				continue;
			}
			curpaddr = tmppaddr;

			/* Have page discontinuity.  Mark it */
			sparedrq = getdrq();
			dcbp->dcb_driveridx = driveridx;

			/*
			 * New memory section has 'curcount' bytes,
			 * next section starts at page boundary whose
			 * page frame is 'curpfn'.  Decr total count
			 * in entry which points at buffer.
			 */
			sparedrq->drq_type = DRQT_MEMBREAK;
			sparedrq->drq_count = curcount;
			set_drq_memaddr (sparedrq,curaddr);
			set_drq_vaddr (sparedrq,curaddr);
			set_drq_bufp (sparedrq, curaddr);   /* save virt addr */
			sparedrq->drq_virt = 1;
			sparedrq->drq_link = drqp;
			prevmem->drq_link = sparedrq;
			prevmem = sparedrq;
			drqp->drq_count -= curcount;
			curcount = 0;
		}
	
		/* do next memory section */
		prevmem = drqp;
		drqp = drqp->drq_link;
	}
}


/*
 * gdev_sect_merge -- merge device and memory sections chains, formed by 
 *		      gdev_count_breakup & gdev_mem_breakup respectively,
 *		      into a single request queue.
 *                    This will be left pointed at by the 'devhead'
 *                    argument.  'devhead' initially points at a drq_entry
 *                    which is the head of the device queue, not part of it.
 *                    Similarly, 'memhead' is a head for the memory section
 *                    queue.  'flags' is b_flags field from buf.
 *                    How we do this depends on the settings of CCAP_DMA and
 *                    CCAP_SCATGATH.  If DMA and not SCATGATH, we build a chain
 *                    which includes MEMXFER-type entries. Work in this type of
 *                    chain causes all activity to be sector-aligned. There will
 *                    be no MEMBREAK entries in such a chain except for pointing
 *                    at buffer headers which get completed. For a SCATGATH (or
 *                    non-DMA)chain, we work at the byte level. MEMBREAK entries
 *                    are inserted into the START/CONT chain as needed to
 *                    handle memory discontinuities.
 */

gdev_sect_merge(dcbp,dpbp,devhead,memhead,flags,procp)
gdev_dcbp dcbp;
gdev_dpbp dpbp;
struct drq_entry *devhead;
struct drq_entry *memhead;
int flags;
struct proc *procp;
{
	register struct drq_entry *devtail = devhead->drq_link;
	register struct drq_entry *memtail = memhead->drq_link;
	register struct drq_entry *drqp;
	ushort  secsiz = dpbp->dpb_secsiz;
	ushort  fixrecp = ((dpbp->dpb_flags & DFLG_FIXREC) ? 1 : 0);
	ushort  driveridx = drvidx(dcbp);

	if ((dcbp->dcb_capab & (CCAP_DMA | CCAP_SCATGATH)) == CCAP_DMA)
	{       /* DMA only -- no scatter gather;  sectorize */
		struct drq_entry *prevdev = devhead;    /* for making 'prior' inserts */
		paddr_t devaddr = drq_memaddr(devtail); /* current dev virt addr */
		daddr_t cursect = drq_daddr(devtail);   /* current sector */
		long    devsecs = devtail->drq_count;   /* sectors in curr dev section */
		long    memsecs = 0;    /* sectors in current mem section */
		long    membytes = 0;   /* partial-sector resid in mem sect */
		ushort  bufidx = 0;

		if (fixrecp == 0)
			devsecs /= (long)secsiz;
		while (memtail)
		{       /* until all mem sections done */
			memsecs += memtail->drq_count / secsiz;
			membytes += memtail->drq_count % secsiz;
			memsecs += membytes / (long)secsiz;
			membytes = membytes % (long)secsiz;

			/*
		 * Hop over dev sections which fit entirely into
		 * current memory section decrementing memory counts
		 * as we go.
		 */

			while (memsecs >= devsecs)
			{
				if (!devtail)
					break;
				memsecs -= devsecs;
				prevdev = devtail;
				devtail = devtail->drq_link;
				if (!devtail)
					break;
				devsecs = devtail->drq_count;
				devaddr = drq_memaddr (devtail);
				cursect = drq_daddr (devtail);
			}

			if (!devtail)
			{
				prevdev->drq_link = memtail;
				while (memtail)
				{       /* clear garbage from ents */
					memtail->drq_count = 0;
					set_drq_memaddr (memtail, 0L);
					memtail = memtail->drq_link;
				}
				break;
			}

			if (memsecs < devsecs)
			{       /* make dev section not span BREAK */
				if (memsecs == 0)
				{       /* discard dev section */
					drqp = devtail;
					prevdev->drq_link = drqp->drq_link;
					devtail = drqp->drq_link;
					reldrq (drqp);
				}
				else {       /* shorten dev section */
					devsecs -= memsecs;
					devtail->drq_count = memsecs;
					devaddr += memsecs * secsiz;
					cursect += memsecs;
					prevdev = devtail;
					devtail = devtail->drq_link;
				}
			}
			memsecs = 0;    /* have exhausted memory sectors */
			if (membytes != 0)
			{       /* have to make MEMXFERs & CONT */
				struct drq_entry *contp = getdrq();
				struct drq_entry *memx1p = getdrq();
				struct drq_entry *memx2p = getdrq();
				ulong bufaddr1, bufaddr2, memaddr1, memaddr2;
				paddr_t buftmp;

				dcbp->dcb_driveridx = driveridx;
				contp->drq_type = DRQT_CONT;
				contp->drq_count = 1;
				set_drq_memaddr (contp,dpbp->dpb_secbuf[bufidx]);
				set_drq_daddr (contp, cursect);

				memx1p->drq_type = DRQT_MEMXFER;
				memx1p->drq_count = membytes;
				memx1p->drq_link = memx2p;

				memx2p->drq_type = DRQT_MEMXFER;
				memx2p->drq_count = secsiz - membytes;

				buftmp = (daddr_t)dpbp->dpb_secbuf[bufidx];
				bufaddr1 = vtop((caddr_t)buftmp,procp);
				bufaddr2 = bufaddr1 + membytes;
				memaddr1 = vtop((caddr_t)devaddr,procp);
				if((memaddr2=(ulong)drq_memaddr(memtail)) != 0){
					memaddr2 = vtop((caddr_t)memaddr2,procp);
				}

				if (++bufidx > 1) 
					bufidx = 0;
				if (flags & B_READ) {
					set_drq_srcaddr (memx1p, bufaddr1);
					set_drq_dstaddr (memx1p, memaddr1);
					set_drq_srcaddr (memx2p, bufaddr2);
					if (memaddr2 != 0)
						set_drq_dstaddr (memx2p, memaddr2);
					else {       /* Handle short record */
						memx2p->drq_type = DRQT_IGNORE;
						membytes = 0;   /* done */
						memsecs = 1;
					}
					contp->drq_link = memx1p;
					prevdev->drq_link = contp;
					memx2p->drq_link = devtail;
					prevdev = memx2p;
				}
				else {
					set_drq_dstaddr (memx1p, bufaddr1);
					set_drq_srcaddr (memx1p, memaddr1);
					set_drq_dstaddr (memx2p, bufaddr2);
					if (memaddr2 != 0)
						set_drq_srcaddr (memx2p, memaddr2);
					else {       /* Handle short record */
						memx2p->drq_type = DRQT_MEMCLEAR;
						membytes = 0;   /* done */
						memsecs = 1;
					}
					memx2p->drq_link = contp;
					prevdev->drq_link = memx1p;
					contp->drq_link = devtail;
					prevdev = contp;
				}

				memsecs -= 1;   /* done part of 1st sector */
				devsecs -= 1;
				cursect += 1;
			}

		/*
		 * If we have a buffer pointer on our current MEMBREAK
		 * section, hang it after 'prevdev', since it is now
		 * done.  If we have any 0-length break entries, they
		 * get hung on, also.
		 * If no buffer pointer, just release the drq entry.
		 */

			if ((!(memtail->drq_virt)) && drq_bufp(memtail))
				do      {       /* tack it on */
					dcdblk_t *bufp;

					drqp = memtail->drq_link;
					bufp = drq_bufp (memtail);
					prevdev->drq_link = memtail;
					memtail->drq_count = 0;
					set_drq_memaddr (memtail, 0L);
					memtail->drq_link = devtail;
					prevdev = memtail;
					memtail = drqp;
				} while (memtail->drq_count == 0);
			else {
				devaddr = (paddr_t)drq_bufp(memtail);
				if (membytes != 0)
					devaddr += secsiz-membytes;
				drqp = memtail;
				memtail = memtail->drq_link;
				reldrq (drqp);
			}

		/* If there are any dev sectors left, generate a CONT */

			if (devsecs)
			{       /* gotta make one */
				struct drq_entry *contp = getdrq();

				dcbp->dcb_driveridx = driveridx;
				contp->drq_type = DRQT_CONT;
				contp->drq_count = devsecs;
				set_drq_memaddr (contp, devaddr);
				set_drq_daddr (contp, cursect);
				contp->drq_link = devtail;
				prevdev->drq_link = contp;
				devtail = contp;
			}
		}
	}
	else 
	{       /* scatter/gather or buffered controller; by bytes */

		ulong  resid;  /* resid bytes that aren't a mult of secsiz */
		ulong  devbytes = (fixrecp ? devtail->drq_count * secsiz
		    : devtail->drq_count); /* bytes in cur dev sect */
		ulong  membytes = 0;   /* bytes in current memory section */
		struct drq_entry *last_break;	/* Last non-padding mem_break section */

		while (memtail) {       /* Until all memory sections are done */
			drqp = devtail->drq_link;
			while (devbytes > membytes) {
				/* need to add a memory section(s) */
				if ( ( !(memtail->drq_link) ) &&
				    ((resid = (membytes + memtail->drq_count)) < devbytes))
				{
					/* Handle short record */
					struct drq_entry *temp;

					resid = resid % secsiz;
					membytes += secsiz;

					/* Create an extra MEMBREAK to round out to secsiz */
					temp = getdrq();
					dcbp->dcb_driveridx = driveridx;
					temp->drq_type = DRQT_MEMBREAK;
					temp->drq_virt = 0;
					temp->drq_count = secsiz - resid;
					temp->drq_link = NULL;
					set_drq_bufp(temp,drq_bufp(memtail));
					set_drq_memaddr(temp, NULL);

					/* Clear buf pointer, set new memaddr */
					memtail->drq_virt = 0;
					set_drq_bufp(memtail,NULL);
					if (flags & B_READ) {
			/*
			 * Set to 2nd scratch buf, used as a garbage can.
			 * Here is where we use the 2nd sratch bufr we 
			 * allocated in tape_init routine.
			 */
						paddr_t buftmp = (daddr_t) dpbp->dpb_secbuf[1];
						ulong bufaddr = 
						vtop((caddr_t)buftmp,procp);
						/* Set the drq memeaddr of temp to new address */
						set_drq_memaddr(memtail,bufaddr);
						set_drq_vaddr(memtail,buftmp);
					}
					else
					{
						/* Set to 1st scratch buf, used as source of nulls */
						paddr_t buftmp = (daddr_t) dpbp->dpb_secbuf[0];
						ulong bufaddr = 
						vtop((caddr_t)buftmp,procp);
						set_drq_memaddr(memtail,bufaddr);
						set_drq_vaddr(memtail,buftmp);
					}

					/* Add in last normal MEMBREAK (now with out bufp) */
					devtail->drq_link = memtail;
					devtail = memtail;
					memtail = memtail->drq_link;

					/* Add in new padding MEMBREAK */
					devtail->drq_link = temp;
					devtail = temp;
					last_break = temp;

					/* Reconnect tail of chain (usually tail is NULL anyway) */
					devtail->drq_link = drqp;

				}
				else
				{
					membytes += memtail->drq_count;
					devtail->drq_link = memtail;
					if (memtail->drq_virt)
					{       /* clear virtual flag & addr */
						memtail->drq_virt = 0;
						set_drq_bufp (memtail, NULL);
					}
					devtail = memtail;
					last_break = memtail;
					memtail = memtail->drq_link;
					devtail->drq_link = drqp;
				}
			}

			/*
	     * Attach zero length MEMBREAK sections to devchain 
	     * if they have valid buf field, otherwise throw them
	     * away.  Valid buf membreaks are kept so that iodone
	     * can be notified when a buf is finished
	     */

			while(memtail && 
			    (memtail->drq_type == DRQT_MEMBREAK) && 
			    (memtail->drq_count == 0))
			{
				devtail->drq_link = memtail;
				devtail = memtail;
				last_break = memtail;
				memtail = memtail->drq_link;
				devtail->drq_link = drqp;     /* Reestablish devchain */
			}

			while (drqp)
			{       /* add more dev section(s) */
				devbytes += (fixrecp
				    ? drqp->drq_count * secsiz
				    : drqp->drq_count);
				devtail = drqp;
				drqp = drqp->drq_link;
				if (devbytes > membytes) break;
			}
		}

		if ( devbytes > membytes )
		{
			/* Handle short record */
			struct drq_entry *temp;

			/* Create an extra MEMBREAK to round out to secsiz */
			temp = getdrq();
			dcbp->dcb_driveridx = driveridx;
			temp->drq_type = DRQT_MEMBREAK;
			temp->drq_virt = 0;
			temp->drq_count = devbytes - membytes;
			temp->drq_link = NULL;
			set_drq_bufp(temp, NULL);

			if (flags & B_READ)
			{
		/*
		 * Set to 2nd scratch buf, used as a garbage can.
		 * Here is where we use the 2nd sratch bufr we 
		 * allocated in tape_init routine.
		 */
				paddr_t buftmp = (daddr_t) dpbp->dpb_secbuf[1];
				ulong bufaddr = vtop((caddr_t)buftmp,procp);

				/* Set the drq memeaddr of temp to new address */
				set_drq_memaddr(last_break, bufaddr);
				set_drq_vaddr(last_break,buftmp);
			}
			else
			{
				/* Set to 1st scratch buf, used as source of nuls */
				paddr_t buftmp = (daddr_t) dpbp->dpb_secbuf[0];
				ulong bufaddr = vtop((caddr_t)buftmp,procp);
				set_drq_memaddr(last_break, bufaddr);
				set_drq_vaddr(last_break,buftmp);
			}

			/* Add in new padding MEMBREAK */
			devtail->drq_link = temp;
			devtail = temp;

			/* Reconnect tail of chain (usually tail is NULL anyway) */
			devtail->drq_link = drqp;
		}
	}
	memhead->drq_link = NULL;        /* Just in case */
}


gdev_start_mult(dcbp)
register gdev_dcbp dcbp;
{
	register gdev_dpbp dpbp;
	register int curdriv;
	int busydrives = 0;     	/* count of busy drives 	*/
	int drivcnt = dcb_drives(dcbp); /* count of how many dpbs to visit */
	int totdrivs = drivcnt;         /* Number of drives 		*/
	int firstdriv = dcb_firstdriv(dcbp);
	int driveridx = drvidx(dcbp);

	/* 	Controller not BUSY -- see if anything to do. 			*/
	while ((dcbp->dcb_flags & CFLG_BUSY) == 0) {
		/*
	 * A little elucidation here... We're going to march dcb_curdriv
	 * around round-robin from wherever it was last looking for something
	 * to do.  This involves a non-busy drive that either has something
	 * in its dpb_req field (something already in progress for which
	 * a new step must be started) or in its dpb_que field (a new
	 * request to start).  If we find something, we start it on its
	 * next step.  If this involves calling the CMD routine for the
	 * controller, we count on it to have left the CFLG_BUSY bit set on
	 * return if no further commands can be issued.  If it isn't
	 * set, we'll look for the next thing to do, and so on.  We get out
	 * of this loop when the controller is left busy by the CMD routine
	 * (the usual case unless MULTI-thread controller) or when we've done
	 * all our drives. If the controller still isn't busy, we can grant 
	 * exclusive access if anybody has requested it.  
	 */


		for (curdriv = (int)(dcb_curdriv(dcbp)+1) % totdrivs; drivcnt > 0;
		    curdriv = ++curdriv % totdrivs, drivcnt -= 1) {
			dpbp = dcbp->dcb_dpbs[curdriv + firstdriv];
			if (dpbp->dpb_flags & DFLG_BUSY) {
				busydrives++;  /* Remember that controller isn't idle.*/
				continue;      /* Drive busy doing offline op, skip   */
			}
			if (dpbp->dpb_flags & DFLG_OFFLINE)
				continue;       /* drive not really there -- skip */
			if (gdev_reload(dpbp))
				break;
		}
		/* 	If drivcnt is 0, we've gone through all our drives 		*/
		if (drivcnt == 0)
			break;

		/*	start new requests						*/
		dcb_curdriv(dcbp) = curdriv;    /* let world know where we are */
		gdev_start_req(dcbp, dpbp);
		dcbp->dcb_driveridx = driveridx;
		if (dpbp->dpb_flags & DFLG_BUSY)
			busydrives++;
	}

	return (busydrives);
}

gdev_start_req(dcbp, dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	register struct drq_entry *drqp;
	ushort	 spc;

	/*
	 * We now have dpbp pointing at a drive for which there is a request
	 * queue entry in dpb_req.
	 * If it is of type DRQ_MEMXFER, do both of them (they come in pairs)
	 * and hop over them.
	 * If it is of type DRQ_START, start a seek on it if appropriate.
	 * If it is of type DRQ_CONT, start data transfer.
	 */

	drqp = dpbp->dpb_req;
	if (drqp->drq_type == DRQT_MEMXFER) {       /* do the MEMXFERs */
		bcopy ((char *)drq_srcaddr(drqp)+KVBASE,
		    (char *)drq_dstaddr(drqp)+KVBASE, drqp->drq_count);
		drqp = reldrq(drqp);
		if (drqp->drq_type == DRQT_MEMXFER) { /* do the second XFER */
			/* */
			bcopy ((char *)drq_srcaddr(drqp)+KVBASE,
			    (char *)drq_dstaddr(drqp)+KVBASE,
			    drqp->drq_count);
		} else {       /* Only other possibility is MEMCLEAR */
			bzero((char
			*)drq_dstaddr(drqp)+KVBASE,drqp->drq_count);
		}
		drqp =reldrq(drqp);
		dpbp->dpb_req = drqp;   /* reset dpb's pointer */
	}
	/* 	Start of new dev section -- check for seek 			*/
	if (drqp->drq_type == DRQT_START) {
		if (!(dcbp->dcb_capab & CCAP_NOSEEK)) {
			/* 		a seeker -- see if we need to do one 			*/
			spc = dpbp->dpb_sectors * dpbp->dpb_heads;
			dpbp->dpb_cursect = drq_daddr(drqp);

			if ((dpbp->dpb_cursect/(daddr_t)spc)!=dpbp->dpb_curcyl){
				/* 				Mark drive & controller busy 		*/
				/* save time when command was started */
				drv_getparm(LBOLT, &dcbp->dcb_laststart);

				dpbp->dpb_flags |= DFLG_BUSY;
				dcbp->dcb_flags |= CFLG_BUSY;
				(dcb_CMD(dcbp))(DCMD_SEEK,dcbp,dpbp);
				drqp->drq_type = DRQT_CONT;
				dpbp->dpb_curcyl=dpbp->dpb_cursect/(daddr_t)spc;
				return;
			}
		}
		drqp->drq_type = DRQT_CONT;     /* assume cyl not important */
	}
	dpbp->dpb_curaddr = drq_memaddr(drqp);
	dpbp->dpb_virtaddr = drq_vaddr(drqp);
	dpbp->dpb_cursect = drq_daddr(drqp);
	dpbp->dpb_sectcount = drqp->drq_count;
	dpbp->dpb_req = reldrq(drqp);   /* Release the CONT		*/

	/*
 * 	If we already have a memsect, we must be working on a continuation
 * 	of a previous request for this drive.  Just leave well enough
 * 	alone, unless the link of memsect's MEMBREAK points at the
 * 	CONT we just creamed.  If that's the case, we make its link
 * 	whatever is in dpb_req.
 * 	If we don't have a memsect, see if we should have (non-scatter/gather
 * 	DMA won't have them).  If so, grab the new one and set the
 * 	running-value fields.  If not, nobody uses the running-value fields.
 */

	if (drqp=dpbp->dpb_memsect) {       /* check for a bad link 	*/
		struct drq_entry *tstdrq = drqp->drq_link;

		if ((tstdrq != NULL) && (tstdrq->drq_type == 0))
		{       /* points at released item */
			drqp->drq_link = dpbp->dpb_req;
		}
	} else {       /* set memsect if not DMA			*/
		if ((dcbp->dcb_capab & (CCAP_DMA | CCAP_SCATGATH)) != CCAP_DMA){
			drqp = dpbp->dpb_req;   /* MUST have entry here, CONT can't be last. */
			/* 			Save memory section pointer for low-level code */
			if ((drqp != NULL)&&(drqp->drq_type == DRQT_MEMBREAK)){
				dpbp->dpb_memsect = drqp;
				dpbp->dpb_req = drqp->drq_link;
			}
		}
	}

	/* 	drqp now points at the memsect, if any 				*/
	/* 	set the rest of the running value fields 			*/
	if((dpbp->dpb_newdrq=drqp) != NULL) {
		dpbp->dpb_newaddr = dpbp->dpb_curaddr;
		dpbp->dpb_newvaddr = dpbp->dpb_virtaddr;
		dpbp->dpb_newcount = drqp->drq_count;
	}

	/* save time when command was started */
	drv_getparm(LBOLT, &dcbp->dcb_laststart);

	dpbp->dpb_flags |= DFLG_BUSY;   /* mark drive as busy */
	dcbp->dcb_flags |= CFLG_BUSY;   /* and controller, too */
	dpbp->dpb_retrycnt = 0;

	(dcb_CMD(dcbp))((dpbp->dpb_flags&DFLG_READING ? DCMD_READ : DCMD_WRITE),
	    dcbp, dpbp);
}

gdev_reload(dpbp)
gdev_dpbp dpbp;
{
	register dcdblk_t *dcdp;
	register dcdblk_t *join_dcdp;

	/*	check for working queue */
	if (dpbp->dpb_req != NULL)
		return (1);

	dcdp = dpbp->dpb_que;
	if (dcdp == NULL)
		return(0);       /* No queue items for drive 		*/
	/* queue suspended for normal command				*/
	if ((dpbp->dpb_flags & DFLG_SUSPEND) &&
	    (dcdp->sb->sb.sb_type == SCB_TYPE))
		return(0);

	dpbp->dpb_procp = dcdp->b_procp; /* dbp holds current proc pointer */
				         /* update prior to start of new I/O */
	if (dcdp->b_flags & B_READ)
		dpbp->dpb_flags |= DFLG_READING;
	else 
		dpbp->dpb_flags &= ~DFLG_READING;

	dpbp->dpb_que = dcdp->av_forw;  /* reset header pointer */
	if (dpbp->dpb_que == NULL) { 	/* check if dpb_que is empty */
		dpbp->dpb_que = dpbp->dpb_nextque;
		dpbp->dpb_nextque = NULL;
	}
	dpbp->dpb_req = (struct drq_entry *)dcdp->drq_srt;
	dpbp->dpb_totcount = 0; /* nothing xferred yet 			*/

	return (1);
}

/*
 * gdev_cplerr -- process device completions and errors
 */

gdev_cplerr(dcbp, dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	register struct drq_entry *drqp;
	struct	dcdblk *dcdbp;


	switch ((int)dpbp->dpb_intret)
	{
	case DINT_CONTINUE:
		break;  /* Don't need to do anything */
	case DINT_COMPLETE:
	case DINT_NEWSECT:
		dpbp->dpb_flags &= ~DFLG_BUSY;
		dpbp->dpb_state = DSTA_IDLE;
		/* a MULTI-thread controller explicitly clears BUSY */
		if (!(dcbp->dcb_capab & CCAP_MULTI))
		{       /* single-thread controller not busy now */
			dcbp->dcb_flags &= ~CFLG_BUSY;
		}

		/* If actually complete and special, only do wakeup */
		if ((dpbp->dpb_intret == DINT_COMPLETE) &&
		    (dpbp->dpb_flags & DFLG_SPECIAL))
		{       /* Wakeup the proc whose special I/O is done */
			wakeup((char *)dpbp);
			break;  /* Don't restart normal I/O yet */
		}
		if ( (dpbp->dpb_intret == DINT_COMPLETE) &&
		   ( (drqp = dpbp->dpb_req) != NULL ) ) {
			if( drqp->drq_type == DRQT_TPCMD ) {

				dcdblk_t *bp = drq_bufp(drqp);

				if (bp == NULL) {
					break;
				}
	
				bp->sb->sb.SCB.sc_comp_code = SDI_ASW;
				drqp=reldrq(drqp);
				dpbp->dpb_req=drqp;
				bp->av_forw = (dcdblk_t *)dpbp->dpb_cbque;
				dpbp->dpb_cbque = (char *)bp;

				break;
			}
		}

		/* Clear out any 0-length memory sections found */
		for (drqp=dpbp->dpb_req;
		    ((drqp != NULL) && (drqp->drq_type == DRQT_MEMBREAK)); )
		{
			dcdblk_t *bp = drq_bufp(drqp);

			if ((bp == NULL) || drqp->drq_virt)  /* %%%% BOGUS! */
			{
				continue;
			}

			dpbp->dpb_totcount -= bp->sb->sb.SCB.sc_datasz;
			bp->sb->sb.SCB.sc_comp_code = SDI_ASW;
			drqp=reldrq(drqp);
			dpbp->dpb_req=drqp;
			bp->av_forw = (dcdblk_t *)dpbp->dpb_cbque;
			dpbp->dpb_cbque = (char *)bp;
		}

		/* Restart I/O (if any) */
		dcbp->dcb_driveridx = 0;
		(dcb_START(dcbp))(dcbp);
		break;
	case DINT_GENERROR:
		/* get an error code */
		dpbp->dpb_state = DSTA_GETERR;

		/* save time when command was started */
		drv_getparm(LBOLT, &dcbp->dcb_laststart);

		dpbp->dpb_flags |= DFLG_BUSY;
		dcbp->dcb_flags |= CFLG_BUSY;
		(dcb_CMD(dcbp))(DCMD_GETERR,dcbp,dpbp);
		break;
	case DINT_ERRABORT:
		/* Something blew up.  Handle error properly */
		if (gdev_error(dcbp,dpbp))
		{       /* we're killing it */
			dpbp->dpb_flags &= ~DFLG_BUSY;
			dpbp->dpb_state = DSTA_IDLE;
			dcbp->dcb_flags &= ~CFLG_BUSY;
			if (dpbp->dpb_flags & DFLG_SPECIAL)
			{       /* Wakeup the proc whose special I/O is done */
				wakeup((char *)dpbp);
				break;  /* Don't restart normal I/O yet */
			}
			dcbp->dcb_driveridx = 0;
			(dcb_START(dcbp))(dcbp);

			if (dpbp->dpb_flags & DFLG_EXCLREQ)
			{
				wakeup((char *)dpbp); /* Wakeup gdev_reserve_drive */
				break;
			}

			break;
		}
		else break;  /* retrying... */
	case DINT_NEEDCMD:
		/* issue some command */
		/* save time when command was started */
		drv_getparm(LBOLT, &dcbp->dcb_laststart);

		dpbp->dpb_flags |= DFLG_BUSY;
		dcbp->dcb_flags |= CFLG_BUSY;
		(dcb_CMD(dcbp))(dpbp->dpb_command,dcbp,dpbp);
		break;
	default:
		break;
	}

	while (dpbp->dpb_cbque) {
		dcdbp = (dcdblk_t *)dpbp->dpb_cbque;
		dpbp->dpb_cbque = (char *)dcdbp->av_forw;
		sdi_callback((caddr_t)dcdbp->sb);
	}
}


/*
 * gdev_error -- handle error reported by controller-level code...
 */
gdev_error(dcbp,dpbp)
gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	struct gdev_err_msg *dep = &gdev_err_msgs[dpbp->dpb_drverror];
	register dcdblk_t *bp;
	register struct drq_entry *drqp;
	int firstdrq = 1;

	if (!(dep->err_flags & ERF_NORETRY))
	{    /* we ALWAYS allow 1 retry, for misqueues by the controller... */
		if ((dpbp->dpb_flags & DFLG_RETRY) ||
		    ((dpbp->dpb_retrycnt < 1) && (dpbp->dpb_devtype == DTYP_DISK)))
			if ((dpbp->dpb_retrycnt+=1) <= 10)
			{       /* retry the last operation */
				if ((drqp=dpbp->dpb_memsect) != NULL)
				{       /* Reset running-value fields */
					dpbp->dpb_newdrq = drqp;
					dpbp->dpb_newcount = drqp->drq_count;
					dpbp->dpb_newaddr = dpbp->dpb_curaddr;
					dpbp->dpb_newvaddr = dpbp->dpb_virtaddr;
				}
				/* save time when command was started */
				drv_getparm(LBOLT, &dcbp->dcb_laststart);

				dpbp->dpb_flags |= DFLG_BUSY;
				dcbp->dcb_flags |= CFLG_BUSY;
				(dcb_CMD(dcbp))(DCMD_RETRY,dcbp,dpbp);
				return(0);
			}
	}

	/* set up sense data info */
	dpbp->dpb_reqdata.sd_ba = sdi_swap32(dpbp->dpb_cursect);
	dpbp->dpb_reqdata.sd_key = SD_MEDIUM;
	dpbp->dpb_reqdata.sd_sencode = SC_IDERR;
	dpbp->dpb_reqdata.sd_valid = 1;

#ifdef DEBUG
	if (((dep->err_flags & ERF_QUIET) == 0) &&
	    ((dpbp->dpb_flags & DFLG_RETRY) != 0))
	{       /* Complain to user */
		/*    FIX MESSAGE TO LOOK LIKE SCSI */
		dri_printf("\n*** DEVICE ERROR: %s ***\n",dep->err_msgptr);
		dri_printf("*** Controller %d (%s), %s Drive %d",
		    dcbp-gdev_ctl_blocks,dcbp->dcb_name,
		    ((dpbp->dpb_devtype == DTYP_DISK) ? "DISK" : "TAPE"),
		    dcb_curdriv(dcbp));
		if (dpbp->dpb_devtype == DTYP_DISK)
			dri_printf(", Absolute Sector # %d",dpbp->dpb_cursect);
		dri_printf(" ***\n");
	}
#endif


	/*
 * mark all buffer headers from current one onward as having error.
 * Start at dpb_memsect (if present), else at dpb_req.
 */

	if ((drqp=dpbp->dpb_memsect) == NULL)
		drqp = dpbp->dpb_req;

	while (drqp != NULL)
	{
	if ((drqp->drq_type == DRQT_MEMBREAK ||
	     drqp->drq_type == DRQT_TPCMD) &&
	    ((bp = drq_bufp(drqp)) != NULL))
		{
			if( firstdrq && dpbp->dpb_devtype == DTYP_TAPE) {
				/*
				 * Save residual count for tape total job
				 */
				dpbp->dpb_sectcount = (bp->sb->sb.SCB.sc_datasz-
					dpbp->dpb_totcount) / dpbp->dpb_secsiz;
				firstdrq = 0;
			}

			dpbp->dpb_totcount = 0;
			bp->sb->sb.SCB.sc_comp_code = SDI_CKSTAT;
			bp->sb->sb.SCB.sc_status = S_CKCON;
			bp->av_forw = (dcdblk_t *)dpbp->dpb_cbque;
			dpbp->dpb_cbque = (char *)bp;
		}
		drqp = reldrq(drqp);
	}
	dpbp->dpb_memsect = dpbp->dpb_req = NULL;
	return (1);
}


/*
 * gdev_drainq -- flush a tape drive's request queue marking all items as
 *                being at EOF (b_resid = b_bcount, b_err = 0)
 */

gdev_drainq(dpbp, ccode)
gdev_dpbp dpbp;
int	ccode;
{
	register dcdblk_t *dcdp = dpbp->dpb_que;
	register dcdblk_t *donedcdp;
	register struct drq_entry *drqp;

	dpbp->dpb_que = NULL;
	while (dcdp)
	{       /* loop through the DRQ chain */
		drqp = dcdp->drq_srt;

		dcdp = dcdp->av_forw;       /* get next dcdblk */
		/* when dpb_que finished do dpb_nextque */
		if (dcdp == NULL) {
			dcdp = dpbp->dpb_nextque;
			dpbp->dpb_nextque = NULL;
		}
		while (drqp != NULL)
		{
			donedcdp = drq_bufp(drqp);
			if ((drqp->drq_type == DRQT_MEMBREAK) && donedcdp)
			{
				donedcdp->sb->sb.SCB.sc_status = SD_NOSENSE;
				donedcdp->sb->sb.SCB.sc_comp_code = ccode;
				sdi_callback((caddr_t)donedcdp->sb);
			}
			drqp = reldrq(drqp);
		}
	}

}
