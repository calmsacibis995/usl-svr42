/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/obj/elf.c	1.11"
#ident	"$Header: $"

/*
 * This file contains ELF object specific routines.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/resource.h>
#include <proc/regset.h>
#include <fs/vnode.h>
#include <proc/mman.h>
#include <mem/kmem.h>
#include <util/sysinfo.h>
#include <proc/proc.h>
#include <util/debug.h>
#include <io/uio.h>
#include <svc/systm.h>
#include <proc/obj/elftypes.h>
#include <proc/obj/elf.h>
#include <proc/obj/elf_386.h>
#include <proc/obj/x.out.h>
#include <proc/auxv.h>
#include <proc/exec.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prsystm.h>
#include <mem/as.h>
#include <mem/seg.h>

/* Enhanced Application Compatibility Support */
#include <io/termios.h>
#include <svc/sco.h>
/* End Enhanced Application Compatibility Support */

#ifdef WEITEK
#include <util/weitek.h>
#endif

short elfmagic = 0x457f;		/* because of reverse byte ordering */

u_long	elf_stackgap = (u_long) (128 * 1024 * 1024);	/* new virtual map */
u_long	elf_stackpgs = (u_long) (288 * 1024);		/* support */
extern int	userstack[];

int getelfhead();

/* Enhanced Application Compatibility Support */
STATIC char *strnstr();
/* End Enhanced Application Compatibility Support */

/*
 * ELF specific exec routine, called through
 * the execsw table.
 */
