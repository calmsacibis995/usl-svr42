/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:svc/startup.c	1.20"
#ident	"$Header: $"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <svc/systm.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <mem/seg.h>
#include <svc/errno.h>
#include <proc/proc.h>
#include <util/map.h>
#include <fs/buf.h>
#include <proc/reg.h>
#include <io/tty.h>
#include <util/var.h>
#include <util/debug.h>
#include <util/cmn_err.h>

#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <proc/mman.h>

#include <fs/vnode.h>
#include <proc/session.h>
#include <mem/kmem.h>

#include <fs/file.h>
#include <io/uio.h>
#include <io/conf.h>

#include <svc/bootinfo.h>
#include <svc/brgmgr.h>
#include <util/kdb/kdebugger.h>	/* for kdb_installed */
#include <util/mod/mod_k.h>

#include <mem/vm_hat.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/seg_vn.h>
#include <mem/seg_u.h>
#include <mem/seg_map.h>
#include <mem/faultcatch.h>
#include <mem/page.h>

#include <mem/rdma.h>	/* for RESTRICTED_DMA support */

#include <io/ansi/at_ansi.h>
#include <io/kd/kd.h>
#ifdef	EISA
#include <svc/eisa.h>
#endif	/* EISA */

extern dev_t	rootdev;
extern dev_t	swapdev;
extern int	lotsfree;
extern int	syssegsz;
extern int	piosegsz;
extern int	segmapsz;
extern char piosegs[];


pte_t	*kptbl;		/* points to dynamically-allocated kernel page table */
pte_t	*piownptbl;	/* points to page table for piosegs */

int	physmem;
uint	bmemsize = 0;	/* MUST initialize to keep it out of BSS */

paddr_t symbase = 0;	/* physical base address of relocated symbol table */
STATIC int sflags = 0;	/* flags field associated with relocated symbol table memory */

uint firstfree;	 	

struct gdscr def_intf0;   /* default handler for int 0xf0, ... 0xff */

extern pte_t  kpd0[];
extern pte_t  kpt0[];
extern pte_t  kspt0[];

extern void adt_init();
STATIC void mktables(unsigned int);
STATIC void kvm_init(unsigned int);

extern char	stext[];	/* Start of kernel text. */
extern char	sdata[];	/* Start of kernel data. */
extern char	sbss[];		/* Start of kernel bss. */
extern char	edata, end;
extern char	_start;

extern ulong	egafontptr[5];
unchar	*egafont_p[5] = { 0 };

			/* Chunks of physical memory that are NOT used for 
			 * kernel text and data.  We start allocating kernel
			 * bss from memNOTused[], so eventually entries in
			 * memNOTused[] actually refer to "used" memory.
			 *
			 * Note that memNOTused[] cannot be in bss.
			 */
struct	bootmem	memNOTused[B_MAXARGS + B_MAXARGS] = {0,0,0};
unsigned		memNOTusedcnt = 0;

unsigned		memused_idx = 0;
unsigned		minkpclick;	  /* min click for kernel page pool */

int			checksumOK = 0;	  /* flag for checksum */ 

/*
 *    maxclick - The largest physical click number.
 *               ctob(maxclick) is the largest physical
 *               address configured plus 1.
 */
int   maxclick;


/* Routine called before main, to initialize various data structures.
*/

mlsetup()
{
	register unsigned	nextclick;
	register paddr_t	free_paddr;
	unsigned		firstclick, lastclick;
	unsigned char		ob;
	unsigned		memsize, bss_size, core_size, i;
	pte_t			*pt;
	paddr_t			addr;
	extern struct map piomap[];
	extern int piomapsz;
	unchar *from, *to;
	int    cnt;
	int	binfosum;
	caddr_t	caddr;
	struct brg_ctl_blk	*bcbp;

#ifdef STARTUP_DEBUG
	ml_printf("Entering mlsetup.\n");
#endif

	/* calculate the checksum in the bootinfo structure */
	binfosum = 0;
	for ( i = 0; i < (sizeof(struct bootinfo) - sizeof(int)); i++ )
		binfosum += ((char *)&bootinfo)[i];

	if ( binfosum == bootinfo.checksum )
		checksumOK = 1;

	/*
 	 * Retrieve information passed from boot via the bootinfo struct.
 	 */
	bootarg_parse();

        bcbp = (struct brg_ctl_blk *)(&_start);

#ifdef STARTUP_DEBUG
        ml_printf("mlsetup: bcbp= 0x%x magic= 0x%x\n", bcbp, bcbp->bcb_magic);
#endif

        switch(bcbp->bcb_magic) {
      	/*      boot program with bridge manager support  		*/
        case BCBmagic:
	        ml_swtch_brgmgr(bcbp);
	      	break;
      	/*      boot program with no bridge manager support enter
      	 *      through uprt entry point
       	 */
        default:
		ml_swtch_uprt();
		break;
	}

	ml_kprep();
}


/*
 * Boot and kernel interface using the bridge manager
 */
STATIC int
ml_swtch_brgmgr(bcbp)
struct brg_ctl_blk	*bcbp;
{

	int	i;
#ifdef STARTUP_DEBUG
	ml_printf("Starting ml_swtch_brgmgr, sysenvmt.machflags= 0x%x\n",
	sysenvmtp->machflags);
#endif

/*	check for valid uprt entry point				*/
	if (bcbp->bcb_uprt) {
/*		boot program enter kernel through the uprt entry point	*/
		ml_swtch_uprt();
		return;
	}

	/*
	 * transfer the BIOS/ROM font locations stored in system
	 * environment structure into the kernel font pointers.
	 */

	for (i = 0; i < MX_FTP; i++)
		egafont_p[i] = (unchar *)sysenvmtp->font_ptr[i];
}

/*
 * Kernel entered through the uprt entry point
 */
STATIC int
ml_swtch_uprt()
{
	int	i;

#ifdef STARTUP_DEBUG
	ml_printf("Starting ml_swtch_uprt\n");
#endif

	/*
	 * Store the ROM font locations we got from the bios
	 * in uprt.s into the driver's font pointers.
	 */
	for (i = 0; i < 5; i++)
		egafont_p[i] = (unchar *)phystokv(ftop(egafontptr[i]));
}

/* return index of bootinfo.memavail entry in which addr is contained 
   save index from last call as an optimization */
STATIC int
ml_availind(paddr_t addr)
{
	static int saveind = 0;
	int osaveind;

	if(addr >= bootinfo.memavail[saveind].base &&
		addr < bootinfo.memavail[saveind].base + 
		       bootinfo.memavail[saveind].extent) {
#ifdef STARTUP_DEBUG
		ml_printf("ml_availind: avail index for 0x%x is %d\n",addr,saveind);
		ml_pause();
#endif
		return(saveind);
	}
	osaveind = saveind;
	for(saveind = saveind+1; saveind != osaveind ; saveind++) {
		if(saveind >= bootinfo.memavailcnt) {
			saveind = -1;
			continue;
		}
		if(addr >= bootinfo.memavail[saveind].base &&
			addr < bootinfo.memavail[saveind].base + 
			       bootinfo.memavail[saveind].extent) {
#ifdef STARTUP_DEBUG
			ml_printf("ml_availind: avail index for 0x%x is %d\n",addr,saveind);
			ml_pause();
#endif
			return(saveind);
		}
	}
	cmn_err(CE_PANIC,"Used memory address not in memavail.\n");
}

