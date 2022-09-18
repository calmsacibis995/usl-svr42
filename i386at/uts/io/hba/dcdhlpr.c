/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/dcdhlpr.c	1.3"
#ident	"$Header: $"

#include <util/types.h>
#include "io/target/sdi_edt.h"
#include "io/target/sdi.h"
#include "io/target/scsi.h"
#include <io/vtoc.h>
#include <io/target/fdisk.h>		/* Included for 386 disk layout		*/
#include <util/param.h>
#include <svc/errno.h>
#include <fs/buf.h>		/* Included for dma routines		*/
#include <io/elog.h>
#include <proc/cred.h>		/* Included for	cred structure argument */
#include <io/uio.h>		/* Included for	uio structure argument  */
#include <util/cmn_err.h>
#include <io/target/alttbl.h>
#include "io/target/altsctr.h"
#include "io/target/sd01.h"
#include <io/ddi.h>

int dcd_fopen();
int dcd_lclose();
void dcd_cmd();

#define INQ_DEVTYPE	8	/* index into inquiry string of device name */

struct dev_spec dcd_esdi = {
	"DCD     ESDI            ",
	dcd_fopen, 0, 0,
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	0
};

struct dev_spec dcd_mfm = {
	"DCD     MFM             ",
	dcd_fopen, 0, 0,
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	0
};

struct dev_spec dcd_esdi_cmd = {
	"DCD     ESDI            ",
	0, 0, 0,
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	dcd_cmd
};

struct dev_spec dcd_esdi_lclose = {
	"DCD     ESDI            ",
	0, dcd_lclose, 0,
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	0
};

struct dev_spec dcd_mfm_cmd = {
	"DCD     MFM             ",
	0, 0, 0,
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	dcd_cmd
};

struct dev_spec dcd_mfm_lclose = {
	"DCD     MFM             ",
	0, dcd_lclose, 0,
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
	0
};

int
dcd_fopen(diskp)
struct disk *diskp;
{
	printf("o");
	diskp->dk_spec = diskp->dk_spec->inquiry[INQ_DEVTYPE] == 'M' ?
				&dcd_mfm_cmd : &dcd_esdi_cmd;
	return 0;
}

int
dcd_lclose(diskp)
struct disk *diskp;
{
	printf("c");
	diskp->dk_spec = diskp->dk_spec->inquiry[INQ_DEVTYPE] == 'M' ?
				&dcd_mfm : &dcd_esdi;
	return 0;
}

void
dcd_catch(sbp)
struct sb *sbp;
{
	printf("i");

	sdi_freeblk(sbp);
}

void
dcd_cmd(diskp, sbp)
struct disk *diskp;
struct sb *sbp;
{
	struct sb *mysbp;
	struct msel_plist {
		unsigned char res1;
		unsigned char medtype;
		unsigned char res2;
		unsigned char bdlen;
		unsigned char pgcode;
		unsigned char pglen;
		struct pdinfo pdinfo;
	} msel_plist;
	static struct scs mselcmd = {
		SS_MSELECT, 0x11, 0, 0, sizeof(struct msel_plist), 0
	};

	printf("s");

	if (diskp->dk_state & DKVTOC) {
		mysbp = sdi_getblk();
		mysbp->sb_type = ISCB_TYPE;
		mysbp->SCB.sc_int = dcd_catch;

		mselcmd.ss_lun = diskp->dk_addr.sa_lun;
		msel_plist.res1 = 0;
		msel_plist.medtype = 0;
		msel_plist.res2 = 0;
		msel_plist.bdlen = 0;
		msel_plist.pgcode = 0;
		msel_plist.pglen = sizeof(struct pdinfo);
		msel_plist.pdinfo = diskp->dk_pdsec;

		mysbp->SCB.sc_cmdpt = (caddr_t)SCS_AD(&mselcmd);
		mysbp->SCB.sc_datapt = (caddr_t)&msel_plist;
		mysbp->SCB.sc_time = 0;
		mysbp->SCB.sc_dev = diskp->dk_addr;
		mysbp->SCB.sc_mode = SCB_WRITE;
		mysbp->SCB.sc_cmdsz = SCS_SZ;
		mysbp->SCB.sc_datasz = sizeof(struct msel_plist);
		mysbp->SCB.sc_resid = 0;

		if (sdi_translate(mysbp) != SDI_RET_OK ||
				sdi_icmd(mysbp) != SDI_RET_OK) {
#ifdef DEBUG
			cmn_err(CE_WARN, "dcd helper: unable to mode select\n");
#endif
		}

		diskp->dk_spec = diskp->dk_spec->inquiry[INQ_DEVTYPE] == 'M' ?
					&dcd_mfm_lclose : &dcd_esdi_lclose;
	}
}
