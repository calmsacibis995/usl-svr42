/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef RegAccess_h
#define RegAccess_h
#ident	"@(#)debugger:inc/common/RegAccess.h	1.2"

#include "Itype.h"
#include "Reg.h"
#include "Proctypes.h"

class Core;
class Frame;
class Proccore;
class Proclive;

class RegAccess {
	Proclive	*pctl;
	greg_ctl	gpreg;
	fpreg_ctl	fpreg;
	int		fpcurrent;
	Proccore 	*core;
	int		readlive( RegRef, long * );
	int		readcore( RegRef, long * );
	int		writelive( RegRef, long * );
public:
			RegAccess();
			~RegAccess() {};
	int		setup_live( Proclive * );
	int		setup_core( Proccore * );
	int		update();
	Iaddr		getreg( RegRef );
	int		readreg( RegRef, Stype, Itype & );
	int		writereg( RegRef, Stype, Itype & );
	int		display_regs(Frame *);
	int		set_pc(Iaddr);
};

#endif

// end of RegAccess.h