/*
 * Adds memory defined by pbase, extent and bootinfo.memavail[aindex].flags
 * to memNOTused array.  NOTE: memNOTused[memused_idx].base may change.
 */

STATIC void
addtomemNU(paddr_t pbase, size_t extent, unsigned int aindex)
{
	int i, j;

	/* look for a block of memory already in the array with which to combine */
	for(i = 0; i < memNOTusedcnt; i ++) {

		/* make sure that flags are the same */
		if(bootinfo.memavail[aindex].flags == memNOTused[i].flags) {

			/* does the memory immediately precede the block? */
			if(pbase + extent == memNOTused[i].base) {
				memNOTused[i].base = pbase;
				memNOTused[i].extent += extent;
				goto NUout;
			}
			/* does the memory follow directly the block? */
			if(pbase == memNOTused[i].base + memNOTused[i].extent) {
				memNOTused[i].extent += extent;

				/* see if can be combined with the next one as well,
				   if happen to be in order - which will be the normal 
				   case */
				if( ++i < memNOTusedcnt && 
				    bootinfo.memavail[aindex].flags == memNOTused[i].flags &&
				    pbase + extent == memNOTused[i].base) {
					memNOTused[i-1].extent += memNOTused[i].extent;
					for(i++; i < memNOTusedcnt; i++)
						memNOTused[i-1] = memNOTused[i];
					memNOTusedcnt -=1;
				}
				goto NUout;
			}
		}
	}

	/* no more space in the memNOTused array - very unlikely */
	if(memNOTusedcnt >= B_MAXARGS+ B_MAXARGS)
		cmn_err(CE_PANIC, "addtomemNU: Overflow of memory array.\n");

	/*
	 * Add the new segment into the memNOTused array in ascending base
	 * address order.
	 */
	for (i = 0; i < memNOTusedcnt; i++) {
		if (memNOTused[i].base > pbase) {
			for (j = memNOTusedcnt; j-- != i;)
				memNOTused[j+1] = memNOTused[j];
			break;
		}
	}
	memNOTused[i].base = pbase;
	memNOTused[i].extent = extent;
	memNOTused[i].flags = bootinfo.memavail[aindex].flags;
	memNOTusedcnt++;
NUout:
	;
#ifdef STARTUP_DEBUG
	for(i = 0; i < memNOTusedcnt; i++)
		ml_printf("memNOTused: base 0x%x, extent 0x%x, flags 0x%x\n",
				memNOTused[i].base,
				memNOTused[i].extent,
				memNOTused[i].flags);
	ml_pause();
#endif
}


/* relocate the the memory specified by vbase and size to the last available
memory (caller must specify if want dmaable or non-dmaable memory) */
STATIC paddr_t
ml_reloc(boolean_t dmaflag, size_t size, caddr_t vbase)
{
	int freeseg;
	int i;
	paddr_t oldbase, newbase, tbase, runbase;
	pte_t *pt;
	unsigned int pfn;
	int aindex, naindex;

	/* look for free memory of the right specs for the relocation */
	for (freeseg = memNOTusedcnt - 1; freeseg >= 0; freeseg--) {

		if(memNOTused[freeseg].extent >= size && 
		    ((memNOTused[freeseg].flags & B_MEM_NODMA) == 
					    (dmaflag ? 0 : B_MEM_NODMA))) {
		    	/* found it */
		    	break;
		}
	}

#ifdef STARTUP_DEBUG
	ml_printf("ml_reloc: Relocating vbase 0x%x to memNOTused[%d]\n",vbase, freeseg);
	ml_pause();
#endif
	if(freeseg < 0)
		return(NULL);

	/* add relocated info to memory from end of this memNOTused segment */
	newbase = memNOTused[freeseg].base + memNOTused[freeseg].extent - size;

	/* remove used memory from memNOTused array */
	memNOTused[freeseg].extent -= size;

	/*  add soon to be freed memory to memNOTused array: This is complicated
	   because the virtual space vbase, size may be mapped to non-contiguous
	   physical memory. Hence, the physical addresses are checked a page at 
	   a time and the largest possible blocks of memory is returned to
	   memNOTused.

	   pt - the page table entry for the next virtual address
	   oldbase - the physical address at which this block of memory started
	   aindex - the index in the memavail array of oldbase
	   runbase - the physical address of the next page following contiguously
	   tbase - the physical address of the next page of the memory to be relocated
			as specified in the page table
	   naindex - the index in the memavail array of tbase */

	pt = kspt0 + btoct(vbase - KVSBASE);
	oldbase = pt->pgm.pg_pfn << PNUMSHFT;
	aindex = ml_availind(oldbase);
	runbase = oldbase + ctob(1);
	pt++;

	for(i = btoc(size) -1; i > 0; pt++, i--) {
		tbase = pt->pgm.pg_pfn << PNUMSHFT;
		naindex = ml_availind(tbase);

		/* if the next physical address in the page table is different from
		   the next contigous physical address or the index into
		   the memavail array changes, the block of memory to be added
		   to memNOTused ends here */
		if(runbase != tbase || naindex != aindex) {
			addtomemNU(oldbase, runbase - oldbase, aindex);
			aindex = naindex;
			oldbase = tbase;
			runbase = oldbase;
		}

		runbase = runbase + ctob(1);
	}

	/* add the last block of memory */
	addtomemNU(oldbase, runbase - oldbase, aindex);

	/* Make a temporary map to the relocation memory so we can copy
	   -kpt0 is the page table for the memory starting at KVBASE
	   -The old pfn numbers do not have to be restored when we
	   are done because a subsequent call to scanmem from ml_kprep will
	   remap these addresses in the linear map starting at KVBASE. */

	pt = kpt0 + btoct(vbase - KVSBASE);
	pfn = PFNUM(newbase);

	for(i = btoc(size); i-- > 0; )
		(pt++)->pgm.pg_pfn = pfn++;
	flushtlb();

	/* copy the data */
	bcopy(vbase, vbase - KVSBASE + KVBASE, size);

	/* Remap the correct virtual addresses to the new data location */
	pt = kspt0 + btoct(vbase - KVSBASE);
	pfn = PFNUM(newbase);
	for(i = btoc(size); i-- > 0; ) 
		(pt++)->pgm.pg_pfn = pfn++;
	flushtlb();
	return(newbase);
}

/*
 *	Kernel environment preparation
 */
