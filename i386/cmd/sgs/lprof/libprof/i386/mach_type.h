/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/i386/mach_type.h	1.1"

#define	MACH_TYPE	M386
#define CANAME		"__coverage."
#define COV_PREFIX	"__coverage."

/*
*	NOTE: When you change MATCH_STR, also change pcrt1.s.
*	(See notes on "verify_match()" in file symintLoad.c.)
*/
#if defined(__STDC__)
#define	MATCH_NAME	_edata
#define	MATCH_STR	"_edata"
#else
#define	MATCH_NAME	edata
#define	MATCH_STR	"edata"
#endif
