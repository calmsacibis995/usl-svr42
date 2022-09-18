/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libsymbol/common/Coffbuild.C	1.1"
#include	"Coffbuild.h"
#include	"Coff.h"
#include	"Object.h"
#include	"Locdesc.h"
#include	"Syminfo.h"
#include	"Fund_type.h"
#include	"Tag.h"
#include	"Cvtaddr.h"
#include	<string.h>

// maximum number of derived types in a COFF symbol
#define	DTYPES	6

Coffbuild::Coffbuild( Coff * obj )
{
	coff = obj;
	if (coff->file_format() != ff_coff)
		return;
	linedisp = coff->get_line_offset();
	textsectno = coff->sectno( ".text" );
	symtab_offset = coff->get_symtab_offset();
	global_offset = 0;
	nextofs = 0;
}

// find the file offsets of the sibling and child symbol table entries

void
Coffbuild::find_arcs( long & sibofs, long & childofs )
{
	char *	name;

	name = sym.n_nptr;

	// the normal case is no children, so the sibling is just the
	// next symbol
	childofs = 0;
	sibofs = nextofs;

	if (sym.n_sclass == C_FILE)
	{
		childofs = nextofs;
		sibofs = symtab_offset + sym.n_value*SYMESZ;
	}
	// an undefined function symbol, or one from a static shared library
	// (scnum < 0) has no local symbols
	else if ( (ISFCN(sym.n_type) && sym.n_scnum > N_UNDEF)
		|| sym.n_sclass == C_STRTAG
		|| sym.n_sclass == C_UNTAG
		|| sym.n_sclass == C_ENTAG
		|| ((sym.n_sclass == C_BLOCK) && strcmp(name,".bb") == 0 ))
	{
		sibofs = symtab_offset + aux.x_sym.x_fcnary.x_fcn.x_endndx*SYMESZ;

		// make sure it really has children before setting childofs
		if (sibofs != nextofs)
			childofs = nextofs;
	}
	else if ((sym.n_sclass == C_EOS)
		 || ((sym.n_sclass == C_FCN) && strcmp(name,".ef")==0)
		 || ((sym.n_sclass == C_BLOCK) && strcmp(name,".eb")==0))
	{
		sibofs = 0;
	}
}

// create attributes for child and sibling links

void
Coffbuild::get_arcs()
{
	long	sibofs, childofs;

	find_arcs( sibofs, childofs );
	if ( sym.n_sclass == C_FILE )
	{
		protorec.add_attr( an_sibling, af_cofffile, sibofs );
		protorec.add_attr( an_child, af_coffrecord, childofs );
		protorec.add_attr( an_scansize, af_int, sibofs-childofs );
	}
	else
	{
		protorec.add_attr( an_parent, af_coffrecord, 0L );
		if ( sibofs )
			protorec.add_attr( an_sibling, af_coffrecord, sibofs );
		if ( childofs )
			protorec.add_attr( an_child, af_coffrecord, childofs );
		if ( sibofs && childofs )
			protorec.add_attr( an_scansize, af_int, sibofs-childofs );
	}
}

// create attribute records for the current symbol