STATIC int
ml_kprep()
{
	register unsigned	nextclick;
	register paddr_t	free_paddr;
	unsigned int		firstclick, lastclick;
	unsigned char		ob;
	unsigned int		memsize, bss_size, i;
	unsigned int		maxpmem;
	unsigned int		highindex;
	paddr_t			highbase;
	pte_t			*pt, *dpt;
	paddr_t			addr;
	extern struct map	piomap[];
	extern int		piomapsz;
	unchar *from, *to;
	int    cnt;
	int	binfosum;
	caddr_t caddr;

#ifdef STARTUP_DEBUG
	for (i=0; i<5; i++)
		ml_printf("egafont_p[%d] 0x%x\n", i, egafont_p[i]);
#endif

	/* On an AT386 machine, this is the size (in K) of "base" memory */
	bmemsize = bootinfo.memavail[0].extent >> 10; 

	/*
	 * Make sure memavail[] memory chunks are page-aligned; also
	 * remove memory exceeding v.v_maxpmem, if set -- assumes to this 
	 * point memory is used strictly in the order of memavail.
	 */
	memsize = 0;
	maxpmem = ctob(v.v_maxpmem);
	for (i = 0; i < bootinfo.memavailcnt; i++) {
		bootinfo.memavail[i].extent =
			ctob(btoc(bootinfo.memavail[i].extent));
		bootinfo.memavail[i].base =
			ctob(btoct(bootinfo.memavail[i].base));

		if(maxpmem && 
		    (memsize += bootinfo.memavail[i].extent) > maxpmem) {
		     	bootinfo.memavail[i].extent -= memsize - maxpmem;
		     	if(bootinfo.memavail[i].extent == 0)
				bootinfo.memavailcnt = i;
		     	else
				bootinfo.memavailcnt = i+1;
			break;
		}
	}

	/* Make sure memused[] memory chunks are page-aligned */
	for (i = 0; i < bootinfo.memusedcnt; i++) {
		bootinfo.memused[i].extent =
			ctob(btoc(bootinfo.memused[i].extent));
		bootinfo.memused[i].base =
			ctob(btoct(bootinfo.memused[i].base));

		/*
		 * Check if used memory is still in avail after trimming,
		 * if done; ml_availind panics if it is not.
		 */
		if(maxpmem) {
			(void) ml_availind(bootinfo.memused[i].base);
			(void) ml_availind(bootinfo.memused[i].base + 
					   bootinfo.memused[i].extent-1);
		}
	}

#ifdef STARTUP_DEBUG
        ml_printf("rootdev= 0x%x bmemsize= 0x%x\n", rootdev, bmemsize);
        ml_printf("mlsetup: memory available\n");
        for (i = 0; i < bootinfo.memavailcnt; i++) {
                ml_printf("Base= 0x%x Extent= 0x%x\n", bootinfo.memavail[i].base, bootinfo.memavail[i].extent);
        }
        ml_printf("mlsetup: memory used\n");
        for (i = 0; i < bootinfo.memusedcnt; i++) {
                ml_printf("Base= 0x%x Extent= 0x%x\n", bootinfo.memused[i].base, bootinfo.memused[i].extent);
        }
        ml_pause();
#endif

	/*
	 * Determine which portions of memavail[] are NOT already in
	 * memused[].  
	 */
	ml_unused_mem();

	/*
	 * Handle relocation of kernel text in special memory, if possible.
	 * After this point, memused is no longer accurate as to location
	 * of used memory.
	 */
	(void) ml_reloc(B_FALSE, sdata - stext, stext);

	if(mod_obj_kern != NULL) {
		/* move the symbol table if paging is desired */
		if((mod_obj_pagesymtab == MOD_FPAGE || 
			(!kdb_installed && mod_obj_pagesymtab == MOD_PAGE)) &&
			((symbase = ml_reloc(B_TRUE,mod_obj_size, (caddr_t) mod_obj_kern)) 
				  	!= NULL))  {
				mod_obj_pagesymtab = MOD_FPAGE;
		} else {
			/*
			 * Move the symbol table to non-dma space if available
			 * so DMA space is available for other things.
			 */
		   	(void) ml_reloc(B_FALSE, mod_obj_size, (caddr_t) mod_obj_kern);
		   	mod_obj_pagesymtab = MOD_NOPAGE;
		}
	} else
		mod_obj_pagesymtab = MOD_NOPAGE;
#ifdef STARTUP_DEBUG
	if(mod_obj_pagesymtab == MOD_FPAGE) {
		ml_printf("symbol table relocated successfully to 0x%x\n",symbase);
	} else
		ml_printf("symbol table not paged\n");
	ml_pause();
#endif
	

	free_paddr = memNOTused[0].base; /* first free physical addr */

	/* Allocate space for the BSS and set up kernel address page table */

	bss_size = btoc(&end) - btoct(sbss);	/* size of bss (clicks) */
	pt = kspt0 + btoct(sbss - KVSBASE);	/* ptr to bss ptes in kspt0[] */

#ifdef STARTUP_DEBUG
        ml_printf("bss_size= 0x%x kspt0= 0x%x pt= 0x%x sbss= 0x%x \n", bss_size, kspt0, pt, sbss);
        ml_printf("&end= 0x%x\n", &end);
        ml_pause();
#endif

	for (i = memsize = 0; bss_size-- > 0;) {
		if (i >= memNOTusedcnt)
			cmn_err(CE_PANIC, "mlsetup: not enough physical mem\n");
		if (free_paddr >= memNOTused[i].base +
						memNOTused[i].extent) {
			/* crossing unused mem boundary in memNOTused[] */
			memsize += memNOTused[i].extent;
			++i;
			free_paddr = memNOTused[i].base;
		}
#ifdef STARTUP_DEBUG
                ml_printf("pt= 0x%x free_paddr= 0x%x\n", pt, free_paddr);
#endif
		(pt++)->pg_pte = mkpte(PG_V, PFNUM(free_paddr));
		free_paddr += NBPC;
	}

	memused_idx = i;/* Save index into memNOTused[], so we know
			 * which memNOTused[] entries are (by now) actually
			 * used.
			 */
	if (free_paddr >= memNOTused[memused_idx].base
				+ memNOTused[memused_idx].extent) {
		/* Current free_paddr is crossing unused mem boundary in 
		 * memNOTused[].  Make free_paddr point to the next actual
		 * unused physical address.
		 */
		memused_idx++;
		free_paddr = memNOTused[memused_idx].base;
	}

	flushtlb();

#ifdef STARTUP_DEBUG
        ml_printf("memused_idx= 0x%x free_paddr= 0x%x\n", memused_idx, free_paddr);
        ml_pause();
#endif

	/*	Zero all BSS.
	 */
	bzero(sbss, (&end - sbss + 3) & ~3);

	/*	Set up memory parameters.
	 */

	nextclick = firstfree = btoct(free_paddr);

	physmem = 0;

	/* 
	 * Set maxclick according to the bootinfo memavail[].
	 */
	highbase = 0;
	highindex = 0;
	for(i = 0; i < bootinfo.memavailcnt; i++) 
		if(bootinfo.memavail[i].base > highbase) {
			highbase = bootinfo.memavail[i].base;
			highindex = i;
		}

	maxclick = 
		btoct(bootinfo.memavail[highindex].base
			+ bootinfo.memavail[highindex].extent);

#ifdef STARTUP_DEBUG
        ml_printf("nextclick= 0x%x firstfree= 0x%x free_paddr= 0x%x\n", nextclick, firstfree, free_paddr);
        ml_printf("maxclick= 0x%x\n", maxclick);
        ml_pause();
#endif

	/* Unfortunately, for AT386 we have to support device drivers
		which assume the device memory at bmemsize to 1M is always
		mapped at KVBASE + bmemsize. */
	nextclick = scanmem(nextclick, btoct(bmemsize*1024), btoct(1024*1024));
	physmem = 0;	/* Don't count device memory in physmem */

	/* Set up linear map for all real memory */
	for (i=0; i < bootinfo.memavailcnt && bootinfo.memavail[i].extent;++i) {
		firstclick = btoct(bootinfo.memavail[i].base);
		lastclick = btoct(bootinfo.memavail[i].base +
						bootinfo.memavail[i].extent);
		nextclick = scanmem(nextclick, firstclick, lastclick);
	}


	/* After we have come up we no longer need addresses at linear 0. */
	/* Make them illegal.  These addresses were needed during the     */
	/* initial code in uprt.s as linear must equal physical for the   */
	/* <cs,pc> prior to the jmp.  Setting linear addresses at 0 to    */
	/* invalid breaks pmon.                                           */

	kpd0[0].pg_pte = mkpte(0, 0);

	flushtlb();

	bzero((caddr_t)phystokv(ctob(nextclick)),
	      ctob(btoct(memNOTused[memused_idx].base +
			   memNOTused[memused_idx].extent) - nextclick));

	for (i = memused_idx+1; i < memNOTusedcnt; i++) {
		bzero((caddr_t)phystokv(memNOTused[i].base),
		      memNOTused[i].extent);
	}

	/*
	 * Allocate u block for proc 0.
	 */

	nextclick = p0u(nextclick);

	/*	Initialize memory mapping for sysseg.
	**	the segment from which the kernel
	**	dynamically allocates space for itself.
	*/

	nextclick = sysseginit(nextclick);

	/*	Initialize the pio window segments.
	*/

	nextclick = pioseginit(nextclick);


	/*	Initialize the map used to allocate
	**	pio virtual space.
	*/

	mapinit(piomap, piomapsz);
#ifdef STARTUP_DEBUG
	ml_printf("piomapped\n");
#endif
	mfree(piomap, piosegsz, btoc(piosegs));

#ifdef STARTUP_DEBUG
        ml_printf("piomap= 0x%x piosegs= 0x%x.\n", piomap, piosegs);
#endif

	/*
	** Allocate page table for kvsegmap.
	*/

	/* Initialize kas stuff. At this point kvseg is initialized, hence calls
	 * to sptalloc now becomes valid - henceforth. memNOTused and therefore
	 * nextclick, nclick_rtn and non_dma_page are also invalid 
	 * after mktables is called in kvm_init.
	 */

	(void) kvm_init(nextclick);
#ifdef STARTUP_DEBUG
	ml_printf("kvm_init done\n");
#endif


#ifndef NODEBUGGER
	/*
	 * Initialize the kernel debugger(s).
	 * At this point:
	 *	BSS must be mapped and zeroed.
	 *	It must be possible to call the putchar/getchar routines
	 *		for the console device.
	 *
	 * This is an early-access point.  The debugger can be entered here,
	 * but not all features can yet be used (in particular, breakpoints
	 * and single-stepping).  The call to kdb_init(0) below is the first
	 * point that the full feature set can be used.
	 *
	 * Debugger calls ("calldebug()") can be placed between here and
	 * the kdb_init(0) call, but they will also be subject to the
	 * restricted feature set.
	 *
	 * Command strings from "unixsyms -e" will be executed here.
	 */
	kdb_init(1);
#endif


	/* Do scheduler initialization.
	*/

	dispinit();
#ifdef STARTUP_DEBUG
	ml_printf("dispinit done\n");
#endif


	/* Initialize the high-resolution timer free list.
	*/

	hrtinit();
#ifdef STARTUP_DEBUG
	ml_printf("hrtinit done\n");
#endif


	/* Initialize the interval timer free list.
	*/

	itinit();
#ifdef STARTUP_DEBUG
	ml_printf("int timer done\n");
#endif


	/* Initialize process 0 and set up "u.".
	*/

	p0init();
#ifdef STARTUP_DEBUG
	ml_printf("proc 0 done\n");
#endif


#ifndef NODEBUGGER
	/*
	 * Kernel debugger full-featured access point.
	 * At this point:
	 *	It must be possible to field debugger traps.
	 *		(Note: k_trap() references u.u_fault_catch)
	 *
	 * This call enables the full feature set.
	 *
	 * Command strings from "unixsyms -i" will be executed here.
	 */
	kdb_init(0);
#endif


	/* Initialize process table.
	*/
	pid_init();
#ifdef STARTUP_DEBUG
	ml_printf("pid init done\n");
#endif

	/*
	 * Intercept devsw entries to support !D_NOBRKUP and D_OLD drivers.
	 */
	fix_swtbls();
#ifdef STARTUP_DEBUG
	ml_printf("fix swtbls done\n");
#endif

/* RESTRICTED_DMA Support */
	if (rdma_enabled) {
		rdma_fix_swtbls();
#ifdef STARTUP_DEBUG
		ml_printf("rdma_fix_swtbls done\n");
#endif
	}
/* End RESTRICTED_DMA Support */


#ifdef STARTUP_DEBUG
	ml_printf("ml_kprep done...\n");
        ml_pause();
#endif
}


