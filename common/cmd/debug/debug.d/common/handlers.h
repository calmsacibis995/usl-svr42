/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * $Copyright: $
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

/* Connects sig_handle.c with the C++ handlers */

#ifndef _Handlers_h
#define _Handlers_h

#ident	"@(#)debugger:debug.d/common/handlers.h	1.1"

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

SIG_TYP poll_handler();
SIG_TYP inform_handler();
SIG_TYP fault_handler();
SIG_TYP internal_error_handler();
SIG_TYP suspend_handler();

#ifdef __cplusplus
}
extern void add_error_handler(void(*error_handler)(int));
#else
extern void add_error_handler(SIG_TYP error_handler);
#endif


#endif
