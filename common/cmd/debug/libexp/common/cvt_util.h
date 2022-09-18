/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $Copyright: $
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
#ifndef _cvt_util_h
#define _cvt_util_h

#ident	"@(#)debugger:libexp/common/cvt_util.h	1.7"

#include "Fund_type.h"
#include "TYPE.h"
#include "Language.h"

class Vector;
class Rvalue;
struct fp_x_t;

int CCtype_match(TYPE&, TYPE& );
// the following conversion routines convert an rvalue to a 
// C type
int CCconvert(Rvalue *, TYPE&);
int cvtTo_and_return_ULONG(Rvalue *rval, unsigned long &ulRslt);
int cvtTo_and_return_LONG(Rvalue *rval,  long &lRslt );
int cvtTo_and_return_INT(Rvalue *rval,  int &iRslt );
int cvtTo_and_return_DOUBLE(Rvalue *rval, double &dbRslt );
int cvtTo_and_return_LDOUBLE(Rvalue *rval, fp_x_t &dbRslt );

// the following conversion routines do the conversion in place, that is,
// they take and return an Rvalue
int cvt_to_SINT(Fund_type, Fund_type, Rvalue&);
int cvt_to_UINT(Fund_type, Fund_type, Rvalue&);
int cvt_to_FP(Fund_type, Fund_type, Rvalue&);
int cvt_to_STR(Fund_type, Fund_type, Rvalue&);
int cvt_INT_to_SINT(Fund_type, Fund_type, Vector&);
int cvt_INT_to_UINT(Fund_type, Fund_type, Vector&);
void double2extended(double*, void *);
void extended2double(void *,double *);
char *print_extended(void *val, char format, char *format_str, char *buf);

// error handling
#ifdef __cplusplus
void init_fp_error_recovery(void(*)(int));
#else
typedef void (*SIG_HANDLER) (int);
void init_fp_error_recovery(SIG_HANDLER);
#endif
void clear_fp_error(void);
void clear_fp_error_recovery(void);

// -------------------  Operand Conversion Support ----------------------

// The following should apply to C and C++ (Language: C, CPLUS).

enum FT_GRP {
	ftgSINT,	// char, short, ..
	ftgUINT,	// unsigned char, unsigned short, ...
	ftgPTR,		// void *
	ftgFP,		// float, double, ..
	ftgSTR,		// string (debug variable)
	ftgOTHER,	// none of the above.
	ftgMAX		// ** value of this enum == #members.
};
enum CVT_ACTION {
	cNULL,		// ok, nothing more to do.
	cNEQV,		// not equivalent - not convertable.
	cSINT,		// convert to signed int.
	cUINT,		// convert to unsigned int.
	cSTR,		// convert to string
	cTOFP,		// convert to floating point type.
};
// -- The following table encapsules most of the fundemental
//    type conversion policy choices.  Hopefully it will be
//    easy to change when new types are added and/or a different
//    version of the rules is desired.

extern CVT_ACTION ft_cvt_table[ftgMAX][ftgMAX];
extern FT_GRP ft_group(Fund_type );

// long double constants in fpemu format
extern fp_x_t	fp_zero;
extern fp_x_t	fp_one;
extern fp_x_t	fp_neg_one;

// convert to/from internal long double representation to
// floating point emulation representation
extern void cvt_to_internal(void *internal, fp_x_t &emu);
extern void cvt_to_emu(fp_x_t &emu, void *internal);

#endif