int page_hashsz;		/* Size of page hash table table (power of 2) */
page_t **page_hash;	/* Page hash table */
struct seg  *segkmap;		/* kernel generic mapping segment */
struct seg  *segu;		/* u area mapping segment - for kvsegu segment */
int segu_size;			/* Size of floating u area */

STATIC void
kvm_init(nextfree)
register int nextfree;
{
	extern int	kvsegmap[];	/* kv segmap */
	extern int	kvsegu[];	/* u area segment */
	struct segmap_crargs a;

	/*
	 * XXX - hard coded value for extra 384K of high physical memory.
	 */
	(void) seg_attach(&kas, (addr_t)KVBASE,
		(KVXBASE + (384*1024)) - KVBASE, &kpseg);
	(void) segkmem_create(&kpseg, (caddr_t) NULL);

	(void) seg_attach(&kas, (caddr_t) stext,
		(unsigned) sbss - (unsigned) stext + (unsigned) (&end - sbss),
		&ktextseg);
	(void) segkmem_create(&ktextseg, (caddr_t) NULL);

	(void) seg_attach(&kas, (addr_t)syssegs, ctob(syssegsz), &kvseg);
	(void) segkmem_create(&kvseg, (caddr_t) kptbl);

	(void) seg_attach(&kas, piosegs, ctob(piosegsz), &kpioseg);
	(void) segkmem_create(&kpioseg, (caddr_t) piownptbl);

	/*
	 * memNOTused, nextfree, nclick_rtn, and non_dma_page are
	 * no longer valid after the return from mktables.
	 */
	mktables(nextfree);
#ifdef STARTUP_DEBUG
	ml_printf("mktables  done\n");
#endif

	hat_init();
#ifdef STARTUP_DEBUG
	ml_printf("hat init  done\n");
#endif

	/*
	 *	Kernel memory allocator initialization. Should be done now
	 *	in order to allocate physical memory for "kvsegmap" and 
	 *	and "kvsegu" kernel segments - as done immediately below.
	 */

	kmem_init();
#ifdef STARTUP_DEBUG
	ml_printf("kmem_init done\n");
#endif

	/*
	 *  	  The "segkmap" page table is NOT allocated here.
	 *	  This will be dynamically allocated by the hat_memload().
	 */

	segkmap = seg_alloc(&kas, (addr_t)kvsegmap, ctob(segmapsz));
	if (segkmap == NULL)
		cmn_err(CE_PANIC,"cannot allocate segkmap");
	a.prot = PROT_READ | PROT_WRITE;
	if (segmap_create(segkmap, (caddr_t)&a) != 0)
		cmn_err(CE_PANIC,"segmap_create segkmap");
#ifdef STARTUP_DEBUG
	ml_printf("seg_kmap created\n");
#endif


	/* floating u area support */

	/*
	 *	Size of the floating u area = Total Number of processes *
	 *				 the max ublock size of a process.
	 */

	if ((segu_size = ctob(v.v_proc * MAXUSIZE)) <= 0)
		cmn_err(CE_PANIC,"No space for floating u areas\n");

	/*
	 *	We may have to skip some segu slots (those which cross
	 *	page table boundaries), so adjust segu_size accordingly.
	 */

#if (MAXUSIZE % NPGPT) != 0
	segu_size += ctob((ptnum(segu_size) + 1) * MAXUSIZE);
#endif

	/*
	 *	Allocate Floating u area segment structure and
	 *	fix Base and Size of Floating u area segment.
	 */

	if ((segu = seg_alloc(&kas, (addr_t)kvsegu, segu_size)) == NULL)
		cmn_err(CE_PANIC,"cannot allocate segu");

	/*
	 *	Create pool of ublock "slots" for Floating u area segment.
	 */

	a.prot = PROT_READ | PROT_WRITE;
	if (segu_create(segu, (caddr_t)&a) != 0)
		cmn_err(CE_PANIC,"segu_create segu");
#ifdef STARTUP_DEBUG
	ml_printf("seg_u created\n");
#endif
}


