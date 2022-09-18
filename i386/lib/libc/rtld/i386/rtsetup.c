/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/rtsetup.c	1.26"

/*
 * 386 specific setup routine - relocate ld.so's symbols, setup its
 * environment, map in loadable sections of a.out; 
 *
 * takes base address ld.so was loaded at, address of ld.so's dynamic structure,
 * and a pointer to the argument list starting at argc.  This list is
 * organized as follows: argc, argv[0], argv[1], ..., 0, envp[0], envp[1],
 * ..., 0, auxv[0], auxv[1], ..., 0 if errors occur, send process SIGKILL -
 * otherwise return a.out's entry point to the bootstrap routine 
 */

#include <signal.h>
#ifdef __STDC__
#include <stdlib.h>
#else
extern int      atexit();
#endif
#include <fcntl.h>
#include "rtinc.h"
#include <sys/auxv.h>
#include <sys/elf_386.h>
#include <unistd.h>

static struct rt_private_map ld_map;  /* link map for ld.so */

unsigned long   _rt_setup(ld_base, ld_dyn, args_p)
    unsigned long   ld_base;
    Elf32_Dyn      *ld_dyn;
    char          **args_p;
{
    Elf32_Dyn      *dyn[DT_MAXPOSTAGS], *ltmp, *ld;
    Elf32_Dyn       interface[2];
    register unsigned long off, reladdr, rend;
    CONST char     *envdirs = 0, *rundirs = 0;
    int             i, bmode = RTLD_LAZY;
    char	  **envp;
    struct rt_private_map *lm;
	struct rt_private_map *save_lm;
    char           *p;
    int             argc;
    int             fd = -1;
    auxv_t         *auxv;
    char	   *pname;
    int phsize;
    int phnum;
    unsigned long entry;
    int flags;
    int pagsz;
    Elf32_Phdr *phdr;

    extern void     _rt_errout();

    /* traverse argument list and get values of interest */
    argc = *((int *) args_p);
    args_p++;
    pname = *args_p;		/* set local to process name for error
				 * messages */
    args_p += argc;		/* skip argv[0] ... argv[n] */
    args_p++;			/* and 0 at end of list */
    envp = args_p;		/* get the environment pointer */
    while (*args_p)
	args_p++;		/* skip envp[0] ... envp[n] */
    args_p++;			/* and 0 at end of list */

    auxv = (auxv_t *) args_p;
	/* search the aux. vector for the information passed by exec */
	for (; auxv->a_type != AT_NULL; auxv++) {
		switch(auxv->a_type) {

		case AT_EXECFD:
			/* this is the old exec that passes a file descriptor */
			fd = auxv->a_un.a_val;
			break;

		case AT_FLAGS:
			/* processor flags (MAU available, etc) */
			flags = auxv->a_un.a_val;
			break;

		case AT_PAGESZ:
			/* system page size */
			pagsz = auxv->a_un.a_val;
			break;

		case AT_PHDR:
			/* address of the segment table */
			phdr = (Elf32_Phdr *) auxv->a_un.a_ptr;
			break;

		case AT_PHENT:
			/* size of each segment header */
			phsize = auxv->a_un.a_val;
			break;

		case AT_PHNUM:
			/* number of program headers */
			phnum = auxv->a_un.a_val;
			break;

		case AT_BASE:
			/* ld.so base address */
			ld_base = auxv->a_un.a_val;
			break;

		case AT_ENTRY:
			/* entry point for the a.out */
			entry = auxv->a_un.a_val;
			break;

		}
	}

	/* store pointers to each item in ld.so's dynamic structure 
	 * dyn[tag] points to the dynamic section entry with d_tag
	 * == tag
	 */
	for (i = 0; i < DT_MAXPOSTAGS; i++)
		dyn[i] = (Elf32_Dyn *)0;
	ld_dyn = (Elf32_Dyn *)((char *)ld_dyn + ld_base);
	for (ld = ld_dyn; ld->d_tag != DT_NULL; ld++)
		dyn[ld->d_tag] = ld;
	
	/* relocate all symbols in ld.so */
	reladdr = ((unsigned long)(dyn[DT_REL]->d_un.d_ptr) + ld_base);

	rend = reladdr + dyn[DT_RELSZ]->d_un.d_val;
    for (; reladdr < rend; reladdr += sizeof(Elf32_Rel)) {
	off = (unsigned long) ((Elf32_Rel *) reladdr)->r_offset + ld_base;
	/*
	 * insert value calculated at reference point we should only have 1
	 * We only deal with R_386_RELATIVE relocations here.
	 * These should be sufficient for rtld's own purposes, because
	 * of the way we were linked (ld -r -Bsymbolic).  Other relocations
	 * are handled if rtld is loaded as a library.
	 * R_386_RELATIVE - we simply add the base address 
	 */
	if (ELF32_R_TYPE(((Elf32_Rel *) reladdr)->r_info) != R_386_RELATIVE)
	    continue;

	*((unsigned long *) off) += ld_base;	/*
						 * THIS CODE WILL BREAK ON
						 * MACHINES
						 * THAT REQUIRE DATA TO ALIGNED
						 */
    }
	/* set global to process name for error messages */
	_proc_name = pname;

	/* look for environment strings */
	envdirs = _readenv( (CONST char **)envp, &bmode );

	DPRINTF(LIST,(2, "rtld: _rt_setup(0x%x, 0x%x, %s, 0x%x, 0x%x)\n",ld_base,(unsigned long)ld_dyn,pname,(unsigned long)envp, auxv));

#if 0
	/* open /dev/zero to use for mapping anonymous memory */
	if ((_devzero_fd = _open(DEV_ZERO, O_RDONLY)) == -1) {
		_rtfprintf(2, "%s: %s: can't open %s\n",(char*) _rt_name,_proc_name,(CONST char *)DEV_ZERO);
		(void)_kill(_getpid(), SIGKILL);
	}
#endif

	/* map in the file, if exec has not already done so.
	 * If it has, just create a new link map structure for the a.out
	 */
	if (fd != -1) {
		/* this is the old exec that doesn't pass as much
		 * information on the stack, so we have to go
		 * through system calls to get it
		 */

		/* set system page size */
		_syspagsz = _sysconf(_SC_PAGESIZE);

		/* set up space for allocator */
		_rt_initAllocator();

		if ((_ld_loaded = _map_so(fd, 0)) == 0) {
			_rtfprintf(2, "%s\n",_dlerror());
			(void)_kill(_getpid(), SIGKILL);
		}
	}
	else {
		Elf32_Phdr *pptr;
		Elf32_Phdr *firstptr = 0;
		Elf32_Phdr *lastptr;
		Elf32_Dyn *mld;

		_flags = flags;
		_syspagsz = pagsz;
			
		/* set up space for allocator */
		_rt_initAllocator();

		/* extract the needed information from the segment headers */
		for (i = 0, pptr = phdr; i < phnum; i++) {
			if (pptr->p_type == PT_LOAD) {
				if (!firstptr)
					firstptr = pptr;
				lastptr = pptr;
			}
			else if (pptr->p_type == PT_DYNAMIC)
				mld = (Elf32_Dyn *)(pptr->p_vaddr);
			pptr = (Elf32_Phdr *)((unsigned long)pptr + phsize);
		}
		if ((_ld_loaded = _new_lm(0, mld, firstptr->p_vaddr,
			(lastptr->p_vaddr + lastptr->p_memsz) - firstptr->p_vaddr,
			entry, phdr, phnum, phsize)) == 0) {
				_rtfprintf(2, "%s\n",_dlerror());
				(void)_kill(_getpid(), SIGKILL);
		}
		if (TEXTREL(_ld_loaded))
			if (_set_protect( _ld_loaded, PROT_WRITE ) == 0) {
				_rtfprintf(2, "%s\n",_dlerror());
				(void)_kill(_getpid(), SIGKILL);
			}
	}

	/* _ld_loaded and _ld_tail point to head and tail of rt_private_map list
	 */
	_ld_tail = _ld_loaded;

	/* initialize debugger information structure 
	 * some parts of this structure were initialized
	 * statically
	 */
	_r_debug.r_map = (struct link_map *)_ld_loaded;
	_r_debug.r_ldbase = ld_base;

	/* create a rt_private_map structure for ld.so */
	_rtld_map = &ld_map;
	DYN(_rtld_map) = ld_dyn;
	ADDR(_rtld_map) = ld_base;
	for (ld = ld_dyn ; ld->d_tag != DT_NULL; ++ld ) {
		switch (ld->d_tag) {
		case DT_SYMTAB:
			SYMTAB(_rtld_map) = (char *)ld->d_un.d_ptr + ld_base;
			break;

		case DT_STRTAB:
			STRTAB(_rtld_map) = (char *)ld->d_un.d_ptr + ld_base;
			break;

		case DT_SYMENT:
			SYMENT(_rtld_map) = ld->d_un.d_val;
			break;

		case DT_REL:
			REL(_rtld_map) = (char *)ld->d_un.d_ptr + ld_base;
			break;

		case DT_RELSZ:
			RELSZ(_rtld_map) = ld->d_un.d_val;
			break;

		case DT_RELENT:
			RELENT(_rtld_map) = ld->d_un.d_val;
			break;

		case DT_HASH:
			HASH(_rtld_map) = (unsigned long *)(ld->d_un.d_ptr + ld_base);
			break;

		case DT_PLTGOT:
			PLTGOT(_rtld_map) = (unsigned long *)(ld->d_un.d_ptr + ld_base);
			break;

		case DT_PLTRELSZ:
			PLTRELSZ(_rtld_map) = ld->d_un.d_val;
			break;

		case DT_JMPREL:
			JMPREL(_rtld_map) = (char *)(ld->d_un.d_ptr) + ld_base;
			break;

		case DT_INIT:
			INIT(_rtld_map) = (void (*)())((unsigned long)ld->d_un.d_ptr + ld_base);
			break;

		case DT_FINI:
			FINI(_rtld_map) = (void (*)())((unsigned long)ld->d_un.d_ptr + ld_base);
			break;

		case DT_SYMBOLIC:
			SYMBOLIC(_rtld_map) = 1;
			break;

		default:	
			break;
		}
	}

	/* we copy the name here rather than just setting a pointer
	 * to it so that it will appear in the data segment and
	 * thus in any core file
	 */
	p = (char *)STRTAB(_rtld_map) + dyn[DT_SONAME]->d_un.d_val;
	if ((NAME(_rtld_map) = _rtmalloc(_rtstrlen(p) + 1)) == 0) {
		_rtfprintf(2, "%s\n",_dlerror());
		(void)_kill(_getpid(), SIGKILL);
	}
	(void)_rtstrcpy(NAME(_rtld_map), p);
	SYMENT(_rtld_map) = dyn[DT_SYMENT]->d_un.d_val;
	PERMIT(_rtld_map) = 1;
	NODELETE(_rtld_map) = 1;


	/* are any directories specified in the a.out's dynamic? */
	rundirs = RPATH(_ld_loaded);

	/* set up directory search path */
	if (!_rt_setpath(envdirs, rundirs)) {
		_rtfprintf(2, "%s\n",_dlerror());
		(void)_kill(_getpid(), SIGKILL);
	}

	/* setup for call to _rtld */
	interface[0].d_tag = DT_MODE;
	interface[0].d_un.d_val = bmode;
	interface[1].d_tag = DT_NULL;

	if (_rtld(interface, &ltmp, 1)) {
		_rtfprintf(2, "%s\n",_dlerror());
		(void)_kill(_getpid(), SIGKILL);
	}

	save_lm = 0;
	/* set refpermit bit of all objects mapped */
	for (lm = _ld_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		PERMIT(lm) = 1;
		if (NEXT(lm) == (struct link_map *)_rtld_map)
			save_lm = lm;
	}
                        
	/* unlink the link map for rtld, so it isn't searched unnecessaily
	 * At this point, _rtld_map should be on the end of the list
	 * (see _rtld())
	 */
	if (save_lm)
		NEXT(save_lm) = 0;

	/*set up errno */
	_rt_setaddr();
#if 0
	/* copy data for __iob and __ctype into user's code if used */
	copy_ctype();
#endif

	/* reconnect the list */
	if (save_lm)
		NEXT(save_lm) = (struct link_map *)_rtld_map;

	return(ENTRY(_ld_loaded));
}

/*
 * verify machine specific flags in ELF header - if the flags indicate an
 * error condition, return 1; else return 0 
 */
int             _flag_error(eflags, pathname)
    unsigned long   eflags;
    CONST char     *pathname;
{
	return 0;					/* On the i386 we do nothing */
}

void copy_ctype ARGS((void));
/* 386 specific routine to aovid problems with COPY relocations and weak symbols
 * __iob and __ctype are special cases - they are both big enough that having
 * a copy of the data in shared library and in every a.out drags down system
 * performance on the 386.  We would prefer to just use copy relocations, but
 * copy relocations don't work very well with weak symbols.  You could
 * have an old .o that references _iob and a new one that references __iob,
 * and with copy relocations the a.out would end up with two seperate
 * objects, since there is no way to connect the two references in the a.out.
 * Instead, there are fake definitions of these symbols in the library
 * that reserve space in the .bss section in the a.out, and this function
 * provides special case copy relocation handling.
 */

#define SPACEinINTS 160
static int _rtAllocatorSp[SPACEinINTS];

static int _rt_initAllocator()
	/* get initial allocator space from
	** the data segment
	*/
{
 	_rtmkspace((char *)_rtAllocatorSp, SPACEinINTS*sizeof(int));
	return 0;
}

