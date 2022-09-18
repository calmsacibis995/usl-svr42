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

#ident	"@(#)uts-x86at:boot/at386/sip/memsizer.c	1.4"
#ident	"$Header: $"

#include "util/types.h"
#include "svc/bootinfo.h"
#include "util/param.h"
#include "util/sysmacros.h"
#include "mem/immu.h"
#include "io/cram/cram.h"
#include "boot/bootdef.h"
#include "boot/initprog.h"

#define T_LEAP		NBPC / sizeof(long)


/*
 *	memory scanning routine
 *	testing all memory chunks except shadow memory
 */
memtest()
{

	struct 	bootmem *map, *mrp;
	ulong	memsrt;
	ulong	base_sum, ext_sum, cmos_base_sum;
	struct	sysenvmt	*sep = &btep->sysenvmt;
	int	extmem_clip = 0;
	int	i;

/*	set page boundary for base memory				*/
	base_sum = (ulong)ptalign(sep->base_mem * 1024);

/*	merging the memory used by the boot program and BIOS together	*/
	if (btep->bf_resv_base)
		base_sum = (ulong)ptalign(btep->bf_resv_base); 

/*	set total base memory from CMOS value				*/
	cmos_base_sum = (ulong)(sep->CMOSmembase * 1024);

/*	set page boundary for extended memory				*/
	ext_sum = (ulong)ptalign(sep->CMOSmemext * 1024);

#ifdef BOOT_DEBUG
	if ( btep->db_flag & (MEMDEBUG | BOOTTALK)) {
		shomem(0,"memtst: memory ranges", btep->memrngcnt,btep->memrng);
		printf("memsize: CMOS base %dKB, expansion %dKB, sys >1MB(max in PS2) %dKB\n",
			sep->CMOSmembase, sep->CMOSmemext,
			sep->CMOSmem_hi);
		printf("         BIOS base %dKB, system %dKB\n",
			sep->base_mem, sep->sys_mem);
		printf("sizer base_sum = %x, ext_sum = %x \n",
				base_sum, ext_sum);
		goany();
	}
#endif

/* 	set aside the first avail slot for use by boot 			*/
	BTEP_INFO.memavailcnt = 1;
	map = &BTEP_INFO.memavail[0];
	map->base = 0;
	map->extent = btep->bootsize;
	map++->flags |= B_MEM_BOOTSTRAP;

/*	loop through all memory ranges					*/
	mrp = &btep->memrng[0]; 
	mrp[btep->memrngcnt].extent = 0; 
	for (; mrp->extent > 0; mrp++) {

		memcpy(map, mrp, sizeof(struct bootmem));

/*		set shadow memory as boot reserved			*/
		if (map->flags & B_MEM_SHADOW) {
			map->flags |= B_MEM_BOOTSTRAP;
/*
 *		split base memory if used by BIOS/boot program memory
 *		allocator. Take the CMOS value and do no memory scan
 */
		} else if ( map->flags & B_MEM_BASE) {
			map->base = btep->bootsize;
			map->extent = base_sum - map->base;
			if ((cmos_base_sum > base_sum) &&
				(BTEP_INFO.bootflags & BF_BIOSRAM)) {
#ifdef BOOT_DEBUG
				if (btep->db_flag & BOOTTALK) {
					printf("NOTICE: Base memory used by BIOS\n");
					printf("cmos base end= 0x%x bios base end= 0x%x\n",
					cmos_base_sum, base_sum);
				}
#endif
				map++;
				BTEP_INFO.memavailcnt++;
				map->extent=ctob(btoct(cmos_base_sum-base_sum));
				map->base = (paddr_t) base_sum;
				map->flags= mrp->flags| B_MEM_BOOTSTRAP;
			}
/*		check extended memory					*/
		} else if (map->flags & B_MEM_EXPANS) { 
#ifdef BOOT_DEBUG
			if (btep->db_flag & BOOTTALK) {
				printf("base 0x%x extent 0x%x flags 0x%x\n",
				    map->base, map->extent, map->flags );
			}
#endif
			memsrt = map->base;
			if (mrp->flags & B_MEM_TREV) 
				memsrt -= NBPC;

/* 			For memory above 16M:-	
 *			no memory scan will be performed unless
 *			there must be at least 3 pages of non-wrap memory
 *			and no clipped extended memory below.
 *			If explicit memory probing is set then ignore clipped
 *			extended memory flag.
 */
			if (memsrt >= MEM16M) {
				if (BTEP_INFO.bootflags & BF_16MWRAPSET) 
					;
				else if (extmem_clip && 
					!(map->flags & B_MEM_FORCE))
					map->extent = 0;
				else {
					for (i=0; i<3; i++) {
				    		if (memwrap(MEM16M+(ulong)(i*NBPC),MEM16M))
							break;
					}
					if (i<3) 
						map->extent = 0;
				}
			} else if (map->extent > ext_sum) {
/* 			For memory below 16M:-	
 *			clip extended memory to CMOS recorded limit	
 */
				map->extent = ext_sum;
				extmem_clip++;	
			}

/*			skip if running out of extended memory		*/
			if (!map->extent) 
				continue;
	
			if (map->extent = memchk(memsrt, map->extent, map->flags)) {
				if (map->flags & B_MEM_TREV)
					map->base -= map->extent;
				if (memsrt < MEM16M) 
					ext_sum -= map->extent;
			}
		}	
		if (map->extent) {
			map++; 
			BTEP_INFO.memavailcnt++;
		}
	}
	map->extent = 0;

#ifdef BOOT_DEBUG
	if ( btep->db_flag & BOOTTALK) 
		shomem(0,"memtst: found avail memory area",BTEP_INFO.memavailcnt,BTEP_INFO.memavail);
#endif
}

