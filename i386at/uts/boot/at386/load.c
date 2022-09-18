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

#ident	"@(#)uts-x86at:boot/at386/load.c	1.1"
#ident  "$Header: $"

#include "util/types.h"
#include "mem/immu.h"
#include "util/param.h"
#include "io/vtoc.h"
#include "util/sysmacros.h"
#include "svc/bootinfo.h"

#include "boot/boot.h"
#include "boot/libfm.h"
#include "boot/error.h"

extern	struct	bootenv	bootenv;

#define INTERVAL( b, e, p)	( e > 0 ? ((p >= b) && ( p < b + e)) : \
					  ((p < b) && ( p >= b + e)) )

/* standard error message; the 0L is the 'error' return code */
#define ERROR(msg, arg) (printf("\nboot: Cannot load %s: %s\n", arg, msg), FAILURE)

/* occasionally check for keyboard input to abort boot process */
#define CHECK_KBD	if (  enforce && ischar() ) return( FAILURE ) 

/* The stand-alone loader; returns a FAILURE load address in case of error */

/*	Boot loader - Loads the coff/elf file whose path is in path.
 *
 *	If enforce is TRUE, load the kernel using path into available
 *		memory as defined in the memavail array in the bootinfo
 *		structure marking used areas in the memused array.
 *
 *	else load the file for a non-virtual module at the address
 *		specified in the file but not below the address in
 *		loadaddr.
 *
 *	Return the loaded module start address or:
 *		0 - if the load interrupted by a keystroke(kernel load only).
 *		  - if the file could not be found.
 *		  - if a load error occured.
 */
unsigned long
bload(lpcbp)
struct	lpcb	*lpcbp; 
{
	register long	size;
	register int	availseg;
	paddr_t	kstart;
	paddr_t	endseg;
	int	foundbki = FALSE;
	int	foundtd = FALSE;
	short	bkiinfo = -1;
	int	i,j;
	ulong	actual;
	int	status;
	short	enforce;
	char	*path;
	paddr_t	loadaddr;

	if (lpcbp->lp_type == KERNEL)
		enforce = TRUE;
	else
		enforce = FALSE;
	path = lpcbp->lp_path;


/* 	open the file 							*/
	BL_file_open(path, NULL, &status);
	if (status == E_FNEXIST)
		return(ERROR("file not found", path));

/* 	get program headers information from file 			*/
	if ((status = getfhdr(&LP_BFTBL)) == FAILURE)
		return(FAILURE);

	CHECK_KBD;

