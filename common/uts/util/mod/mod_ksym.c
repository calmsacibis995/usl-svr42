/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/mod/mod_ksym.c	1.6"
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
#include <mem/faultcatch.h>
#include <fs/file.h>
#include <util/sysmacros.h>
#include <proc/obj/elf.h>
#include <proc/user.h>
#include <acc/priv/privilege.h>
#include <util/cmn_err.h>
#include <util/mod/mod.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>
#include <util/mod/ksym.h>

STATIC boolean_t mod_obj_addrcheck(const struct module *mp, unsigned long addr);
STATIC char *mod_obj_searchsym(const struct module *mp, 
			unsigned long value, unsigned long *offset, boolean_t pagetoget);
STATIC int mod_obj_getsymp(const char *name, boolean_t kernelonly,
			boolean_t pagetoget, Elf32_Sym *);


extern int mod_obj_pagesymtab;		/* tunable */


struct gksyma {
	char *symname;
	unsigned long *value;
	unsigned long *info;
};

/*
 *	int
 *	getksym(struct gksyma *uap, rval_t *rvp)
 *	
 *	system call interface to get information about a symbol in the dynamic
 *	symbol table.
 *
 *	Calling/Exit State: On call, if the space pointed to by uap->value contains 0,
 *	then the routine tries to locate a symbol with the name given in
 *	uap->symname and returns the value of that symbol in the space pointed to by
 *	uap->value and its type in the space pointed to by uap->info. Otherwise, the
 *	routine tries to locate the symbol whose value is the closest one less than
 *	or equal to uap->value and returns its name in uap->name and the difference 
 *	in values in uap->info.  rvp is used to return 0 on success and non-zero
 *	on failure. The routine returns 0 on success and the appropriate errno on
 *	failure.
 */
int
getksym(struct gksyma *uap, rval_t *rvp)
{

	unsigned long kvalue;
	unsigned long kinfo;
	size_t klen;
	char kname[MAXSYMNMLEN];
	char *kname2;
	Elf32_Sym sp;
	int error;

	if((error = ucopyin((char *) uap->value, (char *)&kvalue, sizeof(unsigned long), 0)) != 0) {
		rvp->r_val1 = -1;
		return(error);
	}


	if(kvalue == 0) {
		/* assume want to look up value for symbol in name */
		if((error = copyinstr(uap->symname, kname, MAXSYMNMLEN, NULL)) != 0) {
			rvp->r_val1 = -1;
			return(error);
		}

		if(mod_obj_getsymp(kname, B_FALSE, B_TRUE, &sp) != 0) {
			rvp->r_val1 = -1;
			return(ENOMATCH);
		}
		kinfo = ELF32_ST_TYPE(sp.st_info);
		if((error = ucopyout((char *) &sp.st_value,(char *)uap->value, sizeof(unsigned long),0)) != 0 ||
		(error = ucopyout((char *) &kinfo,(char *) uap->info, sizeof(unsigned long),0)) != 0) {
			rvp->r_val1 = -1;
			return(error);
		}
		rvp->r_val1 = 0;
		return(0);
	}

	/* assume want to find symbol whose address is closest to but not greater
	   than kvalue */

	if((kname2 = mod_obj_getsymname(kvalue, &kinfo, B_TRUE,kname)) == NULL) {
		rvp->r_val1 = -1;
		return(ENOMATCH);
	}
	if((error = ucopyout((char *) &kinfo,(char *) uap->info, sizeof(unsigned long),0)) != 0) {
		rvp->r_val1 = -1;
		return(error);
	}

	klen = strlen(kname2) + 1;

	if(klen > MAXSYMNMLEN) {
		klen = MAXSYMNMLEN;
	}

	if((error = ucopyout(kname2, uap->symname, klen,CATCH_KPAGE_FAULT)) != 0) {
		rvp->r_val1 = -1;
		return(error);
	}
	rvp->r_val1 = 0;
	return(0);
}

/*
 *	char *
 *	mod_obj_getspace(size_t symsize,boolean_t *paged)
 *
 *	Get paged or non-paged space as available and appropriate for symbol table.
 *	Called from mod_obj_load and modinit.
 *
 *	Calling/Exit State: symsize is the size in bytes of the space needed. If
 *	mod_obj_pagesymtab is set to MOD_FPAGE, the routine attempts to acquire paged 
 *	space (segvn kzfod segment) from the reserved kernel vitual space.  If 
 *	there is no space of the appropriate size, then space is kmem_zalloced.  
 *	If mod_obj_pagesymtab is
 *	set to MOD_NOPAGE, then the space is kmem_zalloced immediately.  The 
 *	boolean_t pointed to by paged is set to B_TRUE is paged space was obtained
 *	and B_FALSE otherwise.  A pointer to the space obtained is returned.
 */
