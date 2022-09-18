/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/weitek/weitek.cf/Stubs.c	1.3"
#ident	"$Header: $"

#include <sys/proc.h>
#include <sys/weitek.h>

/* Weitek stubs */

char weitek_kind = WEITEK_NO;
unsigned long weitek_cfg = 0L;
unsigned long weitek_paddr = 0L;
int weitek_pt = -1;
struct proc *weitek_proc = (struct proc *) 0;

void weitek_map() {}
void weitek_unmap() {}
void init_weitek_pt() {}
void map_weitek_pt() {}
void unmap_weitek_pt() {}
void weitek_save() { weitek_kind = WEITEK_NO; }
void weitek_restore(regs) long *regs; {}
void init_weitek() {}
void wemulate(vaddr) long vaddr; {}
void weitek_reset_intr() {}
void clear_weitek_ae() {}
void weitintr(i) int i; {}
int get87() { return(0); }