/*  Allocate page structures and page hash structures for all user free memory.
 *  Scheme: Chew up as much as NON-DMA-able memory as possible for this.
 *  Invalidates memNOTused, nclick_rtn, and non_dma_page
 */
STATIC void
mktables(nextfree)
int nextfree;
{
	register int i, j, m, totalclicks;
	page_t *pp, *spp;

	/*  Find total clicks left in all unused memory segments.
	 */

	totalclicks = btoct(memNOTused[memused_idx].base +
				memNOTused[memused_idx].extent) - nextfree;
	for (j = memused_idx + 1; j < memNOTusedcnt; j++) {
		totalclicks += btoct(memNOTused[j].extent);
	}

	/* if paging symbol table, include that memory as well, mod_obj_size
	   is an even click multiple as set in unixsyms */
	if(mod_obj_pagesymtab == MOD_FPAGE)
		totalclicks += btoct(mod_obj_size);


	/*	Initialize the map used to allocate
	**	kernel virtual space.  Don't let this be bigger than
	**	(totalclicks - lotsfree) to avoid thrashing.
	*/

	mapinit(sptmap, v.v_sptmap);
#ifdef STARTUP_DEBUG
	ml_printf("sptmapped\n");
#endif
	mfree(sptmap, syssegsz, btoc(syssegs));

#ifdef STARTUP_DEBUG
        ml_printf("mktables: v.v_sptmap= 0x%x totalclicks= 0x%x lotsfree= 0x%x.\n", v.v_sptmap, totalclicks, lotsfree);
#endif

	/*
	 * Find total bytes needed for the page hash structures.
	 */
	m = totalclicks / PAGE_HASHAVELEN;
	while (m & (m - 1))
		m = (m | (m - 1)) + 1;
	page_hashsz = m;
	i = m * sizeof(page_t *);

	/*
	 * Find total bytes needed for page hash structures and page structures.
	 * We don't need page structures for pages that will be occupied by the
	 * page hash and page structures themselves.
	 */
	m = totalclicks - btoc(totalclicks * sizeof(page_t) + i);
	i = (i + m * sizeof(page_t));
	/*
	 * Try to pick up any page(s) saved in the page pool; may have to 
	 * use another page for page and hash tables in the process.
	 */
	while (btoc(i + sizeof(page_t)) + m + 1 <= totalclicks) {
		m++;
		i += sizeof(page_t);
	}
	i = totalclicks - m;
	/* i = btoc(i); */
	
	/*
	 * Remember kernel virtual start for page and page hash structures;
	 * use NON-DMA-ABLE page if possible.
	 */
#ifdef STARTUP_DEBUG
	ml_printf("before first sptalloc\n");
#endif
	pp = (page_t *) sptalloc(1, PG_V, (caddr_t)non_dma_page(&nextfree), 0);

	/*
	 * Allocate space for the rest of the page and page hash structures;
	 * use NON-DMA-ABLE pages if possible.
	 */
	for (j = 1; j < i; j++)
		sptalloc(1, PG_V, (caddr_t)non_dma_page(&nextfree), 0);

	page_hash = (page_t **) (pp + m);

#ifdef STARTUP_DEBUG
        ml_printf("mktables: i=0x%x page_hash=0x%x pp= 0x%x m=0x%x page_hashsz= 0x%x, nextfree=0x%x.\n ", i,page_hash, pp, m, page_hashsz,nextfree);
	ml_pause();
#endif

	/* Fix up current segment base and extent to make computation easier.
	*/
	memNOTused[memused_idx].extent = ctob(btoct(memNOTused[memused_idx].base +
					memNOTused[memused_idx].extent) - nextfree);
	memNOTused[memused_idx].base = ctob(nextfree);

	/* if paging the symbol table, add memory where symbol table is to memNOTused
	    for convenience in initializing page pool chucks.
	    After this memNOTused, nextfree, nclick_rtn, and non_dma_page are no
	    longer valid. */
	if (mod_obj_pagesymtab == MOD_FPAGE)
		addtomemNU(symbase, mod_obj_size, ml_availind(symbase));

#ifdef STARTUP_DEBUG
	ml_printf("before page_init loop\n");
#endif
	/* Initialize the page pool */
	for (j = memused_idx; j < memNOTusedcnt; j++) {
		i = btoct(memNOTused[j].extent);
#ifdef STARTUP_DEBUG
		ml_printf("page_init_chunk(%x, %x, %x)\n",
			  pp, i, btoct(memNOTused[j].base));
#endif
		page_init_chunk(pp, i, btoct(memNOTused[j].base));
		pp += i;
	}
	/* if the symbol table is being paged,
	   get the page pointer corresponding to the first page of the symbol table */
	if(mod_obj_pagesymtab == MOD_FPAGE) {
		spp = page_numtopp(PFNUM(symbase));
#ifdef STARTUP_DEBUG
		ml_printf("mktables: symbol table added to the memNOTused array. spp = 0x%x\n",spp);
		ml_pause();
#endif
	} else
		spp = NULL;

#ifdef STARTUP_DEBUG
	ml_printf("before page_init\n");
#endif
	/* initialize all pages and page_free all pages 
	   except those in the range of the arguments to page_init */
	page_init(spp, btoct(mod_obj_size));

	/*  ASSERT: kernel virtual for page structures lower than kernel virtual
	 *	    for page hash structures.
	 */
	if ((char *)pp > (char *)page_hash) {
#ifdef STARTUP_DEBUG
		ml_printf("Page pool (%x) overlaps page hash table (%x)\n",
			  pp, page_hash);
		ml_pause();
#endif
		cmn_err(CE_PANIC,"Invalid page struct and page hash tables\n");
	}
}

/*	Allocate page tables for the kernel segment sysseg.
 *	syssegs must be on a page table boundary.
*/

