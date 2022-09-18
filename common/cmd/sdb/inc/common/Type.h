/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/common/TYPE.h	1.3"

#ifndef TYPE_h
#define TYPE_h

#include "Symbol.h"

enum Stype;

enum Type_form {
    TF_fund,  // char, short, int, unsigned int, ...
    TF_user   // ptr, array, struct, enum, ...
};
enum Fund_type;

class TYPE {
    Type_form _form;
    Fund_type ft;     // meaningful iff form == TF_fund.
    Symbol    symbol; // meaningful iff form == TF_user.
public:

    void null();      // make null.
    int  isnull();    // is null ?
    TYPE()  { null(); }

    TYPE(TYPE&);		// bit copy. (needed because of cfront bug)
    TYPE& operator=(TYPE&);	// bit copy. (needed because of cfront bug)

    TYPE& operator=(Fund_type);       // init as a fundamental type.
    TYPE& operator=(Symbol&);         // init as a user defined type.

    Type_