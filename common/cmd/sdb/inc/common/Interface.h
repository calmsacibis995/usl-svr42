/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/common/Interface.h	1.7"

// Debugger interface structure.

// Enables different interfaces (e.g., screen and line mode)
// to work together.

#ifndef Interface_h
#define Interface_h

#include <stdio.h>
#include <stdarg.h>
#include "Itype.h"

class	Vector;
class	Process;

extern int interrupted;

enum	OutType	{ ot_none, ot_file, ot_vector };

enum	CmdType { NOCOM, PRCOM, DSCOM, DSICOM };

extern CmdType	lastcom;
extern Iaddr	instaddr;

struct	OutPut {
	OutType	type;
	union {
		FILE *		fp;
		Vector *	vec;
	};
};

extern	int	pushoutfile( FILE * );
extern	int	pushoutvec( Vector * );
extern	void	popout();

extern	int	show_location( Process *, Iaddr, int );
extern	int	printx( const char *, ... );
extern	int	printe( const char *, ... );

#endif	/* Interface_h */