char *
mod_obj_getspace(size_t symsize,boolean_t *paged)
{
	addr_t base = (char *) KVDSYMBASE;
	unsigned int len = KVDSYMEND - KVDSYMBASE;
	size_t rsymsize;
	rsymsize = ctob(btoc(symsize));

	if(mod_obj_pagesymtab == MOD_FPAGE &&
		  as_gap(&kas, rsymsize, &base, &len, AH_LO, 0) == 0 &&
		  as_map(&kas, base, rsymsize, segvn_create, kzfod_argsp) == 0) {
			*paged = B_TRUE;
			return((char *) base);
	}

	base = (addr_t) kmem_alloc(symsize,KM_SLEEP);
	*paged=B_FALSE;
	return((char *) base);
}

		
/*
 *	boolean_t
 *	mod_obj_validaddr(unsigned long value)
 *
 *	Check if given value is a valid address for the kernel or currently loaded 
 *	modules. Called primarily from the debugger.
 *
 *	Calling/Exit State: Returns B_TRUE if the given value is a valid address and
 *	B_FALSE otherwise.
 */
boolean_t
mod_obj_validaddr(unsigned long value)
{
	struct modctl *modp;

	/* If there is no kernel symbol table, use the globals indicating where text
	   is located */
	if(mod_obj_kern == NULL)
		return(value >= (unsigned long) stext && value <  (unsigned long) sdata);

	/* Otherwise check static kernel through mod_obj_kern */
	if(mod_obj_addrcheck(mod_obj_kern,value))
		return(B_TRUE);

	/* Check all currently loaded modules */
	for (modp = modhead.mod_next;
		modp != &modhead;
		modp = modp->mod_next) {

		/* Don't check modules on their way in or out */
		if(!(modp->mod_flags & MOD_SYMTABOK))
			continue;

		if (mod_obj_addrcheck(modp->mod_mp,value))
			return(B_TRUE);
	}
	return (B_FALSE);
}


/*
 *	int
 *	mod_obj_lookupone (const struct module *mp, const char *name, 
 *				boolean_t pagetoget, Elf32_Sym *retsp)
 *
 *	Find symbol table entry associated with symbol given by name in module
 *	given by mp. 
 *
 *	Calling/Exit State: If the symbol table for this module is paged and
 *	the pagetoget flag is B_FALSE, or the symbol is not found in the module,
 *	then ENOMATCH is returned else 0 is returned and the symbol table entry
 *	for the symbol is copied to the space pointed to by retsp.
 */
int
mod_obj_lookupone (const struct module *mp, const char *name, boolean_t pagetoget,
	Elf32_Sym *retsp)
{
	unsigned long hval;
	unsigned long *ip;
	char *name1;
	Elf32_Sym *sp = NULL;
	int error;

	if(mp == NULL || (mp->md_page && !pagetoget))
		return(ENOMATCH);

	hval = mod_obj_hashname (name);

	if(mp->md_page) {
	CATCH_FAULTS(CATCH_KPAGE_FAULT) {
	for (ip = &mp->md_buckets[hval % MOD_OBJHASH]; *ip;
	    ip = &mp->md_chains[*ip]) {
		sp = (Elf32_Sym *)(mp->md_symspace +
		    mp->md_symentsize* (*ip));
		name1 = mp->md_strings + sp->st_name;
		if (strcmp ((char *) name, name1) == 0) {
			*retsp = *sp;
			END_CATCH();
			return(0);
		}
	}
	}
	return((error = END_CATCH()) ? error : ENOMATCH);
	}
	else {
	for (ip = &mp->md_buckets[hval % MOD_OBJHASH]; *ip;
	    ip = &mp->md_chains[*ip]) {
		sp = (Elf32_Sym *)(mp->md_symspace +
		    mp->md_symentsize* (*ip));
		name1 = mp->md_strings + sp->st_name;
		if (strcmp ((char *) name, name1) == 0) {
			*retsp = *sp;
			return(0);
		}
	}
	return(ENOMATCH);
	}
}

