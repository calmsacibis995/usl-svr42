/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wmisc/getwidth.c	1.1.2.2"
#ident  "$Header: getwidth.c 1.2 91/06/26 $"
#include <widec.h>
#include "libw.h"
#include "_wchar.h"
#include <ctype.h>

void getwidth(eucstruct)
eucwidth_t *eucstruct;
{
	eucstruct->_eucw1 = eucw1;
	eucstruct->_eucw2 = eucw2;
	eucstruct->_eucw3 = eucw3;
	eucstruct->_multibyte = multibyte;
	eucstruct->_pcw = sizeof(wchar_t);
	eucstruct->_scrw1 = scrw1;
	eucstruct->_scrw2 = scrw2;
	eucstruct->_scrw3 = scrw3;
}