sysseginit(nextfree)
int nextfree;
{
	register pte_t *pt;
	register int ptcnt;
	register u_int pfn;

	/* Get a chunk of contiguous physical pages for the page tables */
	do {
		pfn = non_dma_page(&nextfree);
		for (ptcnt = 1; ptcnt < syssegsz / NPGPT; ptcnt++) {
			if (non_dma_page(&nextfree) != pfn + ptcnt)
				break;
		}
	} while (ptcnt < syssegsz / NPGPT);

	/*
	 * kptbl points to the syssegs page table(s).
	 * Since they're contiguous physically, the phystokv virtual will
	 * also be contiguous.
	 */
	kptbl = (pte_t *)phystokv(ctob(pfn));

	pt = &kpd0[ptnum(syssegs)];
	for (ptcnt = syssegsz / NPGPT; ptcnt-- > 0; pt++, pfn++) {
		pt->pg_pte = mkpte(PG_V|PG_US, pfn);
		bzero((caddr_t)phystokv(ctob(pfn)), NBPP);
	}

	return(nextfree);
}

/*	Allocate page tables for the kernel segment pioseg.
 *	piosegs must be on a page table boundary.
*/

pioseginit(nextfree)
int nextfree;
{
	register pte_t *pt;
	register int i;

	i = ptnum(piosegs);
	pt = &kpd0[i];
	if (!PG_ISVALID(pt)) {
		pt->pg_pte = mkpte(PG_V, non_dma_page(&nextfree));
		bzero((caddr_t)phystokv(ctob(pt->pgm.pg_pfn)), NBPP);
	}

	/* piownptbl points to the piosegs page table */
	piownptbl = (pte_t *)phystokv(ctob(pt->pgm.pg_pfn));

	return(nextfree);
}


/*
 *	Allocate proc 0 u area.
 */

p0u(nextfree)
int nextfree;
{
	register pte_t	*ptptr;

	/* Allocate the usertable page table for &u */

	ptptr = &kpd0[ptnum(&u)];

	if (!PG_ISVALID(ptptr))
		ptptr->pg_pte = mkpte(PG_V, non_dma_page(&nextfree));
	bzero((caddr_t)phystokv(ctob(ptptr->pgm.pg_pfn)), ctob(1));
	usertable = (pte_t *) phystokv(ctob(ptptr->pgm.pg_pfn)) + pnum(&u);

	PG_SETPROT(ptptr, PTE_RW);  /* Page directory entry is user read/write */

	/*
	 *  Add a system-wide page below the ublock (stack grows downwards) to
	 *  prevent kernel stack overflow. Note: This page is not per-process and
	 *  hence cannot be saved/restored as a context of any given process.
	 *
	 *  Note: 80386 B1 Errata 13 doesn't apply here, since we never return
	 *	  to user mode while on this section of the stack.
	 */

	/* The page table */

	ptptr = &kpd0[ptnum((char *)&u - NBPP)];
	if (!PG_ISVALID(ptptr))
		ptptr->pg_pte = mkpte(PG_V, non_dma_page(&nextfree));
	PG_SETPROT(ptptr, PTE_RW);

	/* And the physical page */

	ptptr = (pte_t *) phystokv(ctob(ptptr->pgm.pg_pfn)) +
					pnum((char *)&u - NBPP);
	ptptr->pg_pte = mkpte(PG_V, non_dma_page(&nextfree));
	bzero(((char *)&u - NBPP), NBPP);

	return(nextfree);
}


/*
 *	Allocate proc 0 u area and set up proc 0.
 */

p0init()
{
	register proc_t		*pp;
	register caddr_t	ldtp;
	register char	*iobm;
	register int	iobmcount;
	extern struct gate_desc	idt[];
	extern struct seg_desc	gdt[];
	extern int		userstack[];
	extern struct tss386	ktss;

	/* Allocate proc structure for proc 0 */
	pp = (struct proc *)kmem_zalloc(sizeof(struct proc), KM_NOSLEEP);
	if (pp == NULL) {
proc0_fail:
		cmn_err(CE_PANIC,"process 0 - creation failed\n");
	}
	curproc = proc_sched = pp;
#ifdef STARTUP_DEBUG
	ml_printf("got struct for proc0\n");
#endif

	/* Initialize process data */
	pp->p_usize = MINUSIZE;
	pp->p_stat = SONPROC;
	pp->p_flag = SLOAD | SSYS | SLOCK | SULOAD;
	pp->p_pri = curpri = v.v_maxsyspri;
	pp->p_clfuncs = class[0].cl_funcs;
	pp->p_clproc = (caddr_t)pp;

	adt_init();

	pp->p_pidp = &pid0;
	pp->p_pgidp = &pid0;
	pp->p_sessp = &session0;
	pid0.pid_pglink = pp;

	/* Allocate the floating ublock */
	pp->p_segu = (struct seguser *)segu_get(pp, 1);
	if (pp->p_segu == NULL)
		goto proc0_fail;
#ifdef STARTUP_DEBUG
	ml_printf("got seg_u for proc0\n");
#endif

	/* Copy the ublock page table into usertable, so we can use &u */
	bcopy((caddr_t)pp->p_ubptbl, (caddr_t)usertable,
					MINUSIZE * sizeof(pte_t));

	/* Zero out the ublock */
	bzero((char *)&u, ctob(MINUSIZE));
#ifdef STARTUP_DEBUG
	ml_printf("uarea cleared\n");
#endif

	/*	Initialize proc 0's ublock
	*/

	u.u_procp   = pp;
	u.u_userstack = (unsigned long) userstack;
	u.u_sub     = u.u_userstack + sizeof(int);
	u.u_ap	= NULL;
	u.u_tsize   = 0;
	u.u_dsize   = 0;
	u.u_ssize   = 0;
	u.u_cmask   = (mode_t) CMASK;

	bcopy((caddr_t)rlimits, (caddr_t)u.u_rlimit,
		sizeof(struct rlimit) * RLIM_NLIMITS);

	/* XENIX Support */
	/*
	 *	Set the default trap handlers for int 0xf0, ... 0xff to
	 *	be invalid traps.  These are used for xenix 286 floating
	 *	point.
	 */
	def_intf0 = * (struct gdscr *) &idt[0xf0];
	* (struct gdscr *) u.u_fpintgate = def_intf0;
	/* End XENIX Support */

	/*
	 *	Now tss is set at the end of (struct user)  - after ufchunk.
	 *	Further ufchunks (to build ufchunk list) are allocated from
	 *	using kmem_zalloc() from "kvseg" (syssegs) - hence the tss
	 *	won't be overrun or corrupted.
	 */
	u.u_tss = (struct tss386 *)
		((int) ((char *)&u.u_flist + sizeof(struct ufchunk) + 3) & ~3L);
#ifdef MERGE386
	u.u_sztss = sizeof(struct tss386) + (IDTSZ + MAXTSSIOADDR + 1) / 8;
#else
	u.u_sztss = sizeof(struct tss386) + (MAXTSSIOADDR + 1) / 8;
#endif

	/* Set up UTSS to refer to current proc's tss (via u) */

	setdscrlim (&gdt[seltoi(UTSSSEL)], u.u_sztss - 1);
	setdscrbase(&gdt[seltoi(UTSSSEL)], (uint)u.u_tss);
	loadtr(UTSSSEL);

	u.u_tss_desc = gdt[seltoi(UTSSSEL)];
	setdscrbase(&u.u_tss_desc, (uint)PTOU(pp) + ((uint)u.u_tss - UVUBLK));
	setdscracc1(&u.u_tss_desc, TSS3_KACC1);

	/*
	 * align ldt on an 8 byte boundary, check size and fixup gdt entry
	 */
	u.u_ldtlimit = MINLDTSZ;
	ldtp = (caddr_t)(((uint)u.u_tss + u.u_sztss + 7) & ~7L);
	pp->p_ldt = (caddr_t)PTOU(pp) + (u_int)(ldtp - UVUBLK);
	setdscrbase(&gdt[seltoi(LDTSEL)], pp->p_ldt);
	u.u_ldt_desc = gdt[seltoi(LDTSEL)];

	if (btoc(ldtp + (u.u_ldtlimit + 1) * sizeof(struct dscr) - UVUBLK)
						> MINUSIZE) {
		cmn_err(CE_PANIC,"MINUSIZE (%d) insufficient\n", MINUSIZE);
	}

	/*	Initialize proc 0's TSS. Fix it up so that
	 *	the stack pointers are set up correctly.
	 */

	bcopy((caddr_t) &ktss, (caddr_t) u.u_tss, sizeof(ktss));

#ifdef MERGE386
	u.u_tss->t_bitmapbase = ((sizeof(struct tss386))+IDTSZ / 8) << 16;
#else
	u.u_tss->t_bitmapbase = (sizeof (struct tss386)) << 16;
#endif
	iobm = (char *) u.u_tss + sizeof(struct tss386);
#ifdef MERGE386
	for (iobmcount = (MAXTSSIOADDR+1+IDTSZ) / 8;iobmcount > 0;iobmcount--)
#else
	for (iobmcount = (MAXTSSIOADDR + 1) / 8; iobmcount > 0; iobmcount--)
#endif
            *iobm++ = 0xff;

	u.u_tss->t_esp0 = u.u_tss->t_esp = (unsigned long)((char *)&u + KSTKSZ);
	u.u_tss->t_ldt = LDTSEL;

	/*
	 * Initialize the page fault error handling routine.
	 * The standard routine does a longjmp to u.u_fault_catch.fc_jmp
	 */
	u.u_fault_catch.fc_func = fc_jmpjmp;

	/*
	 * Confirm that the configured number of supplementary groups
	 * is between the min and the max.  If not, print a message
	 * and assign the right value.
	 */
	if (ngroups_max < NGROUPS_UMIN) {
		cmn_err(CE_NOTE, 
		  "Configured value of NGROUPS_MAX (%d) is less than \
min (%d), NGROUPS_MAX set to %d\n", ngroups_max, NGROUPS_UMIN, NGROUPS_UMAX);
		ngroups_max = NGROUPS_UMAX;
	}
	if (ngroups_max > NGROUPS_UMAX) {
		cmn_err(CE_NOTE,
		  "Configured value of NGROUPS_MAX (%d) is greater than \
max (%d), NGROUPS_MAX set to %d\n", ngroups_max, NGROUPS_UMAX, NGROUPS_UMAX);
		ngroups_max = NGROUPS_UMAX;
	}
}

