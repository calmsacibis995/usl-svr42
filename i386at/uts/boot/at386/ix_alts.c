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

#ident	"@(#)uts-x86at:boot/at386/ix_alts.c	1.1"
#ident	"$Header: $"
#ident "@(#) (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990"

#include "util/types.h"
#include "util/param.h"
#include "util/sysmacros.h"
#include "io/vtoc.h"
#include "io/target/alttbl.h"
#include "io/target/altsctr.h"
#include "svc/bootinfo.h"
#include "io/target/fdisk.h"

#include "boot/boot.h"
#include "boot/s51kconf.h"

/*
 * Stand-alone filesystem alternate sector handling routines.
 */

#define	D_ENTRY	GBUFSZ/NBPSCTR+1
struct	d_blk {
	ulong	sec;
	ulong	mem;
	ulong	cnt;
};

struct	alts_part {
	unsigned int		ap_flag;
	struct alts_parttbl	ap_tbl;
	struct alts_ent		*ap_entp;
} alts_part;
struct	alts_part *ap = &alts_part;

struct	dpb {
	int			dpb_secsiz;
} dpb;
struct	dpb	*dpbp = &dpb;


/* global data buffer and cached block id */

extern char	gbuf[];
extern off_t	boot_delta;

extern	short	spt;		  /* disk sectors per track 		*/
extern  struct ipart *actpart_ptr;/* pointer to active partition	*/

/*
 *	get the alternate sector entry table
 */
bd_getalts(vtp, pdp)
struct	vtoc	*vtp;
struct	pdinfo	*pdp;
{
	int	i;


/*	get disk sector size						*/
	dpbp->dpb_secsiz = NBPSCTR;

/*	check for alternate sector/track partition			*/
	for (i = 0; i < (int)vtp->v_nparts; i++)
		if (vtp->v_part[i].p_tag == V_ALTSCTR)
			break;

	if (i<(int)vtp->v_nparts)
		bd_get_altsctr(vtp,pdp,&(vtp->v_part[i]));
	else
		bd_get_altsec(vtp,pdp);

#ifdef BOOT_DEBUG2
	printf ("bd_getalts:\n");
	for (i=0; i<ap->ap_tbl.alts_ent_used; i++) {
		printf ("[%d]: badsec= %d altsec= %d count= %d\n",
			i, ap->ap_entp[i].bad_start, ap->ap_entp[i].good_start, 
			(ap->ap_entp[i].bad_end - ap->ap_entp[i].bad_start + 1));
	}
#endif
}


/*
 *  dsk_xalt  -- translate AT&T alternates table into HPDD internal format
 *		 no disk read is permitted, since the global disk buffer
 *		 contains the alternate table
 */
bd_xalt(alttbl,hpdd_part,enttbl)
struct 	alt_info *alttbl;		/* AT&T disk alt structure 	*/
struct	alts_parttbl *hpdd_part;
struct 	alts_ent enttbl[];		/* alternate entry table	*/
{
	daddr_t good;
	int 	trk; 
	int	idx;
	int 	j; 
	int	entused;

/*	assume no alternate entry					*/
	hpdd_part->alts_ent_used = 0;
	hpdd_part->alts_sanity   = 0;

/*	sanity check							*/
	if (alttbl->alt_sanity != ALT_SANITY) {
#ifdef BOOT_DEBUG
		printf("dsk_xalt: Alternates table invalid.");
#endif
		return;
	}

	entused = alttbl->alt_trk.alt_used + alttbl->alt_sec.alt_used;
/*	if no bad sector found, then return 				*/
	if (!entused) 
		return;

/*	initialize the alternate partition table			*/
	hpdd_part->alts_sanity   = alttbl->alt_sanity;
	hpdd_part->alts_version  = alttbl->alt_version;
	hpdd_part->alts_map_base = 0;
	hpdd_part->alts_ent_base = 0;
	hpdd_part->alts_ent_end= byte_to_dsksec((ALTS_ENT_SIZE * entused),dpbp) 
				/ dpbp->dpb_secsiz - 1;
	hpdd_part->alts_ent_used = entused;

/* 	get base of AT&T good sectors for bad track mapping		*/
/* 	all good sectors are contiguous from here  			*/
	good = alttbl->alt_trk.alt_base;

/*  	Now process each AT&T bad track  				*/
	for (trk=0, idx=0; trk <(int)alttbl->alt_trk.alt_used; idx++, trk++) {  
		enttbl[idx].bad_start = alttbl->alt_trk.alt_bad[trk];
		enttbl[idx].bad_end   = enttbl[idx].bad_start + spt -1;
		enttbl[idx].good_start = good; 
		good = good + spt;
	}

/* 	get base of AT&T good sectors for bad sector mapping		*/
/* 	Now translate the sectors  					*/
	good = alttbl->alt_sec.alt_base;

