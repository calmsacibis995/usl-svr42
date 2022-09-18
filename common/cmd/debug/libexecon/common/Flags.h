/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libexecon/common/Flags.h	1.1"
#ifndef Flags_h
#define Flags_h

#define IS_DISABLED(x)	( !((x) & 0x1) )
#define IS_ENABLED(x)	( (x) & 0x1 )
#define DISABLE(x)	( x = (x) & ~0x1 )
#define ENABLE(x)	( x = (x) | 0x1 )

#define REMOVE(x)	( x = (x) & ~0x2 )
#define IS_INSERTED(x)	( (x) & 0x2 )
#define INSERT(x)	( x = (x) | 0x2 )

#define SET_ANNOUNCE(x)	( x = (x) | 0x4 )
#define ANNOUNCE(x)	( (x) & 0x4 )

#endif

// end of Flags.h

