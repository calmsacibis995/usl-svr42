/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Coffbuild.h	1.1"

#ifndef Coffbuild_h
#define Coffbuild_h

// Interface to the COFF symbol tables - converts COFF
// symbol table entries and line number entries into
// the internal format
//
// Uses a file descriptor for access to the COFF file,
// reads the line number entries, symbol table, and
// string table in each as one contigous chunk.
// Deals with symbol table entries one at a time
// through make_record and get_syminfo, handles line
// number info a file at a time through line_info

#include	"Build.h"
#include	"Coff.h"
#include	"Protorec.h"
#include	"Itype.h"
#include	"Reflist.h"
#include	"Protoline.h"

struct Syminfo;

class Coffbuild : public Build {
	Coff		*coff;		// low level routines read the file
	Protorec	protorec;
	Protorec	prototype;
	Protoline	protoline;
	Reflist		reflist;
	long		nextofs;	// offset of symbol past current one
	long		linedisp;	// file offset of line number entries
	long		symtab_offset;	// beginning of the symbol table
	long		global_offset;	// beginning of block of globals at the
					// end of the symbol table
	int		textsectno;
	struct syment	sym;
	union auxent	aux;

	void		find_arcs( long & sibofs, long & childofs );
	void		get_arcs();
	void		get_data();
	void		get_type_C();
	void		get_type();
	void		get_addr_C();
	void		get_addr();
	int		find_record( long offset, int want_file );
	void		get_lineinfo( long loffset, long lncnt, long foffset );
	const char *	get_fcn_lineinfo( const char * start, const char * end );
	int		has_line_info( long offset ); // does this file
						// have line information?
public:
			Coffbuild( Coff * );
			~Coffbuild() {}

	Attribute *	make_record( long offset, int want_file = 0 );
	int		get_syminfo( long offset, Syminfo & syminfo );
	long		first_symbol();
	long		globals_offset();
	Lineinfo *	line_info( long offset );
	void		get_pc_info( long offset, Iaddr & lopc, Iaddr & hipc );
};

extern int	 coff_bit_loc(int, int, int, int &, int &);

#endif /* Coffbuild_h */
