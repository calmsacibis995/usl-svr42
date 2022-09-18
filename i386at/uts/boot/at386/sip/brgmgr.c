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

#ident	"@(#)uts-x86at:boot/at386/sip/brgmgr.c	1.1"
#ident	"$Header: $"

#include "util/types.h"
#include "mem/immu.h"
#include "proc/seg.h"
#include "util/weitek.h"
#include "svc/bootinfo.h"
#include "svc/brgmgr.h"

#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"
#include "boot/sip.h"

typedef union desc_tbl {
	struct i_seg_desc  iseg_d;
	struct r_seg_desc  rseg_d;
	struct seg_desc  seg_d;
	struct i_gate_desc igate_d;
	struct r_gate_desc rgate_d;
	struct gate_desc gate_d;
} desc_tbl_ntry, *Desc_Tbl_Ptr;

extern short	kgdtlimit;
extern paddr_t	kgdtbase;
extern char	*KGDTptr;
extern short	kidtlimit;
extern paddr_t	kidtbase;
extern char	*KIDTptr;
extern char	*flatdesc;

#ifdef BOOT_DEBUG2
#define PAUSENUM	10
int	pausecnt = PAUSENUM;
#endif

paddr_t B_Virt_to_Phys();
struct	brg_ctl_blk *bcbp;

brgmgr(lpcbp)
struct	lpcb *lpcbp;
{

/* 	copy binfo down to the correct low memory location 	*/
	memcpy( BOOTINFO_LOC, &BTEP_INFO, sizeof(struct bootinfo));

/*	get bridge control block pointer				*/
	bcbp = (struct brg_ctl_blk *) SIP_kentry;
	switch(bcbp->bcb_magic){
	case BCBmagic:
		brgm_swtch(lpcbp);
		break;
	default:
		kjump((ulong)bcbp);
	}

/*	pass control to the kernel via a task switch.			*/
#ifdef BOOT_DEBUG
	printf("brgmgr: enter kernel thru task switch\n");
#endif
	kswitch();
	for (;;) {
		printf("BRGMGR: Invalid execution path. \n Reboot system.\n");
		goany();
	}
}

brgm_swtch(lpcbp)
struct	lpcb *lpcbp;
{
#ifdef BOOT_DEBUG
	brgm_prt(0);
	printf("version= 0x%x uprt= 0x%x sysenvmt= 0x%x\n", bcbp->bcb_version, bcbp->bcb_uprt, bcbp->bcb_sysenvmt);
#endif
/*	setup page table so that virtual can be converted to physical	*/
	brgm_ptab((ptbl_t *)KPTBL_LOC);

/*	check for sysenvmt address					*/
	if (bcbp->bcb_sysenvmt) 
		memcpy((ulong)B_Virt_to_Phys(bcbp->bcb_sysenvmt), 
			&(btep->sysenvmt), sizeof(struct sysenvmt));

/*	check for uprt entry point					*/
	if (bcbp->bcb_uprt) {
#ifdef BOOT_DEBUG
		printf("brgm_swtch: enter kernel thru uprt\n");
		goany();
#endif
		kjump((ulong)B_Virt_to_Phys(bcbp->bcb_uprt));
	}

	brgm_pdir((ptbl_t *)KPD_LOC, (ulong)KPTBL_LOC);
	brgm_desc((long *)SIP_fdesc_p);

}


brgm_desc(lptr)
long	*lptr;
{
	long	*gptr;
	int	n;

	Cvt_Desc_Tbl(bcbp->bcb_gdtp, bcbp->bcb_gdt_cnt);
	Cvt_Desc_Tbl(bcbp->bcb_idtp, bcbp->bcb_idt_cnt);
	Cvt_Desc_Tbl(bcbp->bcb_scallp, 1);
	Cvt_Desc_Tbl(bcbp->bcb_sigretp, 1);
#ifdef VPIX
	Cvt_Desc_Tbl(bcbp->bcb_idt2p, bcbp->bcb_idt2_cnt);
#endif

#ifdef BOOT_DEBUG2
	bdump(B_Virt_to_Phys(bcbp->bcb_gdtp),400);
	goany();
	bdump(B_Virt_to_Phys(bcbp->bcb_idtp),400);
	goany();
#endif


/* 	To switch to page mode, virtual addresses have to map 1:1 to
 *	physical. When we do the task switch, we will get a new
 *	stack. Copy our GDTs to the kernel gdt table. 
 */
	gptr = (long *)B_Virt_to_Phys(bcbp->bcb_gdtp);
#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK) 
		printf("lptr= 0x%x gptr= 0x%x\n", lptr, gptr);
#endif

	gptr += 8;
	for (n=0;n<4;n++) 
		*gptr++ = *lptr++;

/* 	makeup the descriptor table register value (48 bits) 		*/
	kgdtbase = bcbp->bcb_gdtp;
	kgdtlimit = ((bcbp->bcb_gdt_cnt * 8) - 1);
	kidtbase = bcbp->bcb_idtp;
	kidtlimit = ((bcbp->bcb_idt_cnt * 8) - 1);

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK) {
		printf("KGDT %x %x  KIGDT %x  %x\n",
			kgdtlimit,kgdtbase,kidtlimit,kidtbase);
		goany();
	}