void
Coffbuild::get_data()
{
	long		offset;
	struct syment	sym2;
	union auxent	aux2;
	Iaddr		lopc,hipc;
	long		sibofs,childofs;

	switch (sym.n_sclass)
	{
		case C_FILE:
			protorec.add_attr(an_tag,af_tag,t_sourcefile);
			protorec.add_attr(an_name,af_stringndx,
				strdup(aux.x_file.x_fname));
			find_arcs( sibofs, childofs );
			if (has_line_info(childofs))
				protorec.add_attr(an_lineinfo,af_coffline,childofs);
			get_pc_info( childofs, lopc, hipc );
			protorec.add_attr(an_lopc,af_addr,lopc);
			protorec.add_attr(an_hipc,af_addr,hipc);
			break;
		case C_AUTO:
		case C_REG:
			protorec.add_attr(an_tag,af_tag,t_local_variable);
			break;
		case C_ARG:
		case C_REGPARM:
			protorec.add_attr(an_tag,af_tag,t_argument);
			break;
		case C_STAT:
		case C_EXT:
		case C_HIDDEN:
			if (((sym.n_type != T_NULL) && ISFCN(sym.n_type))
				|| ( sym.n_scnum == textsectno ))
			{
				protorec.add_attr(an_tag,af_tag,
					(sym.n_sclass == C_STAT) ? t_subroutine:
						t_global_sub);
				protorec.add_attr(an_lopc,af_addr,sym.n_value);
				protorec.add_attr(an_hipc, af_addr,
					(sym.n_numaux == 0) ? 0L :
					sym.n_value + aux.x_sym.x_misc.x_fsize);
			}
			else
			{
				protorec.add_attr(an_tag,af_tag,
					(sym.n_sclass == C_STAT)
					? t_local_variable: t_global_variable);
			}
			break;
		case C_STRTAG:
			protorec.add_attr(an_tag,af_tag,t_structuretype);
			protorec.add_attr(an_bytesize,af_int,aux.x_sym.x_misc.x_lnsz.x_size);
			break;
		case C_UNTAG:
			protorec.add_attr(an_tag,af_tag,t_uniontype);
			protorec.add_attr(an_bytesize,af_int,aux.x_sym.x_misc.x_lnsz.x_size);
			break;
		case C_ENTAG:
			protorec.add_attr(an_tag,af_tag,t_enumtype);
			protorec.add_attr(an_bytesize,af_int,aux.x_sym.x_misc.x_lnsz.x_size);
			break;
		case C_TPDEF:
			protorec.add_attr(an_tag,af_tag,t_typedef);
			break;
		case C_MOS:
			protorec.add_attr(an_tag,af_tag,t_structuremem);
			break;
		case C_MOE:
			protorec.add_attr(an_tag,af_tag,t_enumlittype);
			protorec.add_attr(an_litvalue,af_int,sym.n_value);
			break;
		case C_MOU:
			protorec.add_attr(an_tag,af_tag,t_unionmem);
			break;
		case C_FIELD:
			protorec.add_attr(an_tag,af_tag,t_bitfield);
			protorec.add_attr(an_bitsize,af_int
				,aux.x_sym.x_misc.x_lnsz.x_size);
			break;
		case C_BLOCK:
			protorec.add_attr(an_tag,af_tag,t_block);
			protorec.add_attr(an_lopc,af_addr,sym.n_value);
			offset = symtab_offset +
				(aux.x_sym.x_fcnary.x_fcn.x_endndx-2) * SYMESZ;
			if ( coff->get_symbol(offset,sym2,aux2) != 0 )
			{
				protorec.add_attr(an_hipc,af_addr,sym2.n_value);
			}
			break;
		case C_LABEL:
		case C_ULABEL:
			protorec.add_attr(an_tag,af_tag,t_label);
			break;
	}

	if (sym.n_sclass != C_BLOCK && sym.n_sclass != C_FILE)
		buildNameAttrs(protorec, sym.n_nptr);
}

// convert the COFF type field into the internal format

