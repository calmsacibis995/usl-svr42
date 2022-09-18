/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/devconf.c	1.4"
#ident	"$Header:"

#include "inst.h"
#include "defines.h"
#include "devconf.h"

extern driver_t *driver_info;
extern int noloadable;
driver_t *bdevices, *cdevices;
ctlr_t *ctlr_info;
ivec_t ivec_info[256];


/* This routine is used to search the System table for some
 * specified device.  If the device is found we return a pointer to
 * that device.  If the device is not found, we return a NULL.
 */
ctlr_t *
sfind(device)
char *device;
{
        register ctlr_t *ctlr;

	for (ctlr = ctlr_info; ctlr != NULL; ctlr = ctlr->next) {
                if (equal(device, ctlr->sdev.name))
                        return(ctlr);
        }
        return(NULL);
}



/* Enter device in block device table, bdevices, and/or character device
 * table, cdevices.
 */
static int
enter(drv)
driver_t *drv;
{
        register driver_t **drv_p;
	int d_start, d_end;

	/* Link into block and char device lists, if appropriate */
	if (INSTRING(drv->mdev.mflags, BLOCK)) {
		d_start = drv->mdev.blk_start;
		d_end = drv->mdev.blk_end;
		if (d_start < BLOW || d_end > BHIGH) {
			sprintf(errbuf, IBDM, d_start, d_end, BLOW, BHIGH);
			error(1);
			return(0);
		}
		drv_p = &bdevices;
		while (*drv_p && (*drv_p)->mdev.blk_start <= d_start) {
			if (d_start <= (*drv_p)->mdev.blk_end) {
				sprintf(errbuf, DBDM, d_start, d_end,
					drv->mdev.name, (*drv_p)->mdev.name);
				error(1);
				return(0);
			}
			drv_p = &(*drv_p)->bdev_link;
		}
		drv->bdev_link = *drv_p;
		*drv_p = drv;
	}
	if (INSTRING(drv->mdev.mflags, CHAR)) {
		d_start = drv->mdev.chr_start;
		d_end = drv->mdev.chr_end;
		if (d_start < CLOW || d_end > CHIGH) {
			sprintf(errbuf, ICDM, d_start, d_end, CLOW, CHIGH);
			error(1);
			return(0);
		}
		drv_p = &cdevices;
		while (*drv_p && (*drv_p)->mdev.chr_start <= d_start) {
			if (d_start <= (*drv_p)->mdev.chr_end) {
				sprintf(errbuf, DCDM, d_start, d_end,
					drv->mdev.name, (*drv_p)->mdev.name);
				error(1);
				return(0);
			}
			drv_p = &(*drv_p)->cdev_link;
		}
		drv->cdev_link = *drv_p;
		*drv_p = drv;
	}

        return(1);
}



/* Get device configuration info; i.e. read Master and System files */

int
getdevconf(chkdev_f)
int (*chkdev_f)();
{
	struct mdev mdev;
        register driver_t *drv, **drv_p;
	struct sdev sdev;
	char fname[512];
	int stat;

	static int process_sdev();

        getinst(MDEV_D, RESET, NULL);

	while ((stat = getinst(MDEV_D, NEXT, &mdev)) == 1) {
                if ((drv = (driver_t *)calloc(sizeof(driver_t), 1)) == NULL) {
			sprintf(errbuf, TABMEM, "mdevice");
                        fatal(1);
                }

                drv->mdev = mdev;

		/* Insert in proper place, according to mdev.order */
		drv_p = &driver_info;
		while (*drv_p && (*drv_p)->mdev.order >= drv->mdev.order)
			drv_p = &(*drv_p)->next;
		drv->next = *drv_p;
		*drv_p = drv;
	}

	if (stat != 0)
		insterror(stat, MDEV_D, mdev.name);

	getinst(SDEV, RESET, NULL);

	while ((stat = getinst(SDEV, NEXT, &sdev)) == 1)
		process_sdev(&sdev, chkdev_f);

	if (stat != 0)
		insterror(stat, SDEV, sdev.name);

	return 0;
}



/* process an entry from sdevice */

static int
process_sdev(sdp, chkdev_f)
struct sdev *sdp;
int (*chkdev_f)();
{
        register driver_t *drv;
	register ctlr_t *ctlr;

        /* check if device instance is to be configured into Kernel */
        if (sdp->conf == 'N')
                return(0);

        if (sdp->conf != 'Y') {
                sprintf(errbuf, CONFD, sdp->name);
                error(1);
                return(0);
        }

        if ((drv = mfind(sdp->name)) == NULL) {
                sprintf(errbuf, SUCH, sdp->name);
                error(1);
                return(0);
        }

	if (chkdev_f && (*chkdev_f)(drv, sdp) == 0)
		return(0);

	if ((ctlr = (ctlr_t *)malloc(sizeof(ctlr_t))) == NULL) {
		sprintf(errbuf, TABMEM, "sdevice");
		fatal(1);
	}

        /* enter name in System table and update Master table */
	ctlr->next = ctlr_info;
	ctlr_info = ctlr;
        ctlr->driver = drv;
        ctlr->sdev = *sdp;
        ctlr->num = (drv->n_ctlr)++;
	ctlr->drv_link = drv->ctlrs;
	drv->ctlrs = ctlr;
        drv->tot_units += sdp->units;
	mdep_ctlr_init(ctlr);

        /* enter name in block or character name tables */
        if (drv->n_ctlr == 1 && !enter(drv))
                return(0);

        return(1);
}