	for (j=0; j<(int)alttbl->alt_sec.alt_used; idx++, good++, j++) {
		enttbl[idx].bad_start  = alttbl->alt_sec.alt_bad[j];
		enttbl[idx].bad_end    = enttbl[idx].bad_start;
		enttbl[idx].good_start = good; 
	}

/*	sort the alternate entry table in ascending bad sector order	*/
	dsk_sort_altsctr(enttbl,entused);
	return;
}

/*
 *	get the alternate sector entry table based on the 
 *	AT&T mapping scheme
 */
bd_get_altsec(vtp, pdp)
struct	vtoc	*vtp;
struct	pdinfo	*pdp;
{
	daddr_t	secno;
	int	i;

	/*
	 * read in alternate sector table
	 *	(Assumes alt tbl starts on sector boundary.)
	 */
	secno = actpart_ptr->relsect + (pdp->alt_ptr >> 9);
	i = (int)(pdp->alt_len + 511) >> 9;
	debug(printf("alts at sector(s) %ld to %ld\n", secno, secno + i - 1));

/*	use gbuf as a temporary storage for alternate table
 *	make no disk read until all alternate sector data have been
 *	translated into common incore alternate table
 */
	if (i > 0)
		disk(secno, physaddr(gbuf), (short)i);

/*	allocate incore alternate entry table				*/
	ap->ap_entp = (struct alts_ent *)bt_malloc(2*MAX_ALTENTS*sizeof(struct alts_ent));

/*
 *	translate from AT&T to INTERACTIVE bad sector/track 
 *	mapping scheme
 */
	bd_xalt(gbuf, &(ap->ap_tbl), ap->ap_entp);
	return;
}

/*
 *	get the alternate sector entry table based on the 
 *	INTERACTIVE alternate partition mapping scheme
 */
bd_get_altsctr(vtp, pdp, partp)
struct	vtoc	*vtp;
struct	pdinfo	*pdp;
struct	partition *partp;
{
	short	dsk_blk;
	daddr_t	alts_begsec;
	int	i;

	dsk_blk = byte_to_dsksec(ALTS_PARTTBL_SIZE, dpbp) / dpbp->dpb_secsiz;
	alts_begsec = partp->p_start;

/*	read alternate partition table					*/
	disk(alts_begsec, physaddr(gbuf), dsk_blk);
	if (((struct alts_parttbl *)gbuf)->alts_sanity != ALTS_SANITY) {
		ap->ap_tbl.alts_ent_used = 0;
		return;
	}

/*	initialize the alternate partition table			*/
	ap->ap_tbl = *(struct alts_parttbl *)gbuf;
	if (!ap->ap_tbl.alts_ent_used)
		return;

	dsk_blk =ap->ap_tbl.alts_ent_end - ap->ap_tbl.alts_ent_base + 1;
/*	allocate incore alternate entry table				*/
	ap->ap_entp = (struct alts_ent *)bt_malloc(dsk_blk * NBPSCTR);

/*	read alternate entry table					*/
	disk(alts_begsec+ap->ap_tbl.alts_ent_base, physaddr(ap->ap_entp), 
		dsk_blk);
}


/*
 * 	bubble sort the entry table into ascending order
 */
dsk_sort_altsctr(buf, cnt)
struct	alts_ent buf[];
int	cnt;
{
struct	alts_ent temp;
int	flag;
int	i,j;

	for (i=0; i<cnt-1; i++) {
	    temp = buf[cnt-1];
	    flag = 1;
	    
	    for (j=cnt-1; j>i; j--) {
		if (buf[j-1].bad_start < temp.bad_start) {
		    buf[j] = temp;
		    temp = buf[j-1];
		} else {
		    buf[j] = buf[j-1];
		    flag = 0;
		}
	    }
	    buf[i] = temp;
	    if (flag) break;
	}

}

/*
 *	read all disk blocks of the given logical block number
 */
bd_rd_sec(secno, totcnt)
daddr_t	secno;
ulong	totcnt;
{
	int	i;
	struct	d_blk d_entry[D_ENTRY];

/*	reset the disk block entry array				*/
	for (i=0; i< D_ENTRY; i++)
		d_entry[i].sec = -1;
/*	initialize the first entry of disk block to request block	*/
	d_entry[0].cnt = totcnt;
	d_entry[0].sec = secno; 
	d_entry[0].mem = physaddr(gbuf);
/*	perform bad sector remap					*/
	bd_alt_badsec(d_entry);

/*	get all disk blocks						*/
	for (i=0; d_entry[i].sec != -1; i++)
		disk(d_entry[i].sec, d_entry[i].mem, (ushort) d_entry[i].cnt);

}


