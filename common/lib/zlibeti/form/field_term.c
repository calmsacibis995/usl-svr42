/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/field_term.c	1.1"

#include "utility.h"

	/*******************
	*  set_field_term  *
	*******************/

int set_field_term (f, func)
FORM * f;
PTF_void func;
{
	Form (f) -> fieldterm = func;
	return E_OK;
}

PTF_void field_term (f)
FORM * f;
{
	return Form (f) -> fieldterm;
}