#endif
}


brgm_ptab(ptp)
ptbl_t	*ptp;
{
	uint	v_ref;
	uint	i, n;
	uint	pgs;
	uint    bpte;

/* 	initalize the page tables to zeroes 				*/
	memset( (char *)ptp, '\0', NBPPT * 2);

/* 	for all the used segments of memory 				*/
	for ( i=0; i<BTEP_INFO.memusedcnt; i++) {

/* 		get virtual start for this area 			*/
		v_ref = (uint) ptalign(btep->sectaddr[i]);

/* 		first page table entry for this area 			*/
		bpte = (BTEP_INFO.memused[i].base & PG_ADDR) | PG_P;

/* 		find the number of pages in this segment 		*/
		pgs =  pfnum(BTEP_INFO.memused[i].extent);

#ifdef BOOT_DEBUG
		printf("Pg Tbl for used at %x, %x pages, virt %x\n",
			BTEP_INFO.memused[i].base, pgs, btep->sectaddr[i]);
#endif
/* 		for each page in the segment 				*/

		for (n=0; n<pgs; n++) {
			ptp->page[pnum(v_ref)] = bpte;
#ifdef BOOT_DEBUG2
			printf("Page table entry %x set to %x\n",
				pnum(v_ref), ptp->page[pnum(v_ref)]);
			if (--pausecnt <= 0) {
				goany();
				pausecnt = PAUSENUM * 2;
			}
#endif
			bpte  += NBPP;
			v_ref += NBPP;
		}
	}
}



brgm_pdir(pdp, kptbl_addr)
ptbl_t	*pdp;
ulong	kptbl_addr;
{
	pdp->page[0] = pdp->page[ptnum(bcbp->bcb_kvbase)] =
			B_Virt_to_Phys(bcbp->bcb_kpt0) | PG_P;
	pdp->page[ptnum(bcbp->bcb_kvsbase)] = 
			PG_P | B_Virt_to_Phys((paddr_t)kptbl_addr);
	pdp->page[ptnum(WEITEK_VADDR)] =
			B_Virt_to_Phys(bcbp->bcb_kptn) | PG_P;
#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK) {
		printf("PTNUM: kvbase= 0x%x kvsbase= 0x%x weitek_vaddr= 0x%x\n",
		ptnum(bcbp->bcb_kvbase), ptnum(bcbp->bcb_kvsbase), ptnum(WEITEK_VADDR));
		printf("The page dir entries 0 = %x, KVBASE = %x, KVSBASE = %x\n",
		pdp->page[0], pdp->page[ptnum(bcbp->bcb_kvbase)],
		pdp->page[ptnum(bcbp->bcb_kvsbase)]); 
		goany();
	}
#endif
}


/*
 * Cvt_Desc_Tbl - Convert descriptor table entry from initalized
 *                format to execution format.
 *                The entry pointer is virtual.
 */