STATIC char *
mod_obj_searchsym(const struct module *mp, unsigned long value, unsigned long *offset, boolean_t pagetoget)
{
	Elf32_Sym *symend;
	char *strtabptr;
	Elf32_Sym *sym;
	Elf32_Sym *cursym;
	unsigned int curval;

	if(mp == NULL || (mp->md_page && !pagetoget))
		return(NULL);

	*offset = (unsigned int) -1;			/* assume not found */
	cursym  = NULL;

	/* assumes mp != NULL */
	if (mod_obj_addrcheck(mp, value) == B_FALSE)
		return (NULL);		/* not in this module */

	symend = (Elf32_Sym *) (mp->md_strings);

	/*
	 * Scan the module's symbol table for a global symbol <= value
	 */
	if(mp->md_page) {
	CATCH_FAULTS(CATCH_KPAGE_FAULT) {
	for (sym = (Elf32_Sym *) (mp->md_symspace);
		sym < symend;
		sym=(Elf32_Sym *) ((char *)sym + mp->md_symentsize)) {

		curval = sym->st_value;

		if (curval > value)
			continue;

		if (value - curval < *offset) {
			*offset = value - curval;
			cursym = sym;
		}
	}
	if(cursym != NULL)
		curval = cursym->st_name;
	}
	if(END_CATCH() || cursym == NULL)
		return(NULL);
	}
	else {
	for (sym = (Elf32_Sym *) (mp->md_symspace);
		sym < symend;
		sym=(Elf32_Sym *) ((char *)sym + mp->md_symentsize)) {

		curval = sym->st_value;

		if (curval > value)
			continue;

		if (value - curval < *offset) {
			*offset = value - curval;
			cursym = sym;
		}
	}
	if(cursym != NULL)
		curval = cursym->st_name;
	else
		return(NULL);
	}



	return (mp->md_strings + curval);
}

/*
 *	STATIC boolean_t
 *	mod_obj_addrcheck(const struct module *mp, unsigned long addr)
 *
 *	See if the given addr is in the text, data, or bss of the module
 *	given by mp. Called primarily from mod_obj_searchsym and mod_obj_validaddr.
 *
 *	Calling/Exit State: addr conatins the address in question and mp
 *	contains a pointer to the module to be checked. The routine returns B_TRUE
 *	if the addr is within the module and B_FALSE otherwise.
 */
STATIC boolean_t
mod_obj_addrcheck(const struct module *mp, unsigned long addr)
{
	if(addr >= (unsigned long) mp->md_space && 
	     addr < (unsigned long) (mp->md_space + mp->md_space_size))
		return(B_TRUE);
	if(addr >= (unsigned long) mp->md_bss 
	     && addr < (unsigned long) (mp->md_bss + mp->md_bss_size))
		return(B_TRUE);
	return(B_FALSE);
}

/*
 *	unsigned long
 *	mod_obj_hashname(const char *p)
 *
 *	Hash function for symbol names.  This must be the same as elf_hash(3E) since
 *	that is what unixsyms uses to set up the static kernel symbol table.
 *
 *	Calling/Exit State: p contains a pointer to a '\0' terminated string and
 *	the routine returns a hash value.
 */
unsigned long
mod_obj_hashname(const char *p)
{
	register unsigned long g;
	unsigned long hval;

	hval = 0;
	while (*p) {
		hval = (hval << 4) + *p++;
		if ((g = (hval & 0xf0000000)) != 0)
			hval ^= g >> 24;
		hval &= ~g;
	}
	return (hval);
}

/*
 *	STATIC int
 *	mod_obj_getsymp(const char *name, boolean_t kernelonly, 
 *				boolean_t pagetoget, Elf32_Sym *retsp)
 *
 *	Look for the symbol given by name in the static kernel and all loaded
 *	modules (unless kernelonly is B_TRUE). Called by getksym and mod_obj_getsymvalue.
 *
 *	Calling/Exit State: name contains a '\0' terminated string of the symbol
 *	in question. kernelonly is B_TRUE if only the static kernel symbol table
 *	should be searched for the symbol, B_FALSE otherwise. pagetoget is set
 *	to B_TRUE if paging can take place during the operation. (In practice,
 *	only the kernel debugger sets this flag to B_FALSE.) The routine returns
 *	0 and a copy of the symbol table entry in the space pointed to be retsp
 *	if it is successful in finding the symbol in the table(s), ENOMATCH
 *	otherwise.
 */
STATIC int
mod_obj_getsymp(const char *name, boolean_t kernelonly, boolean_t pagetoget, Elf32_Sym *retsp)
{


	struct modctl *modp;

	if (mod_obj_lookupone (mod_obj_kern, name, pagetoget, retsp) == 0)
		return (0);

	if (kernelonly == B_TRUE)
		return (ENOMATCH);	/* didn't find it in the kernel so give up */

	/*  kernel is NOT in modhead list */
	for (modp = modhead.mod_next;
		modp != &modhead;
		modp = modp->mod_next) {
		if(!(modp->mod_flags & MOD_SYMTABOK))
			continue;

		/* must put a hold on the module in case the module is unloaded
		   while sleeping on a page fault resolution */
		MOD_HOLD(modp);
		if (mod_obj_lookupone (modp->mod_mp, name, pagetoget,retsp) == 0) {
			MOD_RELE(modp);
			return (0);
		}
		MOD_RELE(modp);
	}
	return (ENOMATCH);
}


