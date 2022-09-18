/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Attribute_h
#define Attribute_h
#ident	"@(#)debugger:inc/common/Attribute.h	1.2"

#include "Language.h"
#include "Itype.h"
#include "Fund_type.h"
#include "Locdesc.h"
#include "Tag.h"	// enum Tag;

// An Attribute is a 2-word struct, consisting of a name, a form, and a value.
// The name is a short, containing one of the members of enum Attr_name.
// The form is also a short, containing one of the members of enum Attr_form.
// The value is a union of several (word-sized) types.
//
// Each attribute name may have one or more forms.  Each form implies one
// member of the "value" union.
//
// An attribute list (the representation of a Symbol) is an array containing
// at least three Attributes:
//
//	the first is always "an_count", "af_int", value.word = number of
//		entries in this list, inclusive
//	the second is always "an_tag", "af_tag", value.tag = tag for this
//		record (see Tag.h and Tag1.h)
//	the last is always "an_nomore", "af_none", value.word = 0
//
// Both a count and an end-marker are used for efficiency in searching for
// a named attribute.  See find_attr() in builder.C.  Only one attribute with
// any given Attr_name is allowed in an attribute list.  This is not enforced,
// but only the first one will ever be found by find_attr().

typedef enum	{
	an_nomore,	// af_none
	an_tag,		// af_tag
	an_name,	// af_stringndx
	an_mangledname,	// af_stringndx
	an_child,	// af_symbol, af_coffrecord, af_dwarfoffs
	an_sibling,	// ditto, plus af_cofffile
	an_parent,	// af_symbol, af_coffrecord, af_dwarfoffs
	an_count,	// af_int
	an_type,	// af_fundamental_type, af_symbol, af_coffrecord, af_dwarfoffs
	an_elemtype,	// ditto
	an_elemspan,	// unused
	an_subscrtype,	// af_fundamental_type
	an_lobound,	// af_int
	an_hibound,	// af_int
	an_basetype,	// af_fundamental_type, af_symbol, af_dwarfoffs
	an_resulttype,	// af_fundamental_type, af_symbol, af_coffrecord, af_dwarfoffs
	an_argtype,	// unused
	an_bytesize,	// af_int
	an_bitsize,	// af_int
	an_bitoffs,	// af_int
	an_litvalue,	// af_int
	an_stringlen,	// unused
	an_lineinfo,	// af_lineinfo, af_coffline, af_dwarfline
	an_location,	// af_locdesc
	an_lopc,	// af_addr
	an_hipc,	// af_addr
	an_visibility,	// unused
	an_scansize,	// af_int
	an_language,	// af_language
	an_assumed_type,  // af_int
} Attr_name;

typedef enum	{
	af_none,
	af_tag,			// value.tag
	af_int,			// value.word
	af_locdesc,		// value.loc
	af_stringndx,		// value.name
	af_coffrecord,		// value.word
	af_coffline,		// value.word
	af_coffpc,		// unused
	af_spidoffs,		// unused
	af_fundamental_type,	// value.fund_type
	af_symndx,		// unused
	af_reg,			// unused
	af_addr,		// value.addr
	af_local,		// unused
	af_visibility,		// unused
	af_lineinfo,		// value.lineinfo
	af_attrlist,		// unused
	af_cofffile,		// value.word
	af_symbol,		// value.symbol
	af_dwarfoffs,		// value.word
	af_dwarfline,		// value.word
	af_elfoffs,		// value.word
	af_language,		// value.language
} Attr_form;

struct Attribute;
struct Lineinfo;

union Attr_value {
	Iaddr		addr;
	Fund_type	fund_type;
	Lineinfo       *lineinfo;
	Addrexp		loc;
	char	       *name;
	Attribute      *symbol;
	Tag		tag;
	Language	language;
	long		word;
};

struct Attribute {
	short		name;	// cfront bug: Attr_name
	short		form;	// cfront bug: Attr_form
	Attr_value	value;
}; 

#ifdef DEBUG
void dumpAttr(Attribute *root, int indent=1);
#endif

#endif /* Attribute_h */
