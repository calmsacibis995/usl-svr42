/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Syminfo.h	1.1"
#ifndef Syminfo_h
#define Syminfo_h

enum sym_bind	{ sb_none, sb_local, sb_global, sb_weak };

enum sym_type	{ st_none, st_object, st_func, st_section, st_file };

struct Syminfo {
	long		name;
	long		lo,hi;
	unsigned char	bind,type;
	long		sibling,child;
	int		resolved;
};

#endif /* Syminfo_h */