Cvt_Desc_Tbl(v_dtp, ent_cnt)
Desc_Tbl_Ptr v_dtp;
int ent_cnt;
{
	desc_tbl_ntry  tmp_desc;
	Desc_Tbl_Ptr   dtp;
	char	*tp;

	dtp = (Desc_Tbl_Ptr) B_Virt_to_Phys( (paddr_t) v_dtp);
#ifdef BOOT_DEBUG2
	printf("Cvt descriptor table at %x virt, %x phys for %d entries\n",v_dtp,dtp,ent_cnt);
	goany();
	pausecnt = PAUSENUM;
#endif
	while (ent_cnt--) {
		tmp_desc = *dtp;
		if ((tmp_desc.gate_d.g_type & DESC_TYPE ) ||
		    ((tmp_desc.gate_d.g_type & GATE_TYPE) == 0)) {
			/* descriptor is NOT a gate type */
			dtp->rseg_d.s_b0015   = tmp_desc.iseg_d.s_b0015;
			dtp->rseg_d.s_b1623   = tmp_desc.iseg_d.s_b1623;
			dtp->rseg_d.s_b2431   = tmp_desc.iseg_d.s_b2431;
			dtp->rseg_d.s_lim0015 = tmp_desc.iseg_d.s_lim0015;
			dtp->rseg_d.s_acc2    = tmp_desc.iseg_d.s_acc2;
			dtp->rseg_d.s_acc1    = tmp_desc.iseg_d.s_acc1;
#ifdef BOOT_DEBUG2
			printf("Cvt_Desc %x  to %x  at %x\n",
				tmp_desc.seg_d.s_base,dtp->seg_d.s_base,dtp);
			printf("         %x     %x  type %x\n",
				tmp_desc.seg_d.s_limacc, dtp->seg_d.s_limacc,
				tmp_desc.gate_d.g_type);
			printf("I b0 %x, b1 %x, b2 %x, lim %x, a2 %x, a1 %x\n",
			    tmp_desc.iseg_d.s_b0015,tmp_desc.iseg_d.s_b1623,
			    tmp_desc.iseg_d.s_b2431,tmp_desc.iseg_d.s_lim0015,
			    tmp_desc.iseg_d.s_acc2,tmp_desc.iseg_d.s_acc1);
			goany();
#endif

		} else {

			/* descriptor is a gate */
			dtp->rgate_d.g_off0015 = tmp_desc.igate_d.g_off0015;
			dtp->rgate_d.g_off1631 = tmp_desc.igate_d.g_off1631;
			dtp->rgate_d.g_sel     = tmp_desc.igate_d.g_sel;
			dtp->rgate_d.g_wcount  = tmp_desc.igate_d.g_wcount;
			dtp->rgate_d.g_type    = tmp_desc.igate_d.g_type;
#ifdef BOOT_DEBUG2
			printf("Cvt_Gate %x  to %x  at %x\n",
				tmp_desc.seg_d.s_base,dtp->seg_d.s_base,dtp);
			printf("         %x     %x  type %x\n",
				tmp_desc.seg_d.s_limacc, dtp->seg_d.s_limacc,
				tmp_desc.gate_d.g_type);
			printf("I f0 %x, f1 %x, sel %x, wc %x\n",
			  tmp_desc.igate_d.g_off0015,tmp_desc.igate_d.g_off1631,
			  tmp_desc.igate_d.g_sel,tmp_desc.igate_d.g_wcount);
			goany();
#endif

		}
		dtp++;
	}
}

/*
 * B_Virt_to_Phys - Boot version of virtual to physical, uses the page
 *                  table at KPTBL_LOC which is based on the loaded
 *                  portion of the kernel ( text & data only).
 */

paddr_t 
B_Virt_to_Phys(vaddr)
paddr_t vaddr;
{
	ptbl_t	*ptp = (ptbl_t *) KPTBL_LOC;
	register paddr_t	raddr;

	if (vaddr < bcbp->bcb_kvsbase)
		raddr = vaddr;
	else
		raddr =
		(vaddr & POFFMASK) + (ptp->page[pnum(vaddr)] & PG_ADDR);
	return(raddr);
}


#ifdef BOOT_DEBUG
brgm_prt(flag)
int	flag;
{
	if (btep->db_flag & BOOTTALK) {
		printf("BRGMGR Boot table: tie_magic= 0x%x kv_base= 0x%x kvsbase= 0x%x kpt0= 0x%x kptn= 0x%x\n", bcbp->bcb_magic, bcbp->bcb_kvbase, bcbp->bcb_kvsbase, bcbp->bcb_kpt0, bcbp->bcb_kptn);
		printf("stext= 0x%x sdata= 0x%x, sbss= 0x%x df_stack= 0x%x dfstksiz= 0x%x\n", bcbp->bcb_stext, bcbp->bcb_sdata, bcbp->bcb_sbss, bcbp->bcb_df_stack, bcbp->bcb_dfstksiz);
		printf("gdtp= 0x%x cnt= 0x%x idtp= 0x%x cnt= 0x%x idt2p= 0x%x cnt= 0x%x\n", bcbp->bcb_gdtp, bcbp->bcb_gdt_cnt, bcbp->bcb_idtp, bcbp->bcb_idt_cnt, bcbp->bcb_idt2p, bcbp->bcb_idt2_cnt);
		printf("scallp= 0x%x sigretp= 0x%x\n", bcbp->bcb_scallp, bcbp->bcb_sigretp);
		goany();
	}
}
#endif

