/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Fund_type_h
#define Fund_type_h
#ident	"@(#)debugger:inc/common/Fund_type.h	1.1"

enum Fund_type {
	ft_none,	// not a type representation
	ft_char,	// generic character
	ft_schar,	// signed character
	ft_uchar,	// unsigned character
	ft_short,	// generic short
	ft_sshort,	// signed short
	ft_ushort,	// unsigned short
	ft_int,		// generic integer
	ft_sint,	// signed integer
	ft_uint,	// unsigned integer
	ft_long,	// generic long
	ft_slong,	// signed long
	ft_ulong,	// unsigned long
	ft_pointer,	// untyped pointer, void *
	ft_sfloat,	// short float
	ft_lfloat,	// long float (double)
	ft_xfloat,	// extra long float (long double)
	ft_scomplex,	// Fortran complex
	ft_lcomplex,	// Fortran double precision complex
	ft_set,		// Pascal set
	ft_void,	// C void
	ft_string,	// string constant (internal)
};

#endif /* Fund_type_h */