int
elfexec(vp, args, level, execsz, ehdp, setid)
struct vnode *vp;
struct uarg *args;
int level;
long *execsz;
exhda_t *ehdp;
int setid;
{
	Elf32_Ehdr	*ehdrp;
	Elf32_Phdr	*phdr;
	caddr_t		phdrbase = 0;
	caddr_t 	base = 0;
	char		*dlnp;
	int		dlnsize, fd, *aux, resid, error;
	long		voffset;
	struct execenv	exenv;
	struct proc *pp = u.u_procp;
	struct vnode	*nvp;
	Elf32_Phdr	*dyphdr = NULL;
	Elf32_Phdr	*stphdr = NULL;
	Elf32_Phdr	*uphdr = NULL;
	Elf32_Phdr	*junk = NULL;
	long		phdrsize;
	int		elfargs[16];
	int		postfixsize = 0;
	register	int i, hcnt;
	int		hasu = 0;
	int		hasdy = 0;
	exhda_t		dehdr;
	struct vattr	vattr;
	caddr_t		lowest_addr = (caddr_t) 0xFFFFFFFF;

	/*
	 * Get the ELF header and program header table.
	 */
	if ((error = getelfhead(&ehdrp, &phdrbase, &phdrsize, ehdp)) != 0)
		return error;

	if (ehdrp->e_type == ET_DYN)
		return ELIBEXEC;

	/*
	 * Determine aux size now so that stack can be built
	 * in one shot (except actual copyout of aux image)
	 * and still have this code be machine independent.
	 */
	hcnt = ehdrp->e_phnum;
	for (i = 0; i < hcnt; i++) {
		phdr = (Elf32_Phdr *)(phdrbase + (ehdrp->e_phentsize *i));
		switch(phdr->p_type) {
		case PT_INTERP:
			hasdy = 1;
			break;
		case PT_PHDR:
			hasu = 1;
			break;
		case PT_LOAD:
				if ((caddr_t)phdr->p_vaddr < lowest_addr)
					lowest_addr = (caddr_t) phdr->p_vaddr;
		break;
		}
	}
	if (hasdy) {
		/* amount of additional info on stack for dynamic linker */
		args->auxsize = (hasu ? (16 * NBPW) : (10 * NBPW));
	}

	/*
	 *	New virtual map support.
	 */
	if (lowest_addr == (caddr_t) 0xFFFFFFFF ||
			(u_long)lowest_addr >= (elf_stackgap + elf_stackpgs)) {
		u.u_userstack = (elf_stackgap + elf_stackpgs) - sizeof(int);
		u.u_shmbase = (caddr_t)(elf_stackgap + elf_stackpgs);
		u.u_shmend = (caddr_t)((uint)lowest_addr & PAGEMASK);
	} else	
		u.u_userstack = (u_long) userstack;

	/*
	 * Remove current process image, and allocate new address space.
	 */
	if ((error = remove_proc(args)) != 0)
		return error;

	/*
	 * Map in executable.
	 */
	if ((error = mapelfexec(vp, ehdrp, phdrbase, &uphdr, &dyphdr, &stphdr, 
				&base, &voffset, execsz, ehdp)) != 0)
			goto bad;

	if (stphdr != NULL){
		/* call COFF stuff for shared libraries */
		if (error = elf_coffshlib(vp, stphdr, execsz, ehdp))
			goto bad;
	}

	if (uphdr != NULL && dyphdr == NULL)
		goto bad;

	/*
	 * Dynamic linking required.
	 */
	if (dyphdr != NULL)	{

		dlnsize = dyphdr->p_filesz;	/* length of linker's path name */

		if (dlnsize > MAXPATHLEN || dlnsize <= 0)
			goto bad;

		/*
		 * Get path name of dynamic linker.
		 */
		error = exhd_getmap(ehdp,
			dyphdr->p_offset,
			dyphdr->p_filesz,
			EXHD_NOALIGN,
			(caddr_t) &dlnp);
		if (error)
			goto bad;

		if (dlnp[dlnsize - 1] != '\0')
			goto bad;

		/*
		 * Get vnode of linker.
		 */
		if (error = lookupname(dlnp, UIO_SYSSPACE, FOLLOW,
			NULLVPP, &nvp))
				goto bad;


		aux = elfargs;
		if (uphdr){
			exenv.ex_brkbase = base;  
			exenv.ex_magic = elfmagic;
			exenv.ex_vp = vp;
			setexecenv(&exenv);

			/*
			 * If the program header table (PHT) is part of the
			 * memory image, let the linker know about it.
			 */
			*aux++ = AT_PHDR;	/* address of the PHT */
			*aux++ = (int)uphdr->p_vaddr + voffset;
			*aux++ = AT_PHENT;	/* size of PHT entry */
			*aux++ = ehdrp->e_phentsize;	
			*aux++ = AT_PHNUM;	/* number of PHT entries */
			*aux++ = ehdrp->e_phnum;	
			*aux++ = AT_ENTRY;	/* entry point of program */
			*aux++ = ehdrp->e_entry + voffset;	
			postfixsize += 8 * NBPW;
		} else {
			/*
			 * The PHT is not part of the memory image,
			 * pass a file descriptor for the executable
			 * to the linker.
			 */
			if (error = execopen(&vp, &fd))
				goto bad;

			*aux++ = AT_EXECFD;
			*aux++ = fd;
			postfixsize += (2 * NBPW);
		}

		/*
		 * Verify access permissions of the dynamic linker.
		 */
		if ((error = execpermissions(nvp, &vattr, &dehdr, args)) != 0) {
			VN_RELE(nvp);
			goto bad;
		}

		/*
		 * Get linker's ELF headers.
		 */
		if ((error = getelfhead(&ehdrp, &phdrbase,
				&phdrsize, &dehdr)) != 0){
			exhd_release(&dehdr);
			VN_RELE(nvp);
			goto bad;
		}

		/*
		 * Map in the linker.
		 */
		error = mapelfexec(nvp, ehdrp, phdrbase, &junk, &junk,
				&junk, &base, &voffset, execsz, dehdr);
		exhd_release(&dehdr);
		VN_RELE(nvp);
		if (error)
			goto bad;

		if (junk != NULL)
			goto bad;

		*aux++ = AT_BASE;	/* base address for dynamic linker */
		*aux++ = voffset & ~(ELF_386_MAXPGSZ - 1);
		*aux++ = AT_FLAGS;	/* processor flags */
		*aux++ = EF_I386_NONE;
		*aux++ = AT_PAGESZ;	/* page size */
		*aux++ = PAGESIZE;
		*aux++ = AT_NULL;
		*aux = 0;
		postfixsize += (8 * NBPW);
		ASSERT(postfixsize == args->auxsize);
		
	}

	/*
	 * Check if current memory limit exceeded.
	 */
	if (*execsz > btoc(u.u_rlimit[RLIMIT_VMEM].rlim_cur)) {
		error = ENOMEM;
		goto bad;
	}

	/*
	 * Set up the users stack.
	 */
	if (postfixsize) {
		error = execpoststack(args, elfargs, postfixsize);
		if (error)
			goto bad;
	}
		
	/* 
	 * XXX -- should get rid of this stuff,
	 * exdata is object file dependent.
	 */
	u.u_exdata.ux_mag = 0413;
	u.u_exdata.ux_entloc  = (caddr_t)(ehdrp->e_entry + voffset);

	/*
	 *	Unfortunate: Xenix support.
	 */

	u.u_renv = XE_V5|XE_EXEC|U_IS386|U_ISELF;

	if (!uphdr){
		exenv.ex_brkbase = base;
		exenv.ex_magic = elfmagic;
		exenv.ex_vp = vp;
		setexecenv(&exenv);
	}

	return 0;

bad:
	if (fd != -1)		/* did we open the a.out yet */
		(void)execclose(fd);

	psignal(pp,SIGKILL);

	if (error == 0)
		error = ENOEXEC;

	return error;
		
}
		

