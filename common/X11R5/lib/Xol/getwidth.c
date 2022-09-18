/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:getwidth.c	1.1"
#endif

#include <libw.h>
#include "_wchar.h"
#include <ctype.h>

void getwidth(eucstruct)
eucwidth_t *eucstruct;
{
	eucstruct->_eucw1 = eucw1;
	eucstruct->_eucw2 = eucw2;
	eucstruct->_eucw3 = eucw3;
	eucstruct->_multibyte = multibyte;
	if (_ctype[520] > 3 || eucw1 > 2)
		eucstruct->_pcw = sizeof(unsigned long);
	else
		eucstruct->_pcw = sizeof(unsigned short);
	eucstruct->_scrw1 = scrw1;
	eucstruct->_scrw2 = scrw2;
	eucstruct->_scrw3 = scrw3;
}