/*
 *	dsk_alt_badsec remaps the bad sectors to alternates.
 *	There are 7 different cases when the comparison is made
 *	between the bad sector cluster and the disk section.
 *
 *	bad sector cluster	gggggggggggbbbbbbbggggggggggg
 *	case 1:			   ddddd
 *	case 2:				   -d-----
 *	case 3:					     ddddd
 *	case 4:			         dddddddddddd
 *	case 5:			      ddddddd-----
 *	case 6:			           ---ddddddd
 *	case 7:			           ddddddd
 *
 *	where: g = good sector,      b = bad sector
 *	       d = sector in disk section
 *             - = disk section may be extended to cover those disk area
 */
bd_alt_badsec(d_entry)
struct	d_blk d_entry[];
{
	struct	alts_ent *altp;
	struct	d_blk *d_entp;
	struct	d_blk *d_blkp;
	long	alts_used;
	ushort	secsiz;
	daddr_t	lastsec;
	int	i;

#ifdef BOOT_DEBUG2
	int	flag_b = 1;
	int	flag_e = 0;
#endif

	d_entp = d_entry;
	secsiz = dpbp->dpb_secsiz;
	alts_used = ap->ap_tbl.alts_ent_used;
	altp = ap->ap_entp;
	lastsec = d_entp->sec + d_entp->cnt - 1;

	for (i=0; i<alts_used; ) {
/*	CASE 1:								*/
		while (lastsec < altp->bad_start) {
			d_entp++;
			if (d_entp->sec != -1)
				lastsec = d_entp->sec + d_entp->cnt - 1;
			else
				break;
		}
		if (d_entp->sec == -1) break;

/*	CASE 3:								*/
		if (d_entp->sec > altp->bad_end) {
			i++;
			altp++;
			continue;
		}

#ifdef BOOT_DEBUG2
	if (flag_b) {
		flag_b = 0;
		flag_e = 1;
		printf("***** Entering bd_alt_badsec:\n ");
		for (i=0; d_entry[i].sec != -1; i++)
		printf("[%d]: sec= %d mem= %d cnt= %d\n", i, d_entry[i].sec,
			d_entry[i].mem, d_entry[i].cnt);
	}
#endif
/*	CASE 2 and 7:							*/
		if ((d_entp->sec >=altp->bad_start) &&
		    (lastsec <= altp->bad_end)) {
#ifdef BOOT_DEBUG2
	printf("bd_alt_badsec: CASE 2 and 7.\n");
#endif
			d_entp->sec = altp->good_start + d_entp->sec - 
					altp->bad_start;			
			d_entp++;
			if (d_entp->sec != -1) {
				lastsec = d_entp->sec + d_entp->cnt - 1;
				continue;
			}
			else break;
		}
		d_blkp = d_entp + 1;
		
/*	CASE 6:								*/
		if ((d_entp->sec <= altp->bad_end) &&
		    (d_entp->sec >= altp->bad_start)) {
#ifdef BOOT_DEBUG2
	printf("bd_alt_badsec: CASE 6.\n");
#endif
			d_blkp->cnt = d_entp->cnt - (altp->bad_end -
					d_entp->sec + 1);
			d_entp->cnt -= d_blkp->cnt;
			d_blkp->sec = altp->bad_end +1;
			d_blkp->mem = d_entp->mem + d_entp->cnt * secsiz;
			d_entp->sec = altp->good_start + d_entp->sec - 
					altp->bad_start;
			d_entp++;
			continue;
		}

/*	CASE 5:								*/
		if ((lastsec >= altp->bad_start) && (lastsec<=altp->bad_end)) {
#ifdef BOOT_DEBUG2
	printf("bd_alt_badsec: CASE 5.\n");
#endif
			d_blkp->cnt = lastsec - altp->bad_start + 1;
			d_entp->cnt -= d_blkp->cnt;
			d_blkp->sec = altp->good_start;
			d_blkp->mem = d_entp->mem + d_entp->cnt * secsiz;
			break;
		}

/*	CASE 4:								*/
#ifdef BOOT_DEBUG2
	printf("bd_alt_badsec: CASE 4.\n");
#endif
		d_blkp->sec = altp->good_start;
		d_blkp->cnt = altp->bad_end - altp->bad_start + 1;
		d_entp->cnt = altp->bad_start - d_entp->sec;
		d_blkp->mem = d_entp->mem + d_entp->cnt * secsiz;
		d_entp++;
		d_blkp++;
		d_blkp->cnt = lastsec - altp->bad_end;
		d_blkp->sec = altp->bad_end + 1;
		d_blkp->mem = d_entp->mem + d_entp->cnt * secsiz;
		d_entp++;
	}

#ifdef BOOT_DEBUG2
	if (flag_e) {
		for (i=0; d_entry[i].sec != -1; i++)
			printf("[%d]: sec= %d mem= %d cnt= %d\n", i, 
				d_entry[i].sec, d_entry[i].mem, d_entry[i].cnt);
		printf("****** Leaving bd_alt_badsec.\n\n");
	}
#endif
}