/*
 * Get the ELF file header, and the ELF program header table.
 */
int
getelfhead(ehdrp, phdrbase, phdrsize, ehdp)
Elf32_Ehdr **ehdrp;
register caddr_t *phdrbase;
long *phdrsize;
exhda_t *ehdp;
{
	register Elf32_Ehdr *ehdr;
	int resid, error=0;

	/*
	 * Read in ELF header.
	 */
	error = exhd_getmap(ehdp, (off_t) 0, sizeof(Elf32_Ehdr),
			EXHD_4BALIGN|EXHD_KEEPMAP,
			(caddr_t)ehdrp);
	if (error)
		goto bad;
	ehdr = *ehdrp;

	/*
   	 * We got here by the first two bytes in ident.
	 */
	if (ehdr->e_ident[EI_MAG2] != ELFMAG2 
	     || ehdr->e_ident[EI_MAG3] != ELFMAG3
	     || ehdr->e_ident[EI_CLASS] != ELFCLASS32
	     || ehdr->e_ident[EI_DATA] != ELFDATA2LSB
	     || ehdr->e_machine != EM_386
	     || (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) 
	     || ehdr->e_phentsize == 0)
			goto bad;

	/*
	 * Size of the program header table.
	 */
	*phdrsize = ehdr->e_phnum * ehdr->e_phentsize;

	/*
	 * Read in program header table.
	 */
	error = exhd_getmap(ehdp, (off_t)ehdr->e_phoff, *phdrsize,
			EXHD_4BALIGN|EXHD_KEEPMAP,
			(caddr_t) phdrbase);
	if (error)
		goto bad;

	return 0;

bad:
	if (error == 0)
		error = ENOEXEC;

	return error;
}

/*
 * Map in the ELF executable.
 */