/* scan memory
 *	Set up page tables to map a chunk of physical
 *	memory 1-1 with linear. Return the next available click.
 */

scanmem(nextclick, firstclick, lastclick)
	uint nextclick, firstclick, lastclick;
{
	register uint	j, k;
	register pte_t	*pt, *pd;

	kpd0[0].pg_pte = mkpte(PG_V, PFNUM(_cr3()));
	j = xphystokv(ctob(firstclick));
	pd = kpd0 + (k = ptnum(j));
	pt = (pte_t *)ctob(k) + pgndx(j);

#ifdef STARTUP_DEBUG
        ml_printf("scan_mem: \n");
        ml_printf("first_click= 0x%x last_click= 0x%x kpd0= 0x%x j= 0x%x k= 0x%x \n",
        firstclick, lastclick, kpd0, j, k);
        ml_printf("pd= 0x%x pt= 0x%x kpd0[0]= 0x%x \n", pd, pt, kpd0[0]);
        ml_pause();
#endif

	/* NOTE: In the following loop, we go one page too far to get around
		 a compiler bug */

	for (j = firstclick; j <= lastclick; ++pd) {
		/* See if we need a new page table */
		if (!PG_ISVALID(pd) || (uint)pd->pgm.pg_pfn < firstfree) {
			pd->pg_pte = mkpte(PG_V, nextclick);
			nextclick = nclick_rtn(nextclick);
			bzero((caddr_t)ptalign(pt), NBPC);
		}
		for (k = j % NPGPT; k < NPGPT && j <= lastclick; ++k, ++j) {
			(pt++)->pg_pte = mkpte(PG_V, j);
		}
#ifdef STARTUP_DEBUG
        	ml_printf("pd= 0x%x pd[]= 0x%x \n", pd, *pd);
#endif
	}

	physmem += --j - firstclick;
#ifdef STARTUP_DEBUG
        ml_printf("physmem= 0x%x nextclick= 0x%x \n", physmem, nextclick);
#endif
	return(nextclick);
}


/*
 * This routine determines which portions of physical memory (memavail)
 * are not already used (memused).  The unused memory tuples 
 * (paddr, extent, flags) are stored in memNOTused[].
 *
 */

ml_unused_mem()
{
    register paddr_t  free_paddr;
    register unsigned avail;
    register unsigned used;
    unsigned unused;
    unsigned long free_size;
    ushort   free_flags;
    struct bootmem tmpNOTused;
    int	     i, j;

    unused = 0;
    for (avail = 0; avail < bootinfo.memavailcnt; avail++) {
	free_paddr = bootinfo.memavail[avail].base;
	free_size = bootinfo.memavail[avail].extent;
	free_flags = bootinfo.memavail[avail].flags;

	for (used = 0; used < bootinfo.memusedcnt; used++) {
		if ((bootinfo.memused[used].base >= free_paddr) &&
		    (bootinfo.memused[used].base < free_paddr + free_size)) {
			/*
			 * Split out the unused parts of the availmem[] entry
			 * from the used parts.
			 */
			if (free_paddr < bootinfo.memused[used].base) {
				/*
				 * There is a chunk of unused, available memory
				 * from free_paddr upto memused[used].base.
				 * Add it to memNOTused[], and skip free_paddr
				 * past the used chunk.
				 */
				memNOTused[unused].base = free_paddr;
				memNOTused[unused].extent = 
				      bootinfo.memused[used].base - free_paddr;
				memNOTused[unused].flags = free_flags;
				free_paddr = bootinfo.memused[used].base +
						bootinfo.memused[used].extent;
				free_size -= (memNOTused[unused].extent + 
						bootinfo.memused[used].extent);
				unused++;
				continue;
			} else if (free_paddr == bootinfo.memused[used].base) {
				/*
				 * Skip over the used chunk of memory
				 * starting at free_paddr.
				 */
				free_paddr = bootinfo.memused[used].base +
						bootinfo.memused[used].extent;
				free_size -= bootinfo.memused[used].extent;
			}
		}
	}

	if (free_size > (unsigned long) 0) {
		/*
		 * There is a free chunk of unused available memory at
		 * the end of the availmem[] entry.  Add it to memNOTused[].
		 */
		memNOTused[unused].base = free_paddr;
		memNOTused[unused].extent = free_size;
		memNOTused[unused].flags = free_flags;
		unused++;
	}
    }
    memNOTusedcnt = unused;

	/*
	 * Sort the memNOTused segments into ascending base address order.
	 */
	for (i = memNOTusedcnt; i-- != 0;) {
		for (j = i; j-- != 0;) {
			if (memNOTused[j].base > memNOTused[i].base) {
				tmpNOTused = memNOTused[j];
				memNOTused[j] = memNOTused[i];
				memNOTused[i] = tmpNOTused;
			}
		}
	}

#ifdef STARTUP_DEBUG
        ml_printf("mlsetup: memory NOTused\n");
        for (i = 0; i < memNOTusedcnt; i++) {
                ml_printf("Base= 0x%x Extent= 0x%x Flag= 0x%x\n", memNOTused[i].base, memNOTused[i].extent, memNOTused[i].flags);
        }
        ml_pause();
#endif
}


