/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xterm:Tekparse.h	1.7"
/*
 *	$XConsortium: Tekparse.h,v 1.3 88/09/06 17:07:31 jim Exp $
 */

#include <X11/copyright.h>

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                         All Rights Reserved
 *
 *
 *
 */


/* @(#)Tekparse.h	X10/6.6	11/7/86 */

/*
 * The following list of definitions is generated from Tekparse.def using the
 * following command line:
 *
 *     egrep -v '^CASE_' Tekparse.def | \
 *     awk 'BEGIN {n = 0;} {printf "#define %s %d\n", $1, n; n++}' 
 *
 * You you need to change something, change Tekparse.def and regenerate the
 * definitions.  This would have been automatic, but since this doesn't change
 * very often, it isn't worth the makefile hassle.
 */

#define CASE_REPORT 0
#define CASE_VT_MODE 1
#define CASE_SPT_STATE 2
#define CASE_GIN 3
#define CASE_BEL 4
#define CASE_BS 5
#define CASE_PT_STATE 6
#define CASE_PLT_STATE 7
#define CASE_TAB 8
#define CASE_IPL_STATE 9
#define CASE_ALP_STATE 10
#define CASE_UP 11
#define CASE_COPY 12
#define CASE_PAGE 13
#define CASE_BES_STATE 14
#define CASE_BYP_STATE 15
#define CASE_IGNORE 16
#define CASE_ASCII 17
#define CASE_APL 18
#define CASE_CHAR_SIZE 19
#define CASE_BEAM_VEC 20
#define CASE_CURSTATE 21
#define CASE_PENUP 22
#define CASE_PENDOWN 23
#define CASE_IPL_POINT 24
#define CASE_PLT_VEC 25
#define CASE_PT_POINT 26
#define CASE_SPT_POINT 27
#define CASE_CR 28
#define CASE_ESC_STATE 29
#define CASE_LF 30
#define CASE_SP 31
#define CASE_PRINT 32
#define CASE_OSC 33
#define CASE_VT_SWITCH 34     