STATIC int
mapelfexec(vp, ehdr, phdrbase, uphdr, dyphdr, stphdr, base, voffset, execsz, ehdp)
struct vnode *vp;
Elf32_Ehdr *ehdr;
caddr_t phdrbase;
Elf32_Phdr **uphdr, **dyphdr, **stphdr;
caddr_t *base;
long *voffset;
long *execsz;
/* Enhanced Application Compatibility Support */
exhda_t *ehdp;
/* End Enhanced Application Compatibility Support */
{
	struct proc *pp = u.u_procp;
	Elf32_Phdr *phdr;
	int i, prot, error;
	caddr_t addr;
	size_t zfodsz;
	int ptload = 0;
	caddr_t data_base = (caddr_t) 0;

/* Enhanced Application Compatibility Support */
	char* note_contents;
	size_t note_len;
/* End Enhanced Application Compatibility Support */

	if (ehdr->e_type == ET_DYN) {
		caddr_t end, segend;

		/*
		 * Loop through program header table, computing the size of
		 * the executable
		 */
		end = 0;
		for (i = 0 ; i < (int)ehdr->e_phnum ; ++i) {
			phdr = (Elf32_Phdr *)(phdrbase +
				(ehdr->e_phentsize *i));
			if (phdr->p_type == PT_LOAD) {
				segend = (caddr_t)phdr->p_vaddr + phdr->p_memsz;
				if (segend > end)
					end = segend;
			}
		}
		map_addr(voffset, ctob(btoc(end)), (off_t)0, 1);
		if (*voffset == 0) {
			error = ENOMEM;
			goto bad;
		}
	} else
		*voffset = 0;

	/*
	 * Loop through the program header table.
	 */
	for ( i=0; i < (int)ehdr->e_phnum; i++){
		phdr = (Elf32_Phdr *)(phdrbase + (ehdr->e_phentsize *i));

		switch(phdr->p_type){
		case PT_LOAD:	/* loadable segment */
			if ((*dyphdr != NULL) && (*uphdr == NULL))
				return 0;

			ptload = 1;

			/*
			 * Determine segment protections.
			 */
			prot = PROT_USER;
			if (phdr->p_flags & PF_R)
				prot |= PROT_READ;
			if (phdr->p_flags & PF_W)
				prot |= PROT_WRITE;
			if (phdr->p_flags & PF_X)
				prot |= PROT_EXEC;

			addr = (caddr_t) phdr->p_vaddr + *voffset;
			zfodsz = (size_t) phdr->p_memsz - phdr->p_filesz;

			/*
			 * Map in segment.
			 */
			if (error = execmap(vp, addr, phdr->p_filesz, 
					     zfodsz, phdr->p_offset, prot))
					goto bad;

			if (phdr->p_flags & PF_W) {
				if (addr > *base)
					*base = addr + phdr->p_memsz;

				/* new virtual map support */
				if (data_base == (caddr_t) 0 ||
					addr < data_base)
					u.u_exdata.ux_datorg = data_base = addr;
			}

			*execsz += btoc(phdr->p_memsz);
			break;

		case PT_INTERP:	/* path name to interpreter (dynamic linker) */
			if (ptload)
				goto bad;
			*dyphdr = phdr;
			break;

		case PT_SHLIB:	/* shared library info */
			*stphdr = phdr;
			break;

		case PT_PHDR:	/* memory image of program header table */
			if (ptload)
				goto bad;
			*uphdr = phdr;
			break;

		case PT_NOTE:	
/* Enhanced Application Compatibility Support */
			/*
			** Check for a converted OMF executable
			*/

			/*
			** Map note section only if it looks like what we're
			** looking for (ie. has at least the same size).
			*/
			if (phdr->p_filesz >= strlen(SCO_OMF_NOTE)) {
				error = exhd_getmap(ehdp,
						    phdr->p_offset,
						    phdr->p_filesz,
						    EXHD_NOALIGN,
						    (caddr_t)&note_contents);
				if (error)
					goto bad;

				/*
				** Since .note sections are coalesced, look for
				** an occurrence of SCO_OMF_NOTE anywhere in
				** the section.  UB_XOUT and UB_PRE_SV are
				** defined in "sys/user.h" and affect the
				** behavior of a number of system calls if set.
				*/
				if (strnstr(note_contents, SCO_OMF_NOTE,
					    phdr->p_filesz) != NULL)
					u.u_renv |= UB_XOUT|UB_PRE_SV;
			}
/* End Enhanced Application Compatibility Support */

		case PT_NULL:
		case PT_DYNAMIC:
			break;

		default:
			break;
		}
	}
	return 0;
bad:
	if (error == 0)
		error = ENOEXEC;
	return error;
}

/* Enhanced Application Compatibility Support */
/*
 * Copy of strstr(3) with the addition of a length check.
 * This should go somewhere more general.
 * Convert to assembler when time permits and move it
 * to util/strings.s
 */
STATIC char *
strnstr(as1, as2, n)
	char *as1;
	char *as2;
	size_t n;
{
	register const char *s1,*s2;
        register char c;
        const char *tptr;
	size_t ti;
	register size_t i = 0;

        s1 = as1;
        s2 = as2;

	if (s1 == NULL)
	   	return(NULL);

        if (s2 == NULL || *s2 == '\0')
                return((char *)s1);
        c = *s2;

        while (i++ < n && *s1)
                if (*s1++ == c) {
                        tptr = s1;
			ti = i;
                        while ((c = *++s2) && i++ < n && c == *s1++) ;
                        if (c == 0)
                                return((char *)tptr - 1);
			if (i == n + 1)
			   	return(NULL);
                        s1 = tptr;
                        s2 = as2;
			i = ti;
                        c = *s2;
                }
	return(NULL);
}
/* End Enhanced Application Compatibility Support */

/*
 * Load shared libraries.
 */
