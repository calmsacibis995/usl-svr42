/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/form_sub.c	1.1"

#include "utility.h"

	/*****************
	*  set_form_sub  *
	*****************/

int set_form_sub (f, window)
FORM * f;
WINDOW * window;
{
	if (Status (f, POSTED))
		return E_POSTED;

	Form (f) -> sub = window;
	return E_OK;
}

WINDOW * form_sub (f)
FORM * f;
{
	return Form (f) -> sub;
}

