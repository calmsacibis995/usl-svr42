/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Source_h
#define Source_h
#ident	"@(#)debugger:inc/common/Source.h	1.1"

#include	"Iaddr.h"

struct	Lineinfo;

class Source {
	Lineinfo *	lineinfo;
	Iaddr		ss_base;
	friend class	Symbol;
public:
			Source();
			Source(const Source& );
	void		pc_to_stmt( Iaddr pc, long& line, int slide = -1 );
	void		stmt_to_pc( long line, Iaddr& pc, int slide  = 0 );
	Source &	operator=( const Source & );
};

#endif

// end of Source.h

