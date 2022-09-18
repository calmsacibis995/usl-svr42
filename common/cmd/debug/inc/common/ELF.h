/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ELF_h
#define _ELF_h

#ident	"@(#)debugger:inc/common/ELF.h	1.1"

#include "Iaddr.h"
#include "Object.h"
#include "Machine.h"
#include <sys/types.h>
#include <sys/elf.h>


// maintain information about an ELF object
// and provide access to needed sections

class Seginfo;

class ELF : public Object {
	int		phdrnum;
	Elf_Phdr	*phdr;
	int		read_section(Sect_type, Elf_Shdr *);
	int		map_sections(Sect_type, Elf_Shdr *, Sect_type, 
				Elf_Shdr *);
public:
			ELF( int fdobj, dev_t, ino_t, time_t );
			~ELF() { delete phdr ;}
	Seginfo		*get_seginfo(int &count, int &isshared);
	int		find_symbol( const char *name, Iaddr &addr );
	int		get_phdr(int &num, Elf_Phdr *&);
};

#endif
