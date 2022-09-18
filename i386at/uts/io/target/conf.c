/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/conf.c	1.7"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include "io/target/sdi_edt.h"
#include "io/target/sdi.h"

/*
#define SDI_DEBUG	1
*/

extern void  (*sdi_rinits[])();
extern int   sdi_rtabsz;
struct owner *addrmatch();
struct owner *getowner();

struct owner *
sdi_doconfig(dev_cfg, dev_cfg_size, drvrname, drv_maj, rinit)
struct dev_cfg dev_cfg[];
int dev_cfg_size;
char *drvrname;
struct drv_majors *drv_maj;
void (*rinit)();
{
	int h, s, l, i;
	struct owner *op, *head = 0, *tail = 0;

#ifdef SDI_DEBUG
	printf("sdi_doconfig( %x, %x, %x)\n",
		dev_cfg, dev_cfg_size, drvrname, drv_maj);
#endif

	for (h=0; h < sdi_hacnt; h++) {
		for (s=0; s < MAX_TCS; s++) {
			for (l=0; l < MAX_LUS; l++) {
				if (op = addrmatch(h,s,l, dev_cfg, dev_cfg_size,
							drvrname, drv_maj)) {
					op->res1 = 0;
					if (!tail) {
						head = tail = op;
					} else {
						tail->res1 = (ulong)op;
						tail = op;
					}
				}
			}
		}
	}

#ifdef SDI_DEBUG
	printf("sdi_doconfig return %x\n", head);
#endif
	if (rinit != NULL) {
		if (dev_cfg[0].match_type & SDI_REMOVE) {
			for (i=0; i < sdi_rtabsz; i++) {
				if (sdi_rinits[i] == rinit) {
					sdi_rinits[i] = NULL;
					break;
				}
			}
		}
		else {
			for (i=0; i < sdi_rtabsz; i++) {
				if (sdi_rinits[i] == NULL) {
					sdi_rinits[i] = rinit;
					break;
				}
			}
		}
	}
	return head;
}

void
sdi_clrconfig(op, flags, rinit)
struct owner *op;
int          flags;
void 	     (*rinit)();
{
	struct	owner	*nop;
	int	i;

	for (; op != NULL; op = nop) {
		/* sdi_access() clears res1 */
		nop = (struct owner *)op->res1;
		sdi_access(op->edtp, flags, op);
	}

	if (rinit != NULL) {
		for (i=0; i < sdi_rtabsz; i++) {
			if (sdi_rinits[i] == rinit) {
				sdi_rinits[i] = NULL;
				break;
			}
		}
	}
}

struct owner *
addrmatch(h, s, l, dev_cfg, dev_cfg_size, drvrname, drv_maj)
int h, s, l;
struct dev_cfg dev_cfg[];
int dev_cfg_size;
char *drvrname;
struct drv_majors *drv_maj;
{
	int i;
	struct sdi_edt dummy_edt;
	struct sdi_edt *edtp;
	struct owner *op;
	int yes=0;
	int matchidx;

	if (!(edtp = sdi_redt(h, s, l)) ) {
		edtp = &dummy_edt;
		edtp->hba_no = h;
		edtp->scsi_id = s;
		edtp->lun = l;
		edtp->pdtype = -1;
	}

	for (i=0; i < dev_cfg_size; i++) {
		int isaddr, istype;

		isaddr =(dev_cfg[i].hba_no == 0xffff ||
			(dev_cfg[i].hba_no == h &&
				(dev_cfg[i].scsi_id == 0xff ||
				(dev_cfg[i].scsi_id == s &&
					(dev_cfg[i].lun == 0xff ||
					(dev_cfg[i].lun == l))))));

		if (!isaddr) {
			continue;
		}

		istype =(dev_cfg[i].devtype == 0xff ||
			    dev_cfg[i].devtype == edtp->pdtype) &&
			(!dev_cfg[i].inq_len ||
			    strncmp(edtp->inquiry, dev_cfg[i].inquiry,
					    dev_cfg[i].inq_len) == 0);

		if (!istype) {
			continue;
		}

		if (dev_cfg[i].match_type & SDI_REMOVE) {
			return (struct owner *)0;
		}

		if (!yes++) {
			matchidx = i;
		}
	}

	if (yes) {
#ifdef SDI_DEBUG
	printf("addrmatch: i = %x  match_idx = %x\n", i, matchidx);
	printf("dev_cfg  = %x match_type = %x\n",
		 dev_cfg, dev_cfg[matchidx].match_type  );
#endif
		op = getowner(edtp, drvrname, drv_maj,
				dev_cfg[matchidx].match_type | SDI_ADD);
		if (op) {
#ifdef SDI_DEBUG
	printf("addrmatch: return %x\n", op);
#endif
			return op;
		}
	}
	return (struct owner *)0;
}

#define MAXOWNERS	24
static struct owner ownerpool[MAXOWNERS];
static freeowner = 0;

struct owner *
getowner(edtp, drvrname, drv_maj, access)
struct sdi_edt *edtp;
char *drvrname;
struct drv_majors *drv_maj;
int access;
{
	struct owner *op;
	struct owner *alloc_ownerblk();
#ifdef SDI_DEBUG
	printf("getowner(%x, %x, %x, %x)\n",edtp, drvrname, drv_maj, access);
#endif

	if(edtp->pdtype >= 0)	{
		for(op = edtp->owner_list; op; op = op->next)	{
			if(op->maj.b_maj == drv_maj->b_maj &&
			   op->maj.c_maj == drv_maj->c_maj)	{
				break;
			}
		}
		if(op)	{
			if(edtp->curdrv != op)	{
				if(sdi_access(edtp, (access&~SDI_ADD)|SDI_CLAIM, op) != SDI_RET_OK)  {
					return (struct owner *)0;
				}
			}
			return op;
		}
	}

#ifdef OLDOWNER
	if (freeowner >= MAXOWNERS) {
		return (struct owner *)0;
	}

	op = &ownerpool[freeowner++];
#else
	if( (op = alloc_ownerblk()) == (struct owner *)NULL ) {
		return( (struct owner *)NULL );
	}
#endif

	op->name = drvrname;
	op->maj = *drv_maj;
	op->edtp = edtp;

	if (sdi_access(edtp, access, op) != SDI_RET_OK) {
#ifdef SDI_DEBUG
	printf("getowner: sdi_access failed\n");
#endif

		freeowner--;
		return (struct owner *)0;
	}

#ifdef SDI_DEBUG
	printf("getowner return %x\n", op);
#endif
	return op;
}

struct dev_spec *
sdi_findspec(edtp, dev_spec)
struct sdi_edt *edtp;
struct dev_spec *dev_spec[];
{
	register int i;

	for (i = 0; dev_spec[i]; i++) {
		if (!strncmp(dev_spec[i]->inquiry, edtp->inquiry, INQ_LEN)) {
#ifdef SDI_DEBUG
	printf("sdi_findspec: %s: return %x\n",
		 dev_spec[i]->inquiry, dev_spec[i]);
#endif
			return dev_spec[i];
		}
	}

	return (struct dev_spec *)0;
}
