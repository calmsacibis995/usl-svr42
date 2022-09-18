/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/mod/mod_obj.c	1.20"
#ident	"$Header: $"

#include <util/debug.h>
#include <util/types.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <util/sysmacros.h>
#include <proc/user.h>
#include <io/uio.h>
#include <fs/file.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/stat.h>
#include <proc/cred.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <proc/obj/elf.h>
#include <mem/seg_kmem.h>
#include <mem/kmem.h>
#include <mem/as.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod.h>
#include <util/mod/mod_obj.h>
#include <util/mod/mod_objmd.h>

STATIC int mod_obj_openpath(const char *modname, const cred_t *credentials, char **path, int *fd);
STATIC int mod_obj_open(const char *pathname, const cred_t *credentials, int *fd);
STATIC int mod_obj_read(int fd, void *ptr, size_t size, unsigned long offset, const cred_t *credentials);
STATIC void mod_obj_close(int fd, const cred_t *credentials);
STATIC void mod_obj_pscnhdrs(struct module *mp, const Elf32_Ehdr *elfhdr, const char *scnhdrs);
STATIC int mod_obj_getprog(struct module *mp, int fd, const Elf32_Ehdr *elfhdr, 
			  const char *scnhdrs, char **dependents, 
			  struct mod_wrapper ***wrapper, const cred_t *credentials);
STATIC int mod_obj_dodep(struct module *mp, const char *dependents, const cred_t *credentials);
STATIC int mod_obj_procsymtab(struct module *mp, int fd, const Elf32_Ehdr *elfhdr, 
				const char *scnhdrs, Elf32_Shdr **symhdr,
				const cred_t *credentials);
STATIC int mod_obj_syminsert(struct module *mp, const char *name, unsigned long index);
STATIC void mod_obj_docommon(struct module *mp, const Elf32_Shdr *symhdr);
STATIC int mod_obj_doreloc(struct module *mp, int fd, const Elf32_Ehdr *elfhdr, 
	                  const char *scnhdrs, const Elf32_Shdr *symhdr,
			  const cred_t *credentials);
STATIC unsigned long mod_obj_lookupall(const struct module *mp, const char *name);



/* round a up to next multiple of b */
#define ALIGN(a, b) ((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))

/*
 *	int
 *	mod_obj_load(const char *modulename, const cred_t *credentials, struct module **retmod)
 *	
 *	called from modld to get the module physically loaded into memory, handle
 *	all symbol table activity, and prepare the module to be executed
 *
 *	Calling/Exit State: The modulename argument is either the full pathname to the
 *	module to be loaded or a simple filename, which is translated into a full
 *	pathname using the modpath PATH mechanism.  The credentials are used in 
 *	the normal file accessing done to load the module. retmod is used to
 *	return a pointer to the struct module created. (The struct module records
 *	information about the module, particularly the symbol table, which is 
 *	necessary for loading other modules which depend on this one as well as
 *	those commands and kernel modules e.g., debugger and profiling which 
 *	require such knowledge.
 *	mod_obj_load returns 0 on success and an errno (in most cases ERELOC) 
 *	otherwise.
 */

