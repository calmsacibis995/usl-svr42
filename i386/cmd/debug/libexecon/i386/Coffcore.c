/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libexecon/i386/Coffcore.c	1.2"

#include "Machine.h"
#include "Msgtypes.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <elf.h>
#include <sys/signal.h>

#include <sys/user.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/reg.h>

/* Routine to handle Coff core files - we try to make them
 * look like ELF core files
 *
 * Written in C because of cfront problems in dealing with user.h
 *
 * Returns 0 on success, an error code suitable for printe
 * on failure
 */


int 
fake_ELF_core( int corefd, off_t size, Elf_Phdr **phdr, int *phdrnum,
	prstatus_t **pstat, fpregset_t **fregs, char **psinfo )
{
	user_t		u;
	int		*ar0;
	Elf_Phdr	*p;
	prstatus_t	 *pr;

	lseek(corefd, 0, SEEK_SET);
	if ( read(corefd, (char *)&u, sizeof(user_t) ) != sizeof(user_t) ) 
	{
		return (int)ERR_core_format;
	}

	if ((ctob(USIZE) + ctob(u.u_dsize) + ctob(u.u_ssize)) > size)
	{
		return (int)ERR_core_truncated;
	}

	*phdrnum = 3;		/*  DATA, STACK, UBLOCK */


	p = *phdr = (Elf_Phdr *) malloc(3 * sizeof(Elf_Phdr));
	if (!p)
		return(ERR_internal);

	p->p_type	= PT_LOAD;	/* DATA segment */
	p->p_offset	= ctob(USIZE);
	p->p_vaddr	= (Elf_Addr) u.u_exdata.ux_datorg;
	p->p_paddr	= 0;
	p->p_filesz	= ctob(u.u_dsize);
	p->p_memsz	= ctob(u.u_dsize);
	p->p_flags	= (PF_R|PF_W);
	p->p_align	= 0;

	++p;
	p->p_type	= PT_LOAD;	/* STACK segment */
	p->p_offset	= ctob(USIZE) + ctob(u.u_dsize);
	p->p_vaddr	= u.u_sub;
	p->p_paddr	= 0;
	p->p_filesz	= ctob(u.u_ssize);
	p->p_memsz	= ctob(u.u_ssize);
	p->p_flags	= (PF_R|PF_W);
	p->p_align	= 0;

	++p;
	p->p_type	= PT_LOAD;	/* UBLOCK */
	p->p_offset	= 0;
	p->p_vaddr	= UVUBLK;
	p->p_paddr	= 0;
	p->p_filesz	= ctob(USIZE);
	p->p_memsz	= ctob(USIZE);
	p->p_flags	= (PF_R);
	p->p_align	= 0;

	pr = *pstat = (prstatus_t *) malloc(sizeof(prstatus_t) );
	if (!pr)
		return(ERR_internal);

	memset( (char *)pr, 0, sizeof(prstatus_t) );

	ar0 = (int *)((long)&u + (long)u.u_ar0 - UVUBLK);

	pr->pr_reg[EAX] = ar0[EAX];
	pr->pr_reg[EBX] = ar0[EBX];
	pr->pr_reg[ECX] = ar0[ECX];
	pr->pr_reg[EDX] = ar0[EDX];
	pr->pr_reg[ESI] = ar0[ESI];
	pr->pr_reg[EDI] = ar0[EDI];
	pr->pr_reg[UESP] = ar0[UESP];
	pr->pr_reg[EBP] = ar0[EBP];
	pr->pr_reg[EIP] = ar0[EIP];
	pr->pr_reg[EFL] = ar0[EFL];
	pr->pr_reg[TRAPNO] = ar0[TRAPNO];

	*psinfo = (char *)malloc(PSARGSZ + 1);
	if (!*psinfo)
		return(ERR_internal);
	strncmp(*psinfo, u.u_psargs, PSARGSZ);
	(*psinfo)[PSARGSZ] = 0;
	if (u.u_fpvalid) 
	{
		*fregs = (fpregset_t *) malloc ( sizeof(fpregset_t) );
		if (!*fregs)
			return(ERR_internal);
		memcpy( (char*) *fregs, 
			  (char *)&(u.u_fps.u_fpstate),
			  sizeof(fpregset_t) );
	}
	else
		*fregs = 0;
	return 0;
}