STATIC int
elf_coffshlib(vp, stphdr, execsz, ehdp)
struct vnode *vp;
Elf32_Phdr *stphdr;
long *execsz;
exhda_t *ehdp;
{
	int i, error;
	int dataprot = PROT_ALL;
	int textprot = PROT_ALL & ~PROT_WRITE;
	struct exdata edp, *shlb_dat, *datp;
	u_int shlb_datsz;

	edp.ux_lsize = stphdr->p_filesz;	/* size of lib section */
	edp.ux_loffset = stphdr->p_offset;	/* file offset to lib section */

	/* enough space for max number of libraries */
	shlb_datsz = shlbinfo.shlbs * sizeof(struct exdata);
	
	/*
	 * Allocate array to hold info for each library.
	 */
	shlb_dat = (struct exdata *)kmem_alloc(shlb_datsz, KM_SLEEP);

	/*
	 * Get header and vnode info for each library.
	 */
	if ((error = getcoffshlibs(vp, &edp, shlb_dat, execsz, ehdp)) != 0)
			goto done;

	datp = shlb_dat;

	/*
	 * For each library, map in text and data segments.
	 */
	for (i=0 ; i < edp.ux_nshlibs; i++, datp++){
		if (error = execmap(datp->vp, datp->ux_txtorg,
			    datp->ux_tsize, (off_t)0, datp->ux_toffset,
					textprot)){
			coffexec_err(++datp, edp.ux_nshlibs - i - 1);
			goto done;
		}

		if (error = execmap(datp->vp, datp->ux_datorg,
			   datp->ux_dsize, (off_t)datp->ux_bsize, 
			    datp->ux_doffset, dataprot)){
			coffexec_err(++datp, edp.ux_nshlibs - i - 1);
			goto done;
		}
		VN_RELE(datp->vp);	/* done with this reference */
	}
		
done:
	kmem_free(shlb_dat, shlb_datsz);

	return error;
}

	

#define WR(vp, base, count, offset, rlimit, credp) \
	vn_rdwr(UIO_WRITE, vp, (caddr_t)base, count, offset, UIO_SYSSPACE, \
	0, rlimit, credp, (int *)NULL)

typedef struct {
	Elf32_Word namesz;
	Elf32_Word descsz;
	Elf32_Word type;
	char name[8];
} Elf32_Note;

#define NT_PRSTATUS	1
#define NT_PRFPREG	2
#define NT_PRPSINFO	3

/*
 * Generate note for ELF core files.
 */
STATIC int
elfnote(vp, offsetp, type, descsz, desc, rlimit, credp)
vnode_t *vp;
off_t *offsetp;
int descsz, type;
caddr_t desc;
struct cred *credp;
rlim_t rlimit;
{
	Elf32_Note note;
	int error;

	bzero((caddr_t)&note, sizeof(note));
	bcopy("CORE", note.name, 4);
	note.type = type;
	note.namesz = 8;
	note.descsz = roundup(descsz, sizeof(Elf32_Word));
	if (error = WR(vp, &note, sizeof(note), *offsetp, rlimit, credp))
		return error;
	*offsetp += sizeof(note);
	if (error = WR(vp, desc, note.descsz, *offsetp, rlimit, credp))
		return error;
	*offsetp += note.descsz;
	return 0;
}

/*
 * ELF specific core routine, called through
 * the execsw table.
 */
