/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Segment_h
#define Segment_h
#ident	"@(#)debugger:libexecon/common/Segment.h	1.4"

#include "Itype.h"
#include "Link.h"
#include "Symtab.h"

class	Object;
class	Procctl;
class	Segdata;	// for machine dependent segment information

class Segment: public Link {
	short		is_write;
	short		is_exec;
	Iaddr		loaddr,hiaddr;
	Procctl		*access;
	long		base;
	const char	*pathname;
	Segdata		*_segdata;
	Object		*object;
	Symtab		sym;
	Segment		*next()	{ return (Segment*)Link::next(); }
	friend class	Seglist;
public:
			Segment( Procctl *, const char *path, 
				Object *obj, Iaddr lo, 
				long sz, long base, long ss_base, 
				int iswrite, int isexec);
			~Segment()	{ delete (void *)pathname; }

	int		read( Iaddr, void *, int len );
	int		write( Iaddr, const void *, int len  );
	int		read( Iaddr, Stype, Itype & );
	int		write( Iaddr, Stype, const Itype & );

	int		get_symtable();
	Segdata		*segdata() { return _segdata; }
	void		set_sdata(Segdata *s) { _segdata = s; }
	const char	*name() { return pathname; }
};

int 	stype_size( Stype );
#endif

// end of Segment.h

