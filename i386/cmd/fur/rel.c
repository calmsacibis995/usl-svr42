/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fur:i386/cmd/fur/rel.c	1.1"

#include <libelf.h>
#include <sys/elf_386.h>
#include "fur.h"

#ifndef NULL
#define NULL	0
#endif

/* NOTE: These macros will work reliably only on 32-bit 2's 
 * complement machines.  The type of P in all cases should
 * be unsigned char *
 */

#define	GET4(P)	((long)(((unsigned long)(P)[3] << 24) | \
			((unsigned long)(P)[2] << 16) | \
			((unsigned long)(P)[1] << 8) | \
			(unsigned long)(P)[0]))

#define	PUT4(V, P)	{ (P)[3] = ((V) >> 24); \
			  (P)[2] = ((V) >> 16); \
			  (P)[1] = ((V) >> 8); \
			  (P)[0] = (V); }



extern int debugflg;
static int
#ifdef __STDC__
comprel(Elf32_Rel *a, Elf32_Rel *b)
#else
comprel(a,b)
Elf32_Rel *a, *b;
#endif
{
	if(a->r_offset > b->r_offset)
		return(1);
	if(a->r_offset < b->r_offset)
		return(-1);
	return(0);
}
	

void
#ifdef __STDC__
updaterels(Elf_Data *rel_data, Elf_Data *sym_data, int text_ndx, int rtype)
#else
updaterels(rel_data, sym_data, text_ndx, rtype)
Elf_Data *rel_data, *sym_data;
int text_ndx;
int rtype;
#endif
{
	Elf32_Rel *trel, *endreloc;
	long delta;
	
	if(rtype != SHT_REL)
		error("SHT_RELA section found\n");

	endreloc = (Elf32_Rel *) ((char *) rel_data->d_buf + rel_data->d_size);
	trel = (Elf32_Rel *) rel_data->d_buf;
	while(trel < (Elf32_Rel *) endreloc) {
		delta = symchg(trel->r_offset, 
				    text_ndx,
			   	    sym_data->d_buf, 
				    (Elf32_Sym *) ((char *) sym_data->d_buf
						     +sym_data->d_size));
		if(debugflg)
			printf("relocation offset old=%x\t",trel->r_offset);
		trel->r_offset += delta;
		if(debugflg)
			printf("new=%x\n",trel->r_offset);
		trel++;
	}
	qsort(rel_data->d_buf,
	      (Elf32_Rel *) endreloc- (Elf32_Rel *) rel_data->d_buf,
	      sizeof(Elf32_Rel),comprel);
}


void
#ifdef __STDC__
chktextrels(Elf_Data *rel_data, Elf_Data *sym_data, int sect_ndx, 
		int text_ndx, int textsym_ndx, int rtype)
#else
chktextrels(rel_data, sym_data, sect_ndx, text_ndx, textsym_ndx, rtype)
Elf_Data *rel_data, *sym_data;
int sect_ndx, textsym_ndx, text_ndx, rtype;
#endif
{
	Elf32_Rel *trel, *endreloc;
	unsigned char *sect_data;
	unsigned char *offset;

	if(rtype != SHT_REL)
		error("SHT_RELA section found\n");

	endreloc = (Elf32_Rel *) ((char *) rel_data->d_buf + rel_data->d_size);
	for(trel = (Elf32_Rel *) rel_data->d_buf; trel < endreloc; trel++) {
		if(ELF32_R_SYM(trel->r_info) != textsym_ndx)
			continue;
		if(ELF32_R_TYPE(trel->r_info) == R_386_32) {
			long value;
			if(esections[sect_ndx].sec_data == NULL)  {
				esections[sect_ndx].sec_data =
					myelf_getdata(esections[sect_ndx].sec_scn,
							0, "section data");
				sect_data = (unsigned char *) 
					esections[sect_ndx].sec_data->d_buf;
			}
			offset = sect_data + trel->r_offset;
			value = GET4(offset);

			value += symchg(value,
				    text_ndx,
			   	    sym_data->d_buf, 
				    (Elf32_Sym *) ((char *) sym_data->d_buf
						     +sym_data->d_size));
			PUT4(value,offset);
			continue;
		}
		error("relocation against text section symbol in section %d\n",sect_ndx);
	}
}