/*
 *	testing shadow memory - stop when testr fails
 */
tst_shdram()
{
	struct 	bootmem *map;
	struct 	bootmem *macurp;
	struct 	bootmem *manxtp;
	ulong	memsrt;

	for (map = &BTEP_INFO.memavail[0]; map->extent > 0; map++) {

/*		skip non-shadow ram				*/
		if (!(map->flags & B_MEM_SHADOW))
			continue;

		memsrt = map->base;
		if (map->flags & B_MEM_TREV) 
			memsrt -= NBPC;

		map->extent = memchk(memsrt, map->extent, map->flags);
		if (map->extent > 0) {
			if (map->flags & B_MEM_TREV)
				map->base -= map->extent;
		} else {
/*			move all next one's up one slot			*/
			macurp = map;
			manxtp = map+1;
			for (;manxtp->extent; macurp++, manxtp++)
				memcpy(macurp,manxtp,sizeof(struct bootmem));
			map--;
			BTEP_INFO.memavail[--BTEP_INFO.memavailcnt].extent=0;
		}
	}

#ifdef BOOT_DEBUG
	if ( btep->db_flag & BOOTTALK) 
		shomem(0,"tst_shdram: found avail memory area",BTEP_INFO.memavailcnt,BTEP_INFO.memavail);
#endif
}

/*
 *	checkerboard memory tester
 */
memchk(memsrt, cnt, flag)
ulong	memsrt;
ulong	cnt;
ushort	flag;
{
	int	bytecnt = 0;
	ulong	*ta;
	ulong	memsave1;
	ulong	memsave2;

	ta = (ulong *)memsrt;
#ifdef BOOT_DEBUG
	if ( btep->db_flag & BOOTTALK)
		printf("Memory test starting %x %s cnt %x",ta,
			(flag & B_MEM_TREV)?"down":"up", cnt);
#endif

	for (; cnt; cnt-=NBPC) {
		/*
		 * Do not save previous contents of the memory probed locations
		 */
		*ta++ = MEMTEST1;
		*ta-- = MEMTEST0;
		if ( *ta == MEMTEST1 ) {
			*ta++ = MEMTEST2;
			*ta-- = MEMTEST0;
			if (*ta == MEMTEST2 ) {
				bytecnt += NBPC;
			} else {
#ifdef BOOT_DEBUG
				if ( btep->db_flag & MEMDEBUG)
					printf(" abort 2");
#endif
				break;
			}
		} else {
#ifdef BOOT_DEBUG
			if ( btep->db_flag & MEMDEBUG)
				printf(" abort 1");
#endif
			break;
		}
		if (flag & B_MEM_TREV)
			ta -= (NBPC / sizeof(long));
		else 
			ta += (NBPC / sizeof(long));
	}

#ifdef BOOT_DEBUG
	if (btep->db_flag & (BOOTTALK|MEMDEBUG))
		printf(" ended at %x, area size %x\n", ta, bytecnt);
#endif

	return(bytecnt);
}



