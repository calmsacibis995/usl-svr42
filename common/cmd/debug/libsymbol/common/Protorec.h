/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Protorec.h	1.1"
#ifndef Protorec_h
#define Protorec_h

#include	"Attribute.h"
#include	"Vector.h"

class Protorec {
	Vector		vector;
	int		count;
public:
			Protorec();
			~Protorec()	{}
	Protorec &	add_attr(Attr_name, Attr_form, long );
	Protorec &	add_attr(Attr_name, Attr_form, void * );
	Protorec &	add_attr(Attr_name, Attr_form, const Attr_value & );
	Attribute *	put_record();
};

#endif /* Protorec_h */