void
Coffbuild::get_type_C()
{
	int		dt;
	unsigned short	deriv;
	Attr_value	value;
	Attr_form	form;
	int		i;
	int		is_func = 0;
	int		bound;
	short		d[DTYPES];

	form = af_fundamental_type;	// the normal case
	switch (BTYPE(sym.n_type))
	{
		case T_NULL:
			// no derived type means no information
			// derived type implies ptr to void, or function
			if (sym.n_type == T_NULL)
			{
				Attr_value	value2;
				value.fund_type = ft_none;
				value2.word = 0;
				protorec.add_attr( an_assumed_type, af_int, value2);
			}
			else
				value.fund_type = ft_void;
			break;
		case T_CHAR:
			value.fund_type = ft_char;
			break;
		case T_SHORT:
			value.fund_type = ft_short;
			break;
		case T_INT:
			value.fund_type = ft_int;
			break;
		case T_LONG:
			value.fund_type = ft_long;
			break;
		case T_FLOAT:
			value.fund_type = ft_sfloat;
			break;
		case T_DOUBLE:
			value.fund_type = ft_lfloat;
			break;
		case T_STRUCT:
		case T_UNION:
		case T_ENUM:
			if (aux.x_sym.x_tagndx == 0)
			{
				form = af_none;
				value.word = 0;
			}
			else
			{
				form = af_coffrecord;
				value.word = symtab_offset + aux.x_sym.x_tagndx * SYMESZ;
			}
			break;
		case T_MOE:
			form = af_none;
			value.word = 0;
			break;
		case T_UCHAR:
			value.fund_type = ft_uchar;
			break;
		case T_USHORT:
			value.fund_type = ft_ushort;
			break;
		case T_UINT:
			value.fund_type = ft_uint;
			break;
		case T_ULONG:
			value.fund_type = ft_ulong;
			break;
	}


	// break apart the derived type, 2 bits per derived type,
	// 6 derived types maximum
	deriv = (unsigned short)sym.n_type >> N_BTSHFT;
	bound = 0;
	for ( i = 0 ; i < DTYPES ; i ++ )
	{
		dt = deriv & 0x3;
		if (dt == DT_NON)	// no more derived types
			break;
		if (dt == DT_ARY)
			bound++ ;
		d[i] = dt;
		deriv = deriv >> N_TSHIFT;
	}

	for ( i--; i >= 0 ; i-- )
	{
		switch (d[i])
		{
			case DT_PTR:
				prototype.add_attr(an_tag,af_tag,t_pointertype);
				prototype.add_attr(an_basetype,form,value);
				prototype.add_attr(an_bytesize,af_int,4L);
				value.symbol = prototype.put_record();
				form = af_symbol;
				break;

			case DT_ARY:
				prototype.add_attr(an_tag,af_tag,t_arraytype);
				prototype.add_attr(an_elemtype,form,value);
				prototype.add_attr(an_bytesize,af_int,
					aux.x_sym.x_misc.x_lnsz.x_size);
				prototype.add_attr(an_subscrtype,af_fundamental_type,ft_sint);
				prototype.add_attr(an_lobound,af_int,0L);

				// coff aux. entry holds a maximum of four
				// array bounds also, dimension is 0 to bound-1,
				// not including bound
				prototype.add_attr(an_hibound,af_int,
					(--bound >= DIMNUM) ? 1 :
					(aux.x_sym.x_fcnary.x_ary.x_dimen[bound] - 1));
				value.symbol = prototype.put_record();
				form = af_symbol;
				break;

			case DT_FCN:
				if (i)	// pointer to function, for example
				{
					prototype.add_attr(an_tag,af_tag,t_functiontype);
					prototype.add_attr(an_resulttype,form,value);
					value.symbol = prototype.put_record();
					form = af_symbol;
				}
				else
					is_func = 1;
				break;

			case DT_NON:
				continue;
		}
	}
	protorec.add_attr(is_func ? an_resulttype : an_type, form, value);
}

void
Coffbuild::get_type()
{
	switch(sym.n_sclass)
	{
		case C_STRTAG:
		case C_UNTAG:
		case C_ENTAG:
		case C_FILE:
		case C_BLOCK:
		case C_LABEL:
		case C_ULABEL:
		case C_FCN:
			break;
		default:
			get_type_C();
	}
}

// create a location description for a symbol
// location descriptions are somewhat machine specific

void
Coffbuild::get_addr_C()
{
	Locdesc	locdesc;
	int	size;
	char *	p;

	(void) locdesc.clear();
	switch(sym.n_sclass)
	{
		case C_ARG:
			cvt_arg( locdesc, sym.n_value );
			break;
		case C_REG:
		case C_REGPARM:
			cvt_reg( locdesc, sym.n_value );
			break;
		case C_LABEL:
		case C_EXT:
		case C_FCN:
		case C_BLOCK:
		case C_STAT:
		case C_ULABEL:
		case C_USTATIC:
			cvt_extern( locdesc, sym.n_value );
			break;
		case C_MOU:
		case C_MOS:
			locdesc.offset(sym.n_value).add();
			break;
		case C_AUTO:
			cvt_auto( locdesc, sym.n_value );
			break;
		case C_FIELD:
		{
			int	loc, off;

			coff_bit_loc(sym.n_type, (int)sym.n_value,
				aux.x_sym.x_misc.x_lnsz.x_size, loc, off);
			
			locdesc.offset(loc).add();
			protorec.add_attr(an_bitoffs,af_int, off);
			break;
		}
	}

	size = locdesc.size();
	p = new char[size];
	if (size == 1)
	{
		// locdesc.clear sets size to 1; if its still 1, then
		// there is no location desc
		*p = 0;
	}
	else
	{
		memcpy( p, locdesc.addrexp(), size );
	}
	(void) protorec.add_attr(an_location, af_locdesc, p );
}

void
Coffbuild::get_addr()
{
	get_addr_C();
}