	if ( enforce) {  /* availseg 0 is the boot so use 1  to start 	*/
		availseg=1;
		loadaddr = BTE_INFO.memavail[availseg].base;
		endseg = BTE_INFO.memavail[availseg].extent + loadaddr;
	} else
		loadaddr = (paddr_t)lpcbp->lp_memsrt;

/* 	load the program in sections/segments				*/
	for ( i = 0, kstart=-1; i < LP_BFTBL.t_nbph; ) {
/*		skip the BSS segment					*/
		if (LP_BFTBL.t_bph[i].p_type == BLOAD) {
			i++;
			continue;
		}
/*		check for BKI (boot kernel interface) version		*/
		if (LP_BFTBL.t_bph[i].p_type == BKI) {
			BL_file_lseek(LP_BFTBL.t_bph[i].p_offset, &status);
			BL_file_read(physaddr(&bkiinfo),NULL,2L,&actual,&status);
			if (status != E_OK)
				return (ERROR("cannot read BKI section", path));

			if (enforce) {
				foundbki = TRUE;
				if (bkiinfo < BKIVERSION) 
					return (ERROR("BKI too old", path));
				if (bkiinfo > BKIVERSION)
					return (ERROR("BKI too new", path));
			}
			i++;
			continue;
		}
/* 	text or data; read the file and copy into memory
 * 
 * 	find area of memory that where LP_BFTBL.t_bph[i] is supposed to live, 
 * 	and see if it fits...
 */
		CHECK_KBD;
		foundtd = TRUE;
		if (enforce) {
			while ((size = endseg - loadaddr) <= 0) {
/*				skipping memory reserved by boot prog	*/
				for (availseg++; (availseg<BTE_INFO.memavailcnt)
				     && (BTE_INFO.memavail[availseg].flags &
				     B_MEM_BOOTSTRAP); availseg++)
					;
				if (availseg >= BTE_INFO.memavailcnt)
					return(ERROR("required memory for kernel not present", path));
				loadaddr = BTE_INFO.memavail[availseg].base;
				endseg=loadaddr+BTE_INFO.memavail[availseg].extent;
			}
			if (size > LP_BFTBL.t_bph[i].p_filsz)
				size = LP_BFTBL.t_bph[i].p_filsz;
		} else {
			size = LP_BFTBL.t_bph[i].p_filsz;
			if ( LP_BFTBL.t_bph[i].p_vaddr < loadaddr )
				return(ERROR("Bad load address", path));
			loadaddr = LP_BFTBL.t_bph[i].p_vaddr;
		}

		BL_file_lseek(LP_BFTBL.t_bph[i].p_offset, &status);
		BL_file_read(loadaddr,NULL,size,&actual,&status);
		if (status != E_OK)
			return(FAILURE);	/* keypress detected */

#ifdef BOOT_DEBUG
		if (bootenv.db_flag & BOOTTALK) {
			printf("bload: type= 0x%x vaddr= 0x%x filsz= 0x%x memsz= 0x%x offset= 0x%x\n",
				LP_BFTBL.t_bph[i].p_type, 
				LP_BFTBL.t_bph[i].p_vaddr,
				LP_BFTBL.t_bph[i].p_filsz, 
				LP_BFTBL.t_bph[i].p_memsz, 
				LP_BFTBL.t_bph[i].p_offset);
			printf("loaded section[%d] at %lx, extent %lx\n", 
				i, loadaddr, size);
		}
#endif

		/* Check if start address is in this section */
		if (INTERVAL(LP_BFTBL.t_bph[i].p_vaddr, size, LP_BFTBL.t_entry)) 
			kstart = LP_BFTBL.t_entry-LP_BFTBL.t_bph[i].p_vaddr 
				+loadaddr;

		if ( enforce) {
#ifdef BOOT_DEBUG2
			if (bootenv.db_flag & BOOTTALK) {
				printf("bload: cnt= 0x%x loadaddr= 0x%x\n",
					BTE_INFO.memusedcnt, loadaddr);
			}
#endif
			BTE_INFO.memused[BTE_INFO.memusedcnt].base = loadaddr;
			BTE_INFO.memused[BTE_INFO.memusedcnt].extent =
						ctob(btoc(size));
			bootenv.sectaddr[BTE_INFO.memusedcnt] =
						LP_BFTBL.t_bph[i].p_vaddr;
			BTE_INFO.memused[BTE_INFO.memusedcnt++].flags = 
				BTE_INFO.memavail[availseg].flags |
				((LP_BFTBL.t_bph[i].p_type == TLOAD) ?
					B_MEM_KTEXT : B_MEM_KDATA);
			loadaddr = ctob(btoc(loadaddr + size));
		}
/*		filsz will drop to zero after program loading		*/
		if ((LP_BFTBL.t_bph[i].p_filsz -= size) == 0)
			i++;
		else {
			LP_BFTBL.t_bph[i].p_vaddr += size;
			LP_BFTBL.t_bph[i].p_offset += size;
		}
	}

	if ( !foundtd )
		return (ERROR("missing text or data segment", path));
	if (  enforce && !foundbki )
		return (ERROR("missing BKI segment", path));

	CHECK_KBD;

#ifdef BOOT_DEBUG2
	if (bootenv.db_flag & LOADDBG)
		printf("ent-point=0x%lx phys 0x%lx\n",LP_BFTBL.t_entry,kstart);
#endif

/* 	return start physical address for the binary 			*/
	lpcbp->lp_entry = kstart;	

/*	get start and end segment addresses for initialization programs */
	if (!enforce) {
		lpcbp->lp_memsrt = LP_BFTBL.t_entry;
/*		assume the first is the highest memory segment		*/
		lpcbp->lp_memend = LP_BFTBL.t_bph[0].p_vaddr;
		for (i=1, j=0; i<LP_BFTBL.t_nbph; i++) {
			if (lpcbp->lp_memend < LP_BFTBL.t_bph[i].p_vaddr) {
				lpcbp->lp_memend = LP_BFTBL.t_bph[i].p_vaddr;
				j=i;
			}
		}
/*		use memsz of file even though only filsz bytes 
 *		has benn loaded.
 */
		lpcbp->lp_memend = LP_BFTBL.t_bph[j].p_vaddr +
				   LP_BFTBL.t_bph[j].p_memsz;
		
	}

	return(SUCCESS);
}