/*
 *	unsigned long
 *	mod_obj_getsymvalue (const char *name, boolean_t kernelonly, boolean_t pagetoget)
 *
 *	Get the value of a symbol using the given search parameters. Calls
 *	mod_obj_getsymp to do the work and extracts the value from the returned
 *	symbol table entry.
 *
 *	Calling/Exit State: name contains a '\0' terminated string of the symbol
 *	in question. kernelonly is B_TRUE if only the static kernel symbol table
 *	should be searched for the symbol, B_FALSE otherwise. pagetoget is set
 *	to B_TRUE if paging can take place during the operation. (In practice,
 *	only the kernel debugger sets this flag to B_FALSE.) The routine returns
 *	the value of the symbol if found and 0 otherwise.
 */

unsigned long
mod_obj_getsymvalue (const char *name, boolean_t kernelonly, boolean_t pagetoget)
{
	Elf32_Sym sp;

	if(mod_obj_getsymp(name, kernelonly, pagetoget, &sp) != 0)
		return(0L);

	return(sp.st_value);
}


/* look for a symbol near value. */
char *
mod_obj_getsymname(unsigned long value, unsigned long *offset, boolean_t pagetoget, char *retspace)
{
	register char *name;
	struct modctl *modp;

	if ((name = mod_obj_searchsym (mod_obj_kern, value, offset, pagetoget)) != NULL) 
		return (name);

	for (modp = modhead.mod_next;
		modp != &modhead;
		modp = modp->mod_next) {
		if(!(modp->mod_flags & MOD_SYMTABOK))
			continue;
		MOD_HOLD(modp);
		if ((name = mod_obj_searchsym(modp->mod_mp, value, offset, pagetoget)) != NULL) {
			if(retspace != NULL) {
				strncpy(retspace,name,MAXSYMNMLEN);
				name = retspace;
			}
			MOD_RELE(modp);
			return (name);
		}
		MOD_RELE(modp);
	}
	return (NULL);
}


/*
 *	int
 *	mod_obj_ioksym(const char *kname, char *buf, 
 *				size_t buflen, boolean_t indirect, int rw)
 *
 *	Read/Write the kernel space at the address for the symbol given by kname
 *	for length buflen into buf.  One level of indirection can be specified with
 *	the indirect flag. Called from mmioctl in support of the MIOC_READKSYM,
 *	MIOC_IREADKSYM, and MIOC_WRITEKSYM ioctls for /dev/kmem.
 *
 *	Calling/Exit State: kname contains the '\0' terminated name of the symbol
 *	at whose address the io operation(s) will commence.  If the indirect flag
 *	is B_TRUE, then that address is expected to contain a valid kernel address
 *	at which the io requested will actually take place.  FREAD being set in rw
 *	specifies a read operation; otherwise a write is done. buf is the user
 *	buffer to/from which the io will take place. buflen is the length
 *	of the desired io.  If the io is successful, 0 is returned.  Otherwise,
 *	the proper errno is returned.
 */
int
mod_obj_ioksym(const char *kname, char *buf, size_t buflen, boolean_t indirect, int rw)
{
	int error;
	void *kaddr;
	Elf32_Sym ksp;
	struct modctl *modp = NULL;

	ksp.st_value = 0;
	if(mod_obj_lookupone(mod_obj_kern, kname, B_TRUE, &ksp) != 0) {

		/*  kernel is NOT in modhead list */
		for (modp = modhead.mod_next;
			modp != &modhead;
			modp = modp->mod_next) {
			if(!(modp->mod_flags & MOD_SYMTABOK))
				continue;
			/* The module must be held in case there is a request to 
			   unload it while a page fault is being serviced. */
			MOD_HOLD(modp);
			if (mod_obj_lookupone (modp->mod_mp, kname, B_TRUE, &ksp) == 0) 
				break;
			MOD_RELE(modp);
		}

	}
	if(ksp.st_value  == 0)
		return(ENOMATCH);

	kaddr = (void *) ksp.st_value;
	if(indirect)
		kaddr = *(void **) kaddr;

	if(rw & FREAD) {
		error = ucopyout(kaddr,buf,buflen,CATCH_KERNEL_FAULTS);
	} else
		error = ucopyin(buf,kaddr,buflen,CATCH_KERNEL_FAULTS);

	if(modp != NULL)
		MOD_RELE(modp);
	return(error);
}
