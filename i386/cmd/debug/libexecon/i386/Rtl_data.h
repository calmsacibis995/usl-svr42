/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Rtl_data_h
#define Rtl_data_h
#ident	"@(#)debugger:libexecon/i386/Rtl_data.h	1.2"

// Machine dependent description of dynamic linking access
// routines

#include "Iaddr.h"
#include <link.h>

class	Process;
class	Procctl;
class	Seglist;

class	Rtl_data {
	Iaddr		r_debug_addr;
	Iaddr		rtld_base( Process *, Proclive * );
	const char	*ld_so;
public:
	Iaddr		rtld_addr;
	r_debug		rdebug;
	int		find_r_debug( Process *, Proclive * );
	int		find_link_map( int, const char *, Seglist *, 
				Iaddr & , Process *);
			Rtl_data(const char *);
			~Rtl_data() { delete (void *)ld_so; }
};

#endif