int
Coffbuild::get_syminfo( long offset, Syminfo & syminfo )
{
	if ( (nextofs = coff->get_symbol( offset, sym, aux )) == 0 )
	{
		syminfo.bind = sb_none;
		return 0;
	}
	else if ( sym.n_sclass == C_FILE )
	{
		syminfo.name = (long) aux.x_file.x_fname;
		syminfo.lo = 0;
		syminfo.hi = 0;
	}
	else
	{
		syminfo.name = (long) sym.n_nptr;
		syminfo.lo = sym.n_value;
		if (ISFCN(sym.n_type))
		{
			syminfo.hi = sym.n_value + aux.x_sym.x_misc.x_fsize;
		}
		else if ( strcmp( sym.n_nptr, ".text" ) == 0 )
		{
			syminfo.hi = sym.n_value + aux.x_scn.x_scnlen;
		}
		else
		{
			syminfo.hi = sym.n_value;
		}
	}

	switch ( sym.n_sclass )
	{
		case C_FILE:	syminfo.bind = sb_global;	break;
		case C_EXT:	syminfo.bind = sb_global;	break;
		case C_STAT:	syminfo.bind = sb_local;	break;
		default:	syminfo.bind = sb_none;		break;
	}
	switch ( sym.n_sclass )
	{
		case C_FILE:
			syminfo.type = st_file;
			syminfo.resolved = 1;
			break;
		case C_EXT:
		case C_STAT:
			if (((sym.n_type != T_NULL ) && (ISFCN(sym.n_type)))
				|| ( sym.n_scnum == textsectno ))
			{
				syminfo.type = st_func;
				syminfo.resolved = ((sym.n_scnum != N_UNDEF) &&
					(sym.n_scnum != N_ABS));
			}
			else
			{
				syminfo.type = st_object;
				syminfo.resolved = (sym.n_scnum != N_UNDEF);
			}
			break;
		default:
			syminfo.type = st_none;
			syminfo.resolved = (sym.n_scnum != N_UNDEF);
			break;
	}
	find_arcs( syminfo.sibling, syminfo.child );
	return 1;
}

int
Coffbuild::find_record( long offset, int want_file )
{
	Syminfo		syminfo;
	long		disp;
	char *		name;

	disp = offset;
	while ( get_syminfo( disp, syminfo ) != 0 )
	{
		if ( want_file && (syminfo.type == st_file) )
		{
			return 1;
		}
		else if ( !want_file && (syminfo.type == st_file) )
		{
			return 0;
		}
		name = (char*)syminfo.name;
		if ( strcmp( name, ".ef" ) == 0 || strcmp( name, ".eb" ) == 0 )
		{
			return 0;
		}
		// if the symbol is one of these, get the next symbol
		// in every other case, it either has the right symbol
		// or something is fouled up
		else if (strcmp(name,".bf") == 0 ||
			strcmp(name,".target") == 0 ||
			strcmp(name,".comment") == 0 ||
			strcmp(name,".text") == 0 ||
			strcmp(name,".data") == 0 ||
			strcmp(name,".bss") == 0 ||
			strcmp(name,".init") == 0)
		{
			disp = syminfo.sibling;
			continue;
		}
		else
		{
			return 1;
		}
	}
}

// convert a COFF symbol table entry into an internal attribute list

Attribute *
Coffbuild::make_record( long offset, int want_file )
{
	Attribute *	attribute;

	if ( offset == 0 || find_record( offset, want_file ) == 0 )
	{
		return 0;
	}
	else if ( reflist.lookup( offset, attribute ) )
	{
		return attribute;
	}
	else if ( sym.n_sclass == C_EOS )
	{
		return 0;
	}
	get_arcs();
	get_data();
	get_type();
	get_addr();
	attribute = protorec.put_record();
	reflist.add(offset,attribute);
	return attribute;
}

void
Coffbuild::get_pc_info( long offset, Iaddr & lopc, Iaddr & hipc )
{
	Syminfo		syminfo;
	char *		name;

	lopc = 0;
	hipc = 0;
	while ( get_syminfo( offset , syminfo ) )
	{
		name = (char*) syminfo.name;
		if ( syminfo.type == st_file )
		{
			offset = syminfo.child;
		}
		else if ( strcmp(name,".text") == 0 )
		{
			lopc = syminfo.lo;
			hipc = syminfo.hi;
			return;
		}
		else
		{
			offset = syminfo.sibling;
		}
	}
}