elfcore(vp, pp, credp, rlimit, sig)
	vnode_t *vp;
	proc_t *pp;
	struct cred *credp;
	rlim_t rlimit;
	int sig;
{
	Elf32_Ehdr ehdr;
	Elf32_Phdr *v;
	u_long hdrsz;
	off_t offset, poffset;
	int error, i, nhdrs;
	struct seg *seg;
	prstatus_t prstat;
	fpregset_t fpregs;
	prpsinfo_t psinfo;

	nhdrs = (prnsegs(pp) + 1);
	hdrsz = nhdrs * sizeof(Elf32_Phdr);

	v = (Elf32_Phdr *)kmem_zalloc(hdrsz, KM_SLEEP);

	/*
	 * Set up ELF header.
	 */
	bzero((caddr_t)&ehdr, sizeof(Elf32_Ehdr));
	ehdr.e_ident[EI_MAG0] = ELFMAG0;
	ehdr.e_ident[EI_MAG1] = ELFMAG1;
	ehdr.e_ident[EI_MAG2] = ELFMAG2;
	ehdr.e_ident[EI_MAG3] = ELFMAG3;
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	ehdr.e_type = ET_CORE;
	ehdr.e_machine = EM_386;
	if (PTOU(pp)->u_fpvalid)
		ehdr.e_flags |= EF_I386_FP;
#ifdef WEITEK
	if (PTOU(pp)->u_weitek == WEITEK_HW)	/* if process used Weitek board */
		ehdr.e_flags |= EF_I386_WEITEK;
#endif

	ehdr.e_version = EV_CURRENT;
	ehdr.e_phoff = sizeof(Elf32_Ehdr);
	ehdr.e_ehsize = sizeof(Elf32_Ehdr);
	ehdr.e_phentsize = sizeof(Elf32_Phdr);
	ehdr.e_phnum = (unsigned short)nhdrs;

	/*
	 * Write ELF header.
	 */
	if (error = WR(vp, &ehdr, sizeof(Elf32_Ehdr), 0, rlimit, credp))
		goto done;

	/*
	 * Set up program header table.
	 */

	offset = sizeof(Elf32_Ehdr);
	poffset = sizeof(Elf32_Ehdr) + hdrsz;

	/*
	 * First program header is for ELF core notes.
	 */
	v[0].p_type = PT_NOTE;
	v[0].p_flags = PF_R;
	v[0].p_offset = poffset;
	v[0].p_filesz = (sizeof(Elf32_Note) * 2)
	  + roundup(sizeof(prstatus_t), sizeof(Elf32_Word))
	  + roundup(sizeof(prpsinfo_t), sizeof(Elf32_Word));
	if (PTOU(pp)->u_fpvalid
#ifdef WEITEK
		|| PTOU(pp)->u_weitek == WEITEK_HW
#endif
	)
		v[0].p_filesz += sizeof(Elf32_Note)
		+ roundup(sizeof(fpregset_t), sizeof(Elf32_Word));
	poffset += v[0].p_filesz;

	/*
	 * Generate program header for each segment.
	 */
	for (i = 1, seg = pp->p_as->a_segs; i < nhdrs; seg = seg->s_next) {
		caddr_t naddr;
		caddr_t saddr = seg->s_base;
		caddr_t eaddr = seg->s_base + seg->s_size;
		do {
			u_int prot, size;
			prot = as_getprot(pp->p_as, saddr, &naddr);
			size = naddr - saddr;
			v[i].p_type = PT_LOAD;
			v[i].p_vaddr = (Elf32_Word)saddr;
			v[i].p_memsz = size;
			if (prot & PROT_WRITE)
				v[i].p_flags |= PF_W;
			if (prot & PROT_READ)
				v[i].p_flags |= PF_R;
			if (prot & PROT_EXEC)
				v[i].p_flags |= PF_X;
			if ((prot & (PROT_WRITE|PROT_EXEC)) != PROT_EXEC) {
				v[i].p_offset = poffset;
				v[i].p_filesz = size;
				poffset += size;
			}
			saddr = naddr;
			i++;
		} while (naddr < eaddr);
	}

	/*
	 * Write program header table.
	 */
	error = WR(vp, v, hdrsz, offset, rlimit, credp);
	if (error)
		goto done;

	offset += hdrsz;

	/*
	 * Get process status.
	 */
	prgetstatus(pp, &prstat);

	/* LINTED */
	prstat.pr_cursig = sig;

	/*
	 * Process status saved in note section of core file.
	 */
	error = elfnote(vp, &offset, NT_PRSTATUS, sizeof(prstat), 
	  (caddr_t)&prstat, rlimit, credp);
	if (error)
		goto done;

	/*
	 * Floating point registers saved in note section of core file.
	 */
	if (PTOU(pp)->u_fpvalid
#ifdef WEITEK
		|| PTOU(pp)->u_weitek == WEITEK_HW
#endif
      	) {
		prgetfpregs(pp, &fpregs);
		error = elfnote(vp, &offset, NT_PRFPREG, sizeof(fpregs), 
		  (caddr_t)&fpregs, rlimit, credp);
		if (error)
			goto done;
	}

	/*
	 * PS information saved in note section of core file.
	 */
	prgetpsinfo(pp, &psinfo);
	error = elfnote(vp, &offset, NT_PRPSINFO, sizeof(psinfo), 
	  (caddr_t)&psinfo, rlimit, credp);

	/*
	 * Write each core segment to file.
	 */
	for (i = 1; !error && i < nhdrs; i++) {
		if (v[i].p_filesz == 0)
			continue;
		error = core_seg(pp, vp, v[i].p_offset, (caddr_t)v[i].p_vaddr, 
		  v[i].p_filesz, rlimit, credp);
	}

done:
	kmem_free(v, hdrsz);
	return error;
}
