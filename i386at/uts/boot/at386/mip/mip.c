/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
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

#ident	"@(#)uts-x86at:boot/at386/mip/mip.c	1.2"
#ident	"$Header: $"

#include "util/types.h"
#include "util/param.h"
#include "svc/bootinfo.h"
#include "boot/initprog.h"
#include "boot/mip.h"
#include "boot/libfm.h"

extern	int	at386();
extern	int	mc386();
extern	int	compaq();
extern	int	dell();
extern	int	olivetti();
extern	int	apricot();
extern	int	necpm();
extern	int	intel();
char		*membrk();

struct machconfig mconf[] = {
	(char *)NULL,    0, "AT386", 0,  MPC_AT386, 0, at386,
/*
	comment out architecture specific mip code until we can
	determine what is save. We know reclaiming shadow ram
	isn't.

	(char *)NULL,    0, "MC386", 0, MPC_MC386, 0, mc386,
	(char *)0xfffea, 6, "COMPAQ", 0, MPC_COMPAQ,0,compaq, 
	(char *)0xfe076, 4, "Dell", 0, MPC_DELL,0,dell, 
	(char *)0xfe017, 8, "OLIVETTI", 0, MPC_M380,0,olivetti, 
	(char *)0xfe089, 7, "Apricot", 0, MPC_APRICOT,0,apricot, 
	(char *)0xfe077, 3, "NEC", 0, MPC_NECpwrmate,M_FLG_SRGE,necpm,
*/

/*	NOTICE: Intel test must remain in this order tho not last */
	(char *)0xfed00, 4, "IDNO", 0, MPC_INTEL30X,0,intel,
	(char *)0xf4000, 8, "INTEL386", 0, MPC_I386PSA,M_FLG_SRGE,intel
/*	(char *)0xfe076,10,"Advanced L", 0, MPC_ALR,0,alr   */
	};
#define	NSIGS	(sizeof(mconf)/sizeof(struct machconfig))


struct 	bootfuncs *bfp;
struct 	bootenv *btep;

mip_start(binfop, bfuncp, cmd, lpcbp)
struct 	bootenv *binfop;
struct 	bootfuncs *bfuncp;
int	cmd;
struct	lpcb *lpcbp;
{
int	mach_id = -1;

/*	setup the global pointers for bootinfo and bootfuncs		*/
	bfp = bfuncp;
	btep = binfop;

	switch (cmd) {
		case MIP_INIT:
			btep->sysenvmt.machine = MPC_UNKNOWN;
			mach_id = identify();
			lpcbp->lp_pptr = (char *) &mconf[mach_id];
			MIP_init = (ulong) mconf[mach_id].m_entry;
			((int (*)()) MIP_init) (lpcbp);
			break;
		case MIP_END:
			if (MIP_end != (ulong) NULL)
				((int (*)()) MIP_end) ();
			break;
	}
}


identify()
{
	int	i;
	unchar	mb;
	struct	sys_desc *sdp;
	struct	int_pb	ic;
	int	mach_id;

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK)
		printf("Begin machine identification.\n");
#endif

	ic.intval = 0x15;
	ic.ax = 0xC000;
	if (doint(&ic)) 
		mb = SYS_MODEL();
	else {
		i = (ic.es & 0xFFFF) << 4;
		sdp = (struct sys_desc *) ( i + (ic.bx & 0xFFFF));
		mb = sdp->sd_model;
		btep->sysenvmt.sys_desc = *sdp;

	}
	if ( mb == MODEL_MC ) {
		btep->sysenvmt.machflags |= MC_BUS;
		mach_id = M_ID_MC386;
	} else {
		btep->sysenvmt.machflags |= AT_BUS;
		mach_id = M_ID_AT386;
	}

	for ( i=2; i < NSIGS; i++) {
		if ( mconf[i].m_flag & M_FLG_SRGE) {
			if ( (membrk(mconf[i].sigaddr, mconf[i].sigid,
				0x200, mconf[i].siglen)) != 0) 
				break;
		} else {
			if ( (memcmp(mconf[i].sigaddr, mconf[i].sigid,
				mconf[i].siglen)) == 0) 
				break;
		}
	}
	if ( i < NSIGS)
		mach_id = i;

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK)
		printf("Machine identified as %s.\n", mconf[mach_id].sigid);
#endif

	btep->sysenvmt.machine = mconf[mach_id].machine;
	return(mach_id);
}

char *
membrk(s1,s2,n1,n2)
char	*s1, *s2;
int	n1, n2;
{
	char	*os = s1;
	int	n;

	for (n = n1 - n2 ; n >= 0; n--) {
		if (memcmp(s1++, s2, n2) == 0) {
/*
			printf("Ident range chk string %s found at %x\n",
				s2,s1);
*/
			return(s1);
		}
	}
	return(0);
}

