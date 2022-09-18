/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/mod/modinit.c	1.2"
#ident	"$Header: $"

#include <util/types.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/seg_kmem.h>
#include <mem/kmem.h>
#include <mem/as.h>
#include <mem/vmparam.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <util/mod/mod.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>


int    mod_initialized = 0;
size_t mod_obj_size = 0;
struct module *mod_obj_kern = NULL;

extern int mod_obj_pagesymtab;		/* tunable */

extern pte_t kspt0[];			/* kernel page table */
extern paddr_t symbase;			/* physical address of kernel symbol table */


/*
 *	void
 *	modinit(const char *modulename)
 *
 *	Sets up static kernel symbol table. Called from main.  Relies on earlier setup
 *	in ml_kprep and other startup code.
 *
 *	Calling/Exit State: modulename is the full pathname from whence the kernel was
 *	booted. Relies on earlier setup by unixsyms, ml_kprep, and other 
 *	kernel startup code.
 *	On return, all the mod_obj_kern fields are correct for the kernel symbol
 *	table.  If mod_obj_kern is NULL on call, nothing is done and no kernel
 *	symbol table will be available.
 */
void
modinit(const char *modulename)
{

	struct module *mod_obj_kern_tmp;
	page_t *pp;
	pte_t *pt;
	int i;
	boolean_t paged;

	/* If unixsyms was not run and hence mod_obj_kern not filled in, no
	   kernel symbol table is available */
	if(mod_obj_kern == NULL)
		return; 

	/*
	 * At this point we can assume the
	 * initializtation will complete.
	 */
	mod_initialized = 1;

	/* If the symbol table is not to be paged, then mod_obj_kern->md_symspace
	   already points to the right place at this point. */
	if(mod_obj_pagesymtab == MOD_NOPAGE) {
		mod_obj_kern->md_page = B_FALSE;
		mod_obj_kern->md_path = (char *) modulename;
		return;
	}

	/* mod_obj_pagesymtab == MOD_FPAGE */

	mod_obj_kern_tmp = mod_obj_kern;
	mod_obj_kern = kmem_zalloc(sizeof(struct module), KM_SLEEP);
	*mod_obj_kern = *mod_obj_kern_tmp;


	mod_obj_kern->md_page = B_TRUE;	/* protect against debugger access
						until bcopy */

	/* get the space for the kernel symbol table */
	mod_obj_kern->md_symspace = mod_obj_getspace(mod_obj_kern->md_symsize,
						&paged);
	bcopy(mod_obj_kern_tmp->md_symspace, mod_obj_kern->md_symspace, mod_obj_kern->md_symsize);
	mod_obj_kern->md_path = (char *) modulename;
	mod_obj_kern->md_strings = mod_obj_kern->md_symspace +
				(unsigned int) (mod_obj_kern->md_strings - mod_obj_kern_tmp->md_symspace);
	mod_obj_kern->md_buckets = 
		(unsigned long *) (mod_obj_kern->md_symspace +
				((char *) mod_obj_kern->md_buckets - mod_obj_kern_tmp->md_symspace));
	mod_obj_kern->md_chains = 
		(unsigned long *) (mod_obj_kern->md_symspace +
				((char *) mod_obj_kern->md_chains - mod_obj_kern_tmp->md_symspace));
	mod_obj_kern->md_page = paged;

	/* Free the temporary pages associated with the kernel symbol table, thereby
	   enabling them for general user use.  These pages were locked in page_init. */
	pp = page_numtopp(PFNUM(symbase));
	pt = kspt0 + btoct((unsigned long) mod_obj_kern_tmp - KVSBASE);
	for(i = btoc(mod_obj_size); i > 0; pt++, pp++, i--) {
		PAGE_RELE(pp);
		PG_CLRVALID(pt);
	}

}