/*
 * Return a NON-DMA-ABLE page from a NON-DMA-ABLE memory segment.
 * If not possible, then return the next page from the current memory segment.
 * We do this to try to use up non-dma-able memory for structures allocated
 * at startup time, so we free up general-purpose memory for other uses.
 */

non_dma_page(nextfree_ptr)
	register int *nextfree_ptr;	/* Pointer to current free page */
{
	register int i, j, rtn_click;

	/*  Scan from High to Low memory Segments ... NON-DMA-ABLE memory usually
	 *  in the high end.
	 */
	for (i = memNOTusedcnt - 1; i > memused_idx; i--)
		if ((memNOTused[i].flags & B_MEM_NODMA) == B_MEM_NODMA)
			break;

	/*  NON-DMA-ABLE memory not found or scan stopped at current segment.
	 *  Also indicate which is the next page that can be used from the current
	 *  memory segment (nextfree).
	 */
	if (i == memused_idx)
		*nextfree_ptr = nclick_rtn((rtn_click = *nextfree_ptr));
	else {
		/* NON-DMA-ABLE segment other than the current segment found.
		*/
		rtn_click = btoct(memNOTused[i].base);
		memNOTused[i].base += ctob(1);

		/* If entire segment used up -- deleted segment from list.
		*/
		if ( ! (memNOTused[i].extent -= ctob(1))) {
			for (j = i; j < memNOTusedcnt - 1; j++)
				memNOTused[j] = memNOTused[j+1];
			memNOTusedcnt--;		/* one less segment */
		}
	}
	return(rtn_click);	/* Return page number that can be used */
}

nclick_rtn(click)
register unsigned click;
{
	if ((memused_idx >= memNOTusedcnt) || (++click >= maxclick))
		return(-1);

	if (click >= btoct(memNOTused[memused_idx].base + 
					memNOTused[memused_idx].extent)) {
		if (++memused_idx >= memNOTusedcnt)
			return(-1);
		return(btoct(memNOTused[memused_idx].base));
	}

	return(click);
}

#ifdef STARTUP_DEBUG

/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %lu %ld (==%lu)
 * %lo %lx are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much suspended.
 * Printf should not be used for chit-chat.
 */

ml_printf(fmt, x1)
register char *fmt;
unsigned x1;
{
	register c;
	register char *adx;
	char *s;
	int	delay = 0;

	adx = (char *)&x1;
loop:
	while((c = *fmt++) != '%') {
		if(c == '\0') {
			if (delay) {
				for (;delay < 10;delay++); 
			}
			return;
		}
#ifdef	CR
		if (c == '\n')
			ml_putchar('\r');
#endif	/* CR */
		ml_putchar(c);
	}
	c = *fmt++;
	if(c == 'd' || c == 'u' || c == 'o' || c == 'x') {
		ml_printn((long)*(unsigned int *)adx, c=='o'? 8: (c=='x'? 16:10));
		adx += sizeof (int);
	} else if(c == 's') {
		s = *(char **)adx;
		while(c = *s++) {
#ifdef	CR
			if (c == '\n')
				ml_putchar('\r');
#endif	/* CR */
			ml_putchar(c);
		}
		adx += sizeof (char *);
	} else if (c == 'l') {
		c = *fmt++;
		if(c == 'd' || c == 'u' || c == 'o' || c == 'x') {
			ml_printn(*(long *)adx, c=='o'? 8: (c=='x'? 16:10));
			adx += sizeof (long);
		}
	} else if (c == 'Y') delay = 1;
	
	goto loop;
}

ml_printn(n, b)
long n;
register b;
{
	register i, nd, c;
	int	flag, dflag;
	int	plmax;
	char d[12];

	c = 1;
	flag = n < 0;
	if (flag)
		n = (-n);
	dflag = b < 0;
	if (dflag)
		b = (-b);
	if (b==8)
		plmax = 11;
	else if (b==10)
		plmax = 10;
	else if (b==16)
		plmax = 8;
	if (flag && b==10) {
		flag = 0;
		ml_putchar('-');
	}
	for (i=0;i<plmax;i++) {
		nd = n%b;
		if (flag) {
			nd = (b - 1) - nd + c;
			if (nd >= b) {
				nd -= b;
				c = 1;
			} else
				c = 0;
		}
		d[i] = nd;
		n = n/b;
		if ((n==0) && (flag==0))
			break;
	}
	if (i==plmax)
		i--;
	else if ( dflag && (i < --plmax))
		for (c = plmax - i; c > 0;c--) ml_putchar(' ');
	for (;i>=0;i--) {
		ml_putchar("0123456789ABCDEF"[d[i]]);
	}
}

/* initial row and column position - force clear screen */
STATIC unsigned int b_col = 0;
STATIC unsigned int b_row = 23;
STATIC unsigned int b_flag = 0;

/* mono and color video adaptor base addresses */
STATIC char *monobase  = (char *)phystokv(0xB0000);
STATIC char *colorbase = (char *)phystokv(0xB8000);

/* write to both mono and color address so will work whichever is present */
#define PUTCH(row, col, ch) {					\
	register int loc;					\
	register char *screen;					\
	loc = ((row) * 2 * 80) + ((col) * 2);			\
	screen = monobase + loc;				\
	*screen++ = (ch);					\
	*screen = 0x07;	/* video attribute */			\
	screen = colorbase + loc;				\
	*screen++ = (ch);					\
	*screen = 0x07;	/* video attribute */			\
}

ml_putchar(pchr)
char 	pchr;
{
	int 	i, j;
	
	if ((b_row == 23) && !b_flag) {
		b_flag = 1;
		ml_pause();
		for (i=0; i<=23; i++) {
			for (j=0; j<80; j++) {
				PUTCH(i, j, ' ');
			}
		}
		b_col = b_row = 0;
		b_flag = 0;
	}
	if (pchr == '\n') {
		b_col = 0;
		b_row++;
	} else {
		PUTCH(b_row, b_col, pchr);
		b_col++;
	}
}

ml_pause()
{
	register unchar scan;       	/* raw keyboard code 	*/
	int  		i;

	ml_printf("Hit any key to continue ...\n");
	for (i=0; i<2; i++) {
/*	wait for keyboard input					*/
	while ((inb(KB_STAT) & KB_OUTBF) == 0) ;

	/* get the scan code */
	scan = inb(KB_IDAT);    /* Read scan data */
	kdskbyte(KB_ENAB);       /* Enable keyboard */
	}
	return;
}

/*
 * send command byte to the keyboard
 */
kdskbyte(cmd)
unsigned char	cmd;
{
	while (inb(KB_STAT) & KB_INBF) /* wait input buffer clear */
		;
	outb(KB_ICMD, cmd); /* give command byte to controller */
}

#endif	/* STARTUP_DEBUG */