int
mod_obj_load(const char *modulename, const cred_t *credentials, struct module **retmod)
{
	int fd = 0;		/* file descriptor from open of modulename */
	struct module *mp = NULL;
	int scnhdrs_size;
	int mod_obj_errno;
	unsigned int saveoff;
	boolean_t paged;

	/* temporary information about module/object file used only during 
    		mod_obj_load and callees */
	Elf32_Ehdr elfhdr;
	Elf32_Shdr *symhdr = NULL;	/* symbol table section header */
	char *scnhdrs = NULL;		/* section headers - 
				  	   char * for arithmetic convenience */
	char *dependents = NULL;	/* list of depdendents from SHT_MOD */
	struct mod_wrapper **wrapper = NULL;/* pointer to wrapper address */


	mp = kmem_zalloc(sizeof(struct module),KM_SLEEP);

	if((mod_obj_errno = mod_obj_openpath(modulename, credentials,&mp->md_path, &fd)) != 0) {
		goto bad;
	}

	if((mod_obj_errno = mod_obj_read(fd, (char *)&elfhdr, sizeof(Elf32_Ehdr), 0, credentials)) != 0) {
		goto bad;
	}


	/* must be an ELF file */
	if(strncmp(ELFMAG, (char *) elfhdr.e_ident, SELFMAG) != 0) {
		cmn_err(CE_NOTE,"!MOD: Bad file type for module %s.\n",mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad;
	}

	/* must be a relocatable object file for this machine type */
	if(elfhdr.e_machine != MOD_OBJ_MACHTYPE || elfhdr.e_type != ET_REL) {
		cmn_err(CE_NOTE,"!MOD: Bad file type for module %s.\n",mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad;
	}

	/* get the section headers from the file */
	scnhdrs = kmem_alloc(elfhdr.e_shentsize * elfhdr.e_shnum, KM_SLEEP);

	if((mod_obj_errno = mod_obj_read(fd, scnhdrs, elfhdr.e_shentsize * elfhdr.e_shnum, elfhdr.e_shoff, credentials)) != 0) {
		cmn_err(CE_NOTE,"Cannot read section headers for module %s.\n", mp->md_path);
		goto bad;
	}



	/* read in all the appropriate loadable sections and fill in
	   dependents so that the info can 
	   be used in later routines */

	if(mod_obj_getprog(mp,fd,&elfhdr,scnhdrs,&dependents,&wrapper,credentials) != 0) {
		cmn_err(CE_NOTE,"!MOD: Getting prog sections in module %s failed.\n", mp->md_path);
		mod_obj_errno = ERELOC;
		goto bad;
		
	}
	if(wrapper == NULL) {
		cmn_err(CE_NOTE,"!MOD: Non-existent or invalid SHT_MOD section in module %s.\n",mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad;
	}

	/* make sure all dependents loaded first - dependents list found in 
	   section SHT_MOD in object file and passed through mp */
	if(mod_obj_dodep(mp, dependents, credentials) != 0) {
		cmn_err(CE_NOTE,"!MOD: Dependency resolution in module %s failed.\n", mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad;
	}


	/* read in symbol table; 
	   fill in symhdr for later use;
	   adjust values for section's  real address;
	   set values for references to kernel and dependents;
	   count bss_COMMON space needed
	*/
	if(mod_obj_procsymtab(mp, fd,&elfhdr, scnhdrs, &symhdr,credentials)  != 0) {
		cmn_err(CE_NOTE,"!MOD: Processing symbol table in module %s failed\n", mp->md_path);
		mod_obj_errno = ERELOC;
		goto bad;
	}


	/* allocate space for all COMMON symbols defined in this file 
	   total space required was calculated in mod_obj_procsymtab */
	mod_obj_docommon(mp,symhdr);

	/* perform relocations specified in relocation tables */
	if(mod_obj_doreloc(mp,fd,&elfhdr,scnhdrs,symhdr,credentials) != 0) {
		cmn_err(CE_NOTE,"!MOD: Relocation in module %s failed\n.", mp->md_path);
		mod_obj_errno = ERELOC;
		goto bad;
	}

	/* save wrapper address */
	mp->md_mw = *wrapper;

	/* done with object file */
	mod_obj_close(fd,credentials);

	/* copy the globals into the symspace - see mod_obj_procsymtab for 
	   more details on this - and free up the temp symbol table */
	bcopy((char *) (symhdr->sh_addr + symhdr->sh_info * symhdr->sh_entsize), 
		mp->md_symspace +mp->md_symentsize, 
		(size_t) (mp->md_strings - mp->md_symspace - mp->md_symentsize));
	kmem_free((char *) symhdr->sh_addr, symhdr->sh_size);
	kmem_free(scnhdrs,elfhdr.e_shentsize * elfhdr.e_shnum);
	*retmod = mp;
	return(0);

bad:
	if(fd != 0)
		mod_obj_close(fd,credentials);
	if(symhdr != NULL)
		kmem_free((char *) symhdr->sh_addr,symhdr->sh_size);
	if(scnhdrs)
		kmem_free(scnhdrs,elfhdr.e_shentsize * elfhdr.e_shnum);
	mod_obj_unload(mp);;
	*retmod = NULL;
	return(mod_obj_errno);

}

/*
 *	STATIC int
 *	mod_obj_pscnhdrs(struct module *mp, const Elf32_Ehdr *elfhdr, const char *scnhdrs) 
 * 	loop through sections to find out how much space we need
 * 	for text, data, (also bss that is already assigned) Called from mod_obj_getprog.
 *
 *	Calling/Exit State: The structure pointed to by mp is used to accumulate
 *	information about the module.  This routine adds information about the 
 *	size of the module in the field md_space_size.  elfhdr and scnhdrs are used
 * 	to get at the necessary information.  
 */

void
mod_obj_pscnhdrs(struct module *mp, const Elf32_Ehdr *elfhdr, const char *scnhdrs) 
{

	unsigned long bits_align, bits_ptr;
	Elf32_Shdr *shp, *shpend;

	bits_align = 0;
	bits_ptr = 0;
	shpend = (Elf32_Shdr *) (scnhdrs + elfhdr->e_shnum * elfhdr->e_shentsize);

	/* loops through all section headers starting with 1 (since shdr 0 is always
	   a dummy in ELF */
	for (shp = (Elf32_Shdr *) (scnhdrs + elfhdr->e_shentsize); shp < shpend; 
		shp = ((Elf32_Shdr *) ((char *) shp + elfhdr->e_shentsize))) {

		if((shp->sh_type != SHT_MOD && shp->sh_type != SHT_PROGBITS && 
			shp->sh_type != SHT_NOBITS) 
			|| !(shp->sh_flags & SHF_ALLOC))
			continue;


		if (shp->sh_addralign > bits_align)
			bits_align = shp->sh_addralign;
		bits_ptr = ALIGN (bits_ptr, shp->sh_addralign);
		bits_ptr += shp->sh_size;
	}
	mp->md_space_size = bits_ptr;
}




/*
 *	STATIC int
 *	mod_obj_getprog (struct module * mp, int fd, const Elf32_Ehdr *elfhdr, 
 *				const char *scnhdrs, char **dependents, 
 *				struct mod_wrapper ***wrapper, const cred_t *credentials)
 *
 *	Read in appropriate sections ((SHT_PROGBITS || SHT_NOBITS || SHT_MOD) 
 *	&& SHF_ALLOC) of module from object file. Called from mod_obj_load.
 *
 *	Calling/Exit State: The structure pointed to by mp accumulates info about
 *	a module.  This routine adds md_space into which the sections are read.
 *	elfhdr, scnhdrs, and credentials are used to access the object file
 *	wrapper is used to return a pointer to the wrapper structure found in
 *	the SHT_MOD section and dependents is used to return the list of modules
 *	upon which this one depends - also found in the SHT_MOD section.
 */
STATIC int
mod_obj_getprog (struct module * mp, int fd, const Elf32_Ehdr *elfhdr, 
			const char *scnhdrs, char **dependents, 
			struct mod_wrapper ***wrapper, const cred_t *credentials)
{
	unsigned long bits_align, bits_ptr;
	Elf32_Shdr *shp, *shpend;

	/*
	 * loop through sections to find out how much space we need
	 * for text, data, (also bss that is already assigned)
	 */
	mod_obj_pscnhdrs(mp, elfhdr, scnhdrs);

	mp->md_space = kmem_zalloc(mp->md_space_size,KM_SLEEP);

	bits_align = 0;
	bits_ptr = (unsigned int)mp->md_space;

	/* now loop though sections assigning addresses and loading the data */
	shpend = (Elf32_Shdr *) (scnhdrs + elfhdr->e_shnum * elfhdr->e_shentsize);

	for (shp = (Elf32_Shdr *) (scnhdrs +elfhdr->e_shentsize); shp < shpend; 
		shp = ((Elf32_Shdr *) ((char *) shp + elfhdr->e_shentsize))) {

		switch(shp->sh_type) {

			/* only interested in loading module, allocated program,
			      and bss sections */
			case SHT_PROGBITS:
			case SHT_NOBITS:
			case SHT_MOD:
		     		if(shp->sh_flags & SHF_ALLOC)
		     			break;
			default:
				continue;
		}

		bits_ptr = ALIGN (bits_ptr, shp->sh_addralign);
		if (shp->sh_type == SHT_PROGBITS || shp->sh_type == SHT_MOD) {
			if (mod_obj_read (fd, (char *)bits_ptr,
			    shp->sh_size, shp->sh_offset, credentials) != 0)
				return (-1);
			if(shp->sh_type == SHT_MOD) {
				*wrapper = (struct mod_wrapper **) bits_ptr;
				if(shp->sh_size > sizeof(struct mod_wrapper *))
					*dependents = (char *) bits_ptr + 
						 	sizeof(struct mod_wrapper *);
				else
					*dependents = NULL;
			}
		}
		shp->sh_type = SHT_PROGBITS;
		shp->sh_addr = bits_ptr;

		bits_ptr += shp->sh_size;
	}
	return (0);
}


/*
 *	STATIC int
 *	mod_obj_dodep(struct module *mp, const char *dependents, const cred_t *credentials)
 *
 *	If this module depends on other modules as given by the module in its 
 *	SHT_MOD section (and as collected by mod_obj_getprog), make sure those
 * 	modules are loaded and record the dependency both by upping the dependency
 * 	count of the module depended upon and by keeping a pointer to the modctl
 *	structure for that module so the dependency count can later by decreased
 *	when this module is unloaded. Called from mod_obj_load.
 *
 *	Calling/Exit State: The structure pointed to by mp accumulates data
 *	about the module as it is being loaded.  This routine adds to md_mcl a list
 *	of modctl pointers for modules upon which this module depends.
 *	The dependents argument has a space separated list of dependent
 *	module names and the credentials are needed tlo load modules as necessary.
 *	This routine returns 0 if all necessary dependencies are satisfied and
 *	-1 otherwise. Assumes calling routine will clean up partially
 *	created modctl list (md_mcl) and decrease the dependent counts
 *	on the modules in the list appropriately.
 */
STATIC int
mod_obj_dodep(struct module *mp, const char *dependents, const cred_t *credentials)
{
	const char *p;
	const char *limit;
	char *q;
	struct modctl *modp;
	struct modctl_list *mclp;
	char **statmod;

	char dependent[MODMAXNAMELEN];

	if(dependents == NULL)
		return(0);

	p = dependents;
	limit = p + strlen(dependents);

	while (1) {
		/* skip whitespace */
		while (p < limit && (*p == ' ' || *p == '\t'))
			p++;
		if (p >= limit || *p == 0)
			break;
		q = dependent;
		while (q - dependent <
		    MODMAXNAMELEN && p < limit && *p &&
		    *p != ' ' && *p != '\t') {
			*q++ = *p++;
		}
		*q = 0;

		if(!mod_static(dependent)) {
			/* attempt to autoload the module */
			if(modld(dependent,sys_cred, &modp) != NULL) {
				cmn_err(CE_NOTE,"!MOD: Cannot load required dependent %s for module %s.\n",dependent,mp->md_path);
				return (-1);
			}


			/* dependent reference count of dependee increased so cannot be unloaded
		   	while this module is still loaded */
			modp->mod_depcnt++;
	
			/* add to list of modules that this module depends upon */
			mclp = (struct modctl_list *) 
			  	kmem_alloc(sizeof(struct modctl_list), KM_SLEEP);
			mclp->mcl_mcp = modp;
			mclp->mcl_next = mp->md_mcl;
			mp->md_mcl = mclp;

		}
		/* skip to next whitespace */
		while (p < limit && *p && *p != ' ' && *p != '\t')
			p++;
	}

	return (0);
}

/*
 *	STATIC int
 *	mod_obj_procsymtab(struct module *mp, int fd, const Elf32_Ehdr *elfhdr, 
 *
 *	sets up the symbol table for later use and resolves symbol references and
 *	calculates size of space needed for COMMON symbols defined in this module.
 *	Called from mod_obj_load.
 *
 *	Calling/Exit State: The structure pointed to by mp accumulates information
 *	about the module being loaded.  This routine adds a pointer to the symbol
 *	table space (md_symspace), including space for the string table (md_strings),
 *	the hash buckets (md_buckets), and the hash chains (md_chains), 
 *	the size of that space (md_symsize), whether that space is paged (md_page), 
 *	the size of a symbol table entry (md_symentsize) and the size of the space 
 *	needed for COMMON symbols defined in this module. fd is the file descriptor for
 *	the file of the module being loaded. elfhdr, scnhdrs, and credentials are all
 *	used to access the file. symhdr is used to return a pointer to the symbol
 *	table section header with the sh_addr field pointing to the symbol table
 *	(including all locals). md_symspace does not actually contain the symbol table
 *	until the end of mod_obj_load and then only contains the globals (see
 *	comments in this routine and mod_obj_load for more details).
 *	Returns 0 if all OK, non-zero if error such as unresolved reference.
 */
STATIC int
mod_obj_procsymtab(struct module *mp, int fd, const Elf32_Ehdr *elfhdr, 
		     const char *scnhdrs, Elf32_Shdr **symhdr, const cred_t *credentials)
{
	unsigned int i;
	Elf32_Sym *sp, *spend;
	Elf32_Shdr *shp, * strhdr;
	Elf32_Shdr *shend;
	Elf32_Shdr *lsymhdr;	
	char *symname;
	int err = 0;
	unsigned int bss_align= 0;
	unsigned int bss_ptr = 0;
	unsigned int ngsyms;
	unsigned long kval;


	/* find symhdr */
	shend = (Elf32_Shdr *) (scnhdrs + elfhdr->e_shnum * elfhdr->e_shentsize);
	for(lsymhdr = (Elf32_Shdr *) (scnhdrs + elfhdr->e_shentsize);
		lsymhdr < shend; lsymhdr = (Elf32_Shdr *) ((char *) (lsymhdr) + elfhdr->e_shentsize)) {
		if((lsymhdr)->sh_type == SHT_SYMTAB)
			break;
	}

	if((lsymhdr) >= shend) {
		cmn_err(CE_NOTE,"!MOD: No symbol table in module %s.\n",mp->md_path);
		return(-1);
	}

	/* get the associated string table header */
	if (lsymhdr->sh_link >= elfhdr->e_shnum) {
		cmn_err(CE_NOTE,"!MOD: String table link for symbol table in module %s is invalid.\n",mp->md_path);
		return (-1);
	}

	strhdr = (Elf32_Shdr *) (scnhdrs + lsymhdr->sh_link * elfhdr->e_shentsize);
	if(strhdr->sh_type != SHT_STRTAB) {
		cmn_err(CE_NOTE,"!MOD: Symbol table does not point to valid strig table in module %s\n",mp->md_path);
		return(-1);
	}


	mp->md_symentsize = lsymhdr->sh_entsize;

	/* keep full symbol table here for relocation purposes;
	  allocate only enough memory in symspace for the globals (including the
	  empty symbol entry);
	  at the end of mod_obj_load, the globals will be bcopied to symspace;
	  this allows all symspace to be allocated and freed as one ;
	  use of lsymhdr->sh_addr should be safe since no symbols are defined
	  in the symbol table section  and no relocations done relative to 
	  symhdr section */

	lsymhdr->sh_addr = (Elf32_Addr) kmem_zalloc(lsymhdr->sh_size,KM_SLEEP);


	ngsyms = lsymhdr->sh_size / mp->md_symentsize - lsymhdr->sh_info + 1;

	/* space for all globals, the whole string header (do not want to incur
	   overhead of copying only strings for globals), the buckets, the chains
	   slots (again only for globals), and an extra long for rounding slop */
	mp->md_symsize = ngsyms * mp->md_symentsize +
		strhdr->sh_size +
		(1 + MOD_OBJHASH + ngsyms) * sizeof(unsigned long);

	mp->md_symspace = mod_obj_getspace(mp->md_symsize,(boolean_t *)&mp->md_page);

	mp->md_strings = mp->md_symspace + ngsyms * mp->md_symentsize;
	mp->md_buckets = (unsigned long *) 
	   ((((int)mp->md_strings + strhdr->sh_size) | (sizeof(unsigned long)-1)) + 1);
	mp->md_chains = mp->md_buckets + MOD_OBJHASH; 

	if(mod_obj_read(fd, (char *) lsymhdr->sh_addr, lsymhdr->sh_size, lsymhdr->sh_offset, credentials) != 0 ||
	   mod_obj_read(fd, mp->md_strings, strhdr->sh_size, strhdr->sh_offset, credentials) != 0 ) {
		cmn_err(CE_NOTE,"!MOD: Cannot read symbol table or string table in module %s.\n",mp->md_path);
		return(-1);
	}

	bzero((char *) mp->md_buckets, 
		(MOD_OBJHASH + ngsyms + 1) * sizeof(long));
	bzero(mp->md_symspace, mp->md_symentsize);


	/*
	 * loop through the symbol table adjusting values to account
	 * for where each section got loaded into memory.  Also
	 * fill in the hash table.
	 */

	spend = (Elf32_Sym *) (lsymhdr->sh_addr + lsymhdr->sh_size);

	for (sp = (Elf32_Sym *) (lsymhdr->sh_addr + lsymhdr->sh_entsize), i = 1; sp < spend; 
		sp = (Elf32_Sym *) ((char *) sp + lsymhdr->sh_entsize), i++) {

		/* SHN_COMMON and SHN_ABS both above SHN_LORESERVE */
		if (sp->st_shndx < SHN_LORESERVE) {
			if (sp->st_shndx >= elfhdr->e_shnum) {
				cmn_err(CE_NOTE, "!MOD: bad shndx for symbol index %d\n",i);
				err = -1;
				continue;
			}
			shp = (Elf32_Shdr *) (scnhdrs +
					 sp->st_shndx * elfhdr->e_shentsize);

			sp->st_value += shp->sh_addr;
		}


		/* net result of code below is that only globals defined in this
		    file wind up being hashed and therefore found on subsequent
		    lookup */

		/* eliminate locals */
		if(ELF32_ST_BIND(sp->st_info) == STB_LOCAL)
			continue;

		if (sp->st_name == 0)
			continue;
		if (sp->st_name >= strhdr->sh_size) {
			cmn_err(CE_NOTE,"!MOD: String table index for symbol %d in module %s is out of range.\n",i,mp->md_path);
			return (-1);
		}

		symname = mp->md_strings + sp->st_name;

		   /* just need to hash  those symbols that are already known
		   to be defined in this file ;
		   hash with index relative to globals since symspace will only
		   have the globals (assumes that NO calls to mod_obj_lookupone for
		   this module until after return from mod_obj_load when the
		   globals have been copied to symspace)
		   */
		if(sp->st_shndx != SHN_UNDEF && sp->st_shndx != SHN_COMMON) {
			err |= mod_obj_syminsert (mp, symname, i - lsymhdr->sh_info + 1);
			continue;
		}


		/* look for symbol resolving reference */
		if ((kval = mod_obj_lookupall (mp, symname)) != 0) {
			sp->st_shndx = SHN_ABS;	
			sp->st_value = kval;
			continue;
		}

		if (sp->st_shndx == SHN_UNDEF && ELF32_ST_BIND(sp->st_info) != STB_WEAK) {
			cmn_err(CE_NOTE, "!MOD: Undefined symbol %s in module %s.\n", symname,mp->md_path);
			err = -1;
			continue;
		}

		/* it's a common symbol defined in this file - 
		    st_value is the required alignment */
		if (sp->st_value > bss_align)
			bss_align = sp->st_value;
		bss_ptr = ALIGN (bss_ptr, sp->st_value);
		bss_ptr += sp->st_size;
		err |= mod_obj_syminsert (mp, symname, i - lsymhdr->sh_info +1);

	}
	mp->md_bss_size = bss_ptr + bss_align;

	*symhdr = lsymhdr;
	return (err);
}

/*
 *	STATIC int
 *	mod_obj_docommon(struct module *mp, const Elf32_Shdr *symhdr)
 *	
 *	allocate space for COMMON symbols defined in this file and adjust symbol
 *	table pointers accordingly. Called from mod_obj_load.
 *
 *	Calling/Exit State: The structure pointed to by mp is used to
 *	accumulate information about the module being loaded.  This routine adds
 *	the space for the COMMON symbols defined in this module (md_bss).
 *	size of space needed calculated in mod_obj_procsymtab and "passed" in
 *	mp->md_bss_size.
 *	symhdr is used to update the symbol table entries.
 */
STATIC void
mod_obj_docommon(struct module *mp, const Elf32_Shdr *symhdr)
{
	Elf32_Sym *sp, *symend;
	unsigned long bss_ptr;

	if(mp->md_bss_size)
	{
		mp->md_bss = kmem_zalloc(mp->md_bss_size, KM_SLEEP);

		bss_ptr = (unsigned long) mp->md_bss;
		symend = (Elf32_Sym *) (symhdr->sh_addr + symhdr->sh_size);

		/* loop through symbols starting with first STB_GLOBAL */
		for (sp= (Elf32_Sym *) (symhdr->sh_addr + 
				symhdr->sh_info * symhdr->sh_entsize); 
		     sp < symend; 
		     sp = (Elf32_Sym *) (((char *) sp) + symhdr->sh_entsize)) {

		     	if(sp->st_shndx != SHN_COMMON)
				continue;

			bss_ptr = ALIGN(bss_ptr, sp->st_value);
			sp->st_shndx = SHN_ABS;
			sp->st_value = bss_ptr;
			bss_ptr += sp->st_size;
		}
	}

}

/*
 *	STATIC int
 *	mod_obj_doreloc(struct module *mp, int fd, const Elf32_Ehdr *elfhdr, 
 *	       const char *scnhdrs, const Elf32_Shdr *symhdr, const cred_t *credentials)
 *
 *	perform symbol relocation for this module. Called from mod_obj_load.
 *	This routine calls the machine dependent mod_obj_relone.
 *
 *	Calling/Exit State: The structure pointed to by mp is used to accumulate 
 *	information about the module being loaded. This routine does not add
 *	any new information but uses some information accumulated in other routines.
 *	The reminaing arguments are used to access the relocation and symbol table
 *	information for the module. This routine returns 0 if all relocations 
 *	were successful, non-zero otherwise.
 */
STATIC int
mod_obj_doreloc(struct module *mp, int fd, const Elf32_Ehdr *elfhdr, 
	       const char *scnhdrs, const Elf32_Shdr *symhdr, const cred_t *credentials)
{

	Elf32_Shdr *shp, *shend;
	const Elf32_Shdr *rshp;
	unsigned int nreloc;
	Elf32_Rel *reltbl = NULL;


	shend = (Elf32_Shdr *) 
	     (scnhdrs + elfhdr->e_shnum * elfhdr->e_shentsize);

	/* find relocation headers skipping hdr 0 since that is always a dummy */
	for (rshp = (Elf32_Shdr *) (scnhdrs + elfhdr->e_shentsize); rshp < shend; 
	      rshp = (Elf32_Shdr *) ((char *) rshp + elfhdr->e_shentsize)) {

		if(!MOD_OBJ_VALRELOC(rshp->sh_type)) {
			if(MOD_OBJ_ERRRELOC(rshp->sh_type)) {
				cmn_err(CE_NOTE, "!MOD: Can't process reloc type %d in module %s.\n", rshp->sh_type, mp->md_path);
				goto bad;
			}
			continue;
		}

		if ((Elf32_Shdr *)(scnhdrs + 
		       rshp->sh_link * elfhdr->e_shentsize) != symhdr) {
			cmn_err(CE_NOTE, "!MOD: Reloc for non-default symtab in module %s.\n",mp->md_path);
			goto bad;
		}

		if (rshp->sh_info >= elfhdr->e_shnum) {
			cmn_err(CE_NOTE, "!MOD: Sh_info out of range %d in module %s.\n", rshp->sh_info, mp->md_path);
			goto bad;
		}
		/* get the section header that this reloc table refers to */
		shp = (Elf32_Shdr *)
			(scnhdrs + rshp->sh_info * elfhdr->e_shentsize);

		/* skip relocations for non allocated sections */
		if(!(shp->sh_flags & SHF_ALLOC))
			continue;

		nreloc = rshp->sh_size / rshp->sh_entsize;
		reltbl = (Elf32_Rel *)kmem_alloc(rshp->sh_size, KM_SLEEP);

		if(mod_obj_read(fd, reltbl, rshp->sh_size, rshp->sh_offset, credentials) != 0) {
			cmn_err(CE_NOTE,"!MOD: Read of relocation table in module %s failed.\n", mp->md_path);
			return(-1);
		}


		if (mod_obj_relone(mp, reltbl, nreloc, rshp->sh_entsize, shp, symhdr) != 0) {
			goto bad;
		}

		kmem_free(reltbl, rshp->sh_size);
		reltbl = NULL;
	}
	return (0);
bad:
	if (reltbl)
		kmem_free(reltbl, rshp->sh_size);
	return (-1);
}




/*
 *	STATIC unsigned long
 *	mod_obj_lookupall (const struct module *mp, const char *name)
 *	
 *	look for symbol given by name in modules upon which the module given
 *	by mp depends and in the static kernel module returning the value of that
 *	symbol.  This is used primarily by mod_obj_procsymtab when doing symbol 
 *	resolution.
 *
 *	Calling/Exit State: as described above. Returns 0 if symbol is not found.
 */
STATIC unsigned long
mod_obj_lookupall (const struct module *mp, const char *name)
{
	Elf32_Sym sp;
	struct modctl_list *mclp;
	struct modctl	*modctlp;

	for (mclp = mp->md_mcl; mclp; mclp = mclp->mcl_next) {
		if (mod_obj_lookupone (mclp->mcl_mcp->mod_mp, name, B_TRUE, &sp) == 0)
			return (sp.st_value);
	}
	if (mod_obj_lookupone (mod_obj_kern, name, B_TRUE, &sp) == 0)
		return (sp.st_value);

	return (0);
}

/*
 *	STATIC int
 *	mod_obj_syminsert (struct module *mp, const char *name, unsigned long index)
 *	
 *	adds symbol given by name and symbol table index 
 *	to hash tables of module given by mp.  CATCH FAULTS necessary because symbol
 *	space may be paged.
 *
 *	Calling/Exit State: as described above.  Returns 0 if success and non-zero
 *	if error in paging the symbol table.
 */
STATIC int
mod_obj_syminsert (struct module *mp, const char *name, unsigned long index)
{
	unsigned long hval;
	hval = mod_obj_hashname(name) % MOD_OBJHASH;
	CATCH_FAULTS(CATCH_KPAGE_FAULT) {
		mp->md_chains[index] = mp->md_buckets[hval];
		mp->md_buckets[hval] = index;
	}
	return(END_CATCH());
}

STATIC char * mpath = MOD_DEFPATH;	/* current search path for modules */


/*
 *	STATIC int
 *	mod_obj_openpath(const char *name, const cred_t *credentials, 
 *			char** pathname, int *retfd)
 *
 *	If given name is an absolute pathname then tries to open else
 *	searches for module along mpath by concatenating the given name with each
 *	path given and trying to open the resulting pathname
 *	Called from mod_obj_load.
 *
 *	Calling/Exit State: name contains the filename of the module to open.
 *	credentials is needed to do the open. pathname is used to pass a pointer
 *	to the pathname actually opened back to the calling routine and retfd
 *	is used to pass the resulting file descriptor back.
 *	The routine returns 0 if the file for the module is successfully opened,
 *	non-zero otherwise.
 */
STATIC int
mod_obj_openpath(const char *name, const cred_t *credentials, char** pathname, int *retfd)
{
	char *p;
	const char *q;
	char *pathp;
	char *pathpsave;
	char *fullname;
	int maxpathlen;
	int fd;
	int err;


	/*
 	* fullname is dynamically allocated to be able to hold the
 	* maximum size string that can be constructed from name.
 	* path is exactly like the shell PATH variable.
 	*/
	if (name[0] == '/')
		pathp = "";		/* use name as specified */
	else
		pathp = mpath;	/* do path search */

	pathpsave = pathp;		/* keep this for error reporting */

	/*
	 * Allocate enough space for the largest possible fullname.
	 * since path is of the form <directory>:<directory>: ...
	 * we're potentially allocating a little more than we need to
	 * but we'll allocate the exact amount when we find the right directory.
	 * (The + 2 below is one for NULL terminator 
	 * and one for
	 * the '/' between path and name.)
	 */
	maxpathlen = strlen(pathp) + strlen(name) + 2;
	fullname = kmem_alloc(maxpathlen,KM_SLEEP);

	while (1) {
		p = fullname;
		while (*pathp && *pathp != ':')
			*p++ = *pathp++;
		if (p != fullname && p[-1] != '/')
			*p++ = '/';
		q = name;
		while (*q)
			*p++ = *q++;
		*p = 0;
		if ((err = mod_obj_open(fullname, credentials, &fd)) == 0) {
			*pathname = kmem_zalloc(strlen(fullname)+1,KM_SLEEP);
			strcpy(*pathname,fullname);
			kmem_free(fullname,maxpathlen);
			*retfd = fd;
			return (0);
		}
		if (*pathp == 0)
			break;
		pathp++;
	}
	kmem_free(fullname, maxpathlen);
	cmn_err(CE_NOTE, "!MOD: Cannot open module %s on path %s\n", name, pathpsave);
	*retfd = 0;
	return (err);
}

STATIC unsigned int mod_pathspacelen;	/* keep track of space size allocated so can
						free on next call to modpath */
struct mpatha {
	char * dirname;
};

/*
 *	int
 *	modpath(const struct mpatha *uap, rval_t *rvp)
 *
 *	system call interface for changing the path used to serach for modules.
 *	A non-NULL argumnet is an absolute pathname to be prepended to the existing 
 *	search path.  A NULL argument causes the path to be set back to the default.
 *	The P_LOADMOD privilege is required to perform this system call.
 *
 *	Calling/Exit State: uap->dirname has a pointer to the pathname to be added or
 *	NULL. rvp is used to pass back 0 on success and -1 on failure.  The routine
 *	returns 0 on success and the proper errno on failure.
 */
int
modpath(const struct mpatha *uap, rval_t *rvp)
{
	char *checkp;
	char *newmodpath;
	char *p, *q;
	unsigned int len;
	int error;

	if(pm_denied(u.u_cred, P_LOADMOD)) {
		rvp->r_val1 = -1;
		return(EPERM);
	}


	if(uap->dirname) {
		newmodpath = kmem_alloc(MAXPATHLEN + strlen(mpath) + 2, KM_SLEEP);

		if((error = copyinstr(uap->dirname,newmodpath, MAXPATHLEN, &len)) != 0) {
			rvp->r_val1 = -1;
			return(error);
		}
		*(newmodpath+len) = '\0';
		len = MAXPATHLEN + strlen(mpath) + 2;
		p = newmodpath;

		/* must be an absolute pathname */
		if(*p != '/') {
		    	kmem_free(newmodpath, len);
			rvp->r_val1 = -1;
			return(EINVAL);
		}
		while(*p) {
			if(*p == ':' || *p == ' ' ) {
				p++;
				if(*p != '/') {
				    	kmem_free(newmodpath, len);
					rvp->r_val1 = -1;
					return(EINVAL);
				}
			}
			p++;
		}
		*p++ = ':';
		q = mpath;
		while(*p++ = *q++)
			;
		q = mpath;
		mpath = newmodpath;
	}
	else {
		q = mpath;
		mpath = MOD_DEFPATH;
	}
	if(strcmp(q,MOD_DEFPATH) != 0)
		kmem_free(q,mod_pathspacelen);
	mod_pathspacelen = len;
	return(0);
}



/*
 *	STATIC int
 *	mod_obj_open(const char *pathname, const cred_t *credentials, int *fd)
 *
 *	Tries to open the file given by pathname using vn_open.  If the 
 *	credentials argument is sys_cred, then the u_area's notion of root
 *	and the credentials are changed to be the system root and sys_cred
 *	so that vn_open and lookuppn will find the right file and be able to open
 *	it.  They are restored to their proper values after the return from vn_open.
 *	The credentials are only sys_cred when this is a call to 
 *	autoload a module. demand loading via the system call has the user's
 *	credentials in the credentials variable and therefore uses the user's
 *	idea of root and the credentials to find and open the file.
 *
 *	Calling/Exit State: pathname contains the file to be opened.
 *	credentials may be used to open the file. On a successful open,
 *	this routine returns 0 and fd is used to return the resulting file
 *	descriptor (which is really a vnode_t *).  On failure the routine returns
 *	non-zero.
 */

STATIC int
mod_obj_open(const char *pathname, const cred_t *credentials, int *fd)
{

	vnode_t *saveroot;
	cred_t *savecred;
	int error;

	if(credentials == sys_cred) {
		/* autoload */
		/* allows code to find module from real root rather than chroot */
		saveroot = u.u_rdir;
		u.u_rdir = rootdir;
		/* allows module to be loaded by system even if user is not privileged */
		savecred = u.u_cred;
		u.u_cred = sys_cred;
		error = vn_open(pathname, UIO_SYSSPACE,FREAD,0, (vnode_t **) fd, 0);
		u.u_rdir = saveroot;
		u.u_cred = savecred;
		return(error);
	}
	else {
		/* demand load */
		return(vn_open(pathname, UIO_SYSSPACE,FREAD,0, (vnode_t **) fd, 0));
	}

}


/*
 *	STATIC void
 *	mod_obj_close(int fd, const cred_t *credentials)
 *	
 *	close the open file descriptor (really a vnode_t * originally obtained from
 *	mod_obj_open) using VOP_CLOSE. The vnode is also released after the close.
 *
 *	Calling/Exit State: fd is a pointer to the vnode to be closed and
 *	the credentials are used to do so.
 */
STATIC void
mod_obj_close(int fd, const cred_t *credentials)
{
	VOP_CLOSE((vnode_t *) fd, FREAD, 1, 0, credentials);
	VN_RELE((vnode_t *) fd);
}

/*
 *	STATIC int
 *	mod_obj_read(int fd, void *buf, size_t size, 
 *			unsigned long offset, const cred_t *credentials)
 *
 *	read information from the file given by the file descriptor using vn_rdwr
 *
 *	Calling/Exit State: fd contains a pointer to the vnode for the file to be read.
 *	size bytes are read starting at offset into buf using the credentials.
 *	On success the rouine returns 0, else non-zero.
 */
STATIC int
mod_obj_read(int fd, void *buf, size_t size, unsigned long offset, const cred_t *credentials)
{
	int resid;

	return(vn_rdwr(UIO_READ, (vnode_t *) fd, buf, size, offset, 
		UIO_SYSSPACE, 0, 0, credentials, &resid));
}

/*
 *	void
 *	mod_obj_unload(struct module * mp)
 *
 *	frees up all space associated with module and "notifies" modules upon
 *	which this one depends that the dependency no longer exists
 *
 *	Calling/Exit State: mp is used to access all the space to be freed
 *	and the dependee modules and then the space pointed to by mp is freed.
 */
void
mod_obj_unload(struct module * mp)
{
	struct modctl_list *mclp, *mclp2;
	mclp = mp->md_mcl;

	/* if there are any modules on which this module depends ... */
	if(mclp) {
		do {
			/* let it know the dependency no longer exists */
			mclp->mcl_mcp->mod_depcnt--;
			if(!MODBUSY(mclp->mcl_mcp))
				UNLOAD(mclp->mcl_mcp);
			mclp2 = mclp->mcl_next;
			kmem_free(mclp,sizeof(struct modctl_list));
		} while(mclp = mclp2);
	}
	if(mp->md_bss)
		kmem_free(mp->md_bss, mp->md_bss_size);
	if(mp->md_space)
		kmem_free(mp->md_space, mp->md_space_size);
	if(mp->md_symspace)  {
		if(mp->md_page) {
			(void) as_unmap(&kas,mp->md_symspace,ctob(btoc(mp->md_symsize)));
		}
		else
			kmem_free(mp->md_symspace, mp->md_symsize);
	}
	if(mp->md_path)
		kmem_free(mp->md_path,strlen(mp->md_path)+1);
	kmem_free(mp, sizeof(struct module ));
}

/*
 *	void
 *	mod_obj_getmodstatus(struct module *mp, struct modstatus *msp)
 *
 *	Fills in modstatus structure with  information (location, size, and pathname) 
 *	on module as stored in struct module pointed to by mp
 *
 *	Calling/Exit State: As described above.
 */
void
mod_obj_getmodstatus(struct module *mp, struct modstatus *msp)
{
	msp->ms_base = mp->md_space;
	msp->ms_size = mp->md_space_size + mp->md_bss_size;
	strncpy(msp->ms_path,mp->md_path,MAXPATHLEN);
}


		
STATIC int
mod_obj_getehsh(int fd, Elf32_Ehdr *elfhdr, char **scnhdrs, const char *modname, const cred_t *credentials)
{
	int error;
	if((error = mod_obj_read(fd, (char *)elfhdr, sizeof(Elf32_Ehdr), 0, credentials)) != 0) {
		return(error);
	}


	if(strncmp(ELFMAG, (char *) elfhdr->e_ident, SELFMAG) != 0) {
		cmn_err(CE_NOTE,"!MOD: Bad file type for module %s.\n",modname);
		return(EINVAL);
	}

	if(elfhdr->e_machine != MOD_OBJ_MACHTYPE || elfhdr->e_type != ET_REL) {
		cmn_err(CE_NOTE,"!MOD: Bad file type for module %s.\n",modname);
		return(EINVAL);
	}
	*scnhdrs = kmem_alloc(elfhdr->e_shentsize * elfhdr->e_shnum, KM_SLEEP);
	if((error = mod_obj_read(fd, *scnhdrs, elfhdr->e_shentsize * elfhdr->e_shnum, elfhdr->e_shoff, credentials)) != 0) {
		cmn_err(CE_NOTE,"Cannot read section headers for module %s.\n", modname);
		return(error);
	}
	return(0);
}

/*
 *	unsigned long
 *	mod_obj_lookup(const struct module *mp, const char *name)
 *
 *	Find value associated with symbol given by name in module given by mp.
 *	Called primarily when resolving stub references.
 *
 *	Calling/Exit State: As described above.  Returns value of found symbol or
 *	0 if no symbol is found.
 */
unsigned long
mod_obj_lookup(const struct module *mp, const char *name)
{
	Elf32_Sym sp;

	if(mod_obj_lookupone(mp,name,B_TRUE,&sp) != 0)
		return(0);
	return(sp.st_value);
}