const char *
Coffbuild::get_fcn_lineinfo( const char * start, const char * end )
{
	long		pcval;
	int		lowline;
	struct lineno	line;
	int		i;
	const char *	ptr = start + LINESZ;

	// address of the function symbol
	pcval = sym.n_value;

	// get the .bf symbol, which has the line number of the beginning
	// of the function.  all the rest are relative to that, and must
	// be converted to absolute numbers
	coff->get_symbol( nextofs, sym, aux );
	lowline = aux.x_sym.x_misc.x_lnsz.x_lnno;

	i = (end - start)/ LINESZ;
	(void) memcpy( &line, start, LINESZ );
	for ( ; i > 0 && line.l_lnno != 0 ; i-- )
	{
		protoline.add_line( pcval, lowline+line.l_lnno-1 );
		(void) memcpy( &line, ptr, LINESZ );
		pcval = line.l_addr.l_paddr;
		ptr += LINESZ;
	}
	return ptr;
}

// determine whether the .file symbol at offset has line information

int
Coffbuild::has_line_info( long offset )
{
	int	notext = 1;
	int	nofcn = 1;
	long	ignore;
	long	loffset = 0;	// offset of the line numbers
	long	foffset = 0;	// offset of first function symbol table entry
	long	lncnt = 0;
	Iaddr	last_hipc;

	// loop until we have both the first function symbol and the .text
	// symbol for the file.  stop if it hits the next .file

	while ( notext || nofcn )
	{
		if ( (nextofs = coff->get_symbol(offset, sym, aux )) == 0
			|| sym.n_sclass == C_FILE )
		{
			break;
		}

		if ( nofcn && ISFCN(sym.n_type))
		{
			foffset = offset;
			loffset = aux.x_sym.x_fcnary.x_fcn.x_lnnoptr;
			nofcn = 0;
		}
		else if ( notext && strcmp(sym.n_nptr,".text") == 0)
		{
			lncnt = aux.x_scn.x_nlinno;
			last_hipc = sym.n_value + aux.x_scn.x_scnlen;
			notext = 0;
		}
		find_arcs(offset,ignore);
	}
	if ( loffset && (loffset >= linedisp) && (loffset < symtab_offset) && (lncnt > 0) )
		return 1;
	else
		return 0;
}

// get the line number entries for the .file symbol at offset

Lineinfo *
Coffbuild::line_info( long offset )
{
	int	notext = 1;
	int	nofcn = 1;
	long	ignore;
	long	loffset = 0;	// offset of the line numbers
	long	foffset = 0;	// offset of first function symbol table entry
	long	lncnt = 0;
	Iaddr	last_hipc;
	Lineinfo *	lineinfo = 0;

	// loop until we have both the first function symbol and the .text
	// symbol for the file.  stop if it hits the next .file

	while ( notext || nofcn )
	{
		if ( (nextofs = coff->get_symbol(offset, sym, aux )) == 0
			|| sym.n_sclass == C_FILE )
		{
			break;
		}

		if ( nofcn && ISFCN(sym.n_type))
		{
			foffset = offset;
			loffset = aux.x_sym.x_fcnary.x_fcn.x_lnnoptr;
			nofcn = 0;
		}
		else if ( notext && strcmp(sym.n_nptr,".text") == 0)
		{
			lncnt = aux.x_scn.x_nlinno;
			last_hipc = sym.n_value + aux.x_scn.x_scnlen;
			notext = 0;
		}
		find_arcs(offset,ignore);
	}
	if (loffset && (loffset >= linedisp) && (loffset < symtab_offset) && (lncnt > 0) )
	{
		get_lineinfo( loffset, lncnt, foffset );
		lineinfo = protoline.put_line(last_hipc);
	}
	return lineinfo;
}

// get_lineinfo calls get_fcn_lineinfo to fill in the line number entries
// for each function symbol under the file symbol at foffset

void
Coffbuild::get_lineinfo( long loffset, long lncnt, long foffset )
{
	const char *	start;
	const char *	cofflbuf;
	long		ignore;
	long		offset;

	cofflbuf = coff->get_lineno( loffset );
	start = cofflbuf + LINESZ;
	offset = foffset;
	while ( (nextofs = coff->get_symbol(offset, sym, aux )) != 0 )
	{
		// get the sibling pointer for the next symbol
		// this has to be down before the call to get_fcn_lineinfo,
		// which changes the current symbol (to .bf)
		find_arcs( offset, ignore );

		if (ISFCN(sym.n_type))
		{
			start = get_fcn_lineinfo( start, cofflbuf + lncnt * LINESZ );
		}
		else if ( sym.n_sclass == C_FILE )
		{
			break;
		}
	}
}

long
Coffbuild::globals_offset()
{
	return symtab_offset;
}

long
Coffbuild::first_symbol()
{
	return symtab_offset;
}
