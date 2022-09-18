/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Dwarfbuild.h	1.2"
#ifndef Dwarfbuild_h
#define Dwarfbuild_h

// Interface to DWARF debugging information.  Uses
// the ELF class for low-level access to the file itself.
//
// Dwarf records consist of tag-value pairs, with the tag
// specifying how to interpret the value.  Each record
// points to a sibling record.  Records between the current
// record and its sibling are children of the current record.
//
// Each dwarf entry consists of a 4-byte length, specifiying
// the length of the entire record; a 2-byte tag and an attribute
// list.  Each attribute may have 1 of 7 forms:
// 1) 4-byte address (relocated)
// 2) 4-byte reference to another dwarf entry
// 3) 2-byte datum
// 4) 4-byte datum
// 5) 2-byte length followed by length bytes
// 6) 4-byte length followed by length bytes
// 7) n bytes, 0-terminated
//
// Dwarf line number entries consist of tables of entries;
// each table refers to the entries for a given address range
// (usually a single dot-o).  Each table has a 4-byte length,
// followed by a 4-byte address, representing the lowest pc
// of the following entries.  This is followed by an array of
// entries, each with a 4-byte line number, a 2-byte intra-line
// position (currently unused) and a 4-byte virtual address.
// The last entry in each table has line number 0.
 

#include	"Attribute.h"
#include	"Build.h"
#include	"Protorec.h"
#include	"Reflist.h"
#include	"Protoline.h"
#include	"Iaddr.h"

struct	Syminfo;
class	ELF;

class Dwarfbuild : public Build {
	Protorec	protorec;
	Protorec	prototype;
	Protoline	protoline;
	Reflist		reflist;
	long		entry_offset, entry_end;
	Iaddr		entry_base;
	long		stmt_offset, stmt_end;
	Iaddr		stmt_base;
	char		*entry_data, *stmt_data;
	char *		ptr;
	long		length;
	long		entryoff;
	long		nextoff;
	short		tag;
	ELF	*	object;
	void		skip_attribute( short attrname );
	char		get_byte();
	short		get_2byte();
	long		get_4byte();
	Addrexp		make_chunk( void *p, int length);
	char *		get_string();
	void		next_item( Attribute * );
	int		subscr_list( Attribute * );
	void		get_location( Attr_form &, Attr_value & );
	void		get_ft( Attr_form &, Attr_value & );
	void		get_udt( Attr_form &, Attr_value & );
	void		get_mft( Attr_form &, Attr_value & );
	void		get_mudt( Attr_form &, Attr_value & );
	void		sibling();
	void		location();
	void		name();
	void		fund_type();
	void		mod_fund_type();
	void		user_def_type();
	void		mod_u_d_type();
	void		byte_size();
	void		bit_offset();
	void		bit_size();
	void		stmt_list();
	Iaddr		low_pc();
	void		high_pc();
	void		element_list( short attrname);
	void		subscr_data();
	void		language();
public:
			Dwarfbuild( ELF * );
			~Dwarfbuild() {};
	long		first_file() { return entry_offset; }
	long		last_entry() { return entry_end; }
	Attribute *	make_record( long offset, int ignored = 0 );
	Lineinfo *	line_info( long  offset);
	int		cache( long offset, long size );
	int		get_syminfo( long offset, Syminfo & );
	char *		get_name( long offset ) { return (char *)offset; };
	long		globals_offset();
};

#endif /* Dwarfbuild_h */
