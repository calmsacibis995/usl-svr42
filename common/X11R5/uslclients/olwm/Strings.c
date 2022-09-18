/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Strings.c	1.2"
#endif

/*
 ************************************************************************	
 * Description:
 *	This file contains the external string definitions.
 ************************************************************************	
 */

#ifndef EXT_STRING
#if defined(__STDC__)
#define EXT_STRING(var,val)		extern const char var[] = val
#else
#define EXT_STRING(var,val)		char var[] = val
#endif

#endif

#include <Strings.h>
