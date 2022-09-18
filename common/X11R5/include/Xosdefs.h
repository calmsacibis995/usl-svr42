/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5include:Xosdefs.h	1.1"
/*
 * O/S-dependent (mis)feature macro definitions
 *
 * $XConsortium: Xosdefs.h,v 1.7 91/07/19 23:22:19 rws Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 */

#ifndef _XOSDEFS_H_
#define _XOSDEFS_H_

/*
 * X_NOT_STDC_ENV means does not have ANSI C header files.  Lack of this
 * symbol does NOT mean that the system has stdarg.h.
 *
 * X_NOT_POSIX means does not have POSIX header files.  Lack of this
 * symbol does NOT mean that the POSIX environment is the default.
 * You may still have to define _POSIX_SOURCE to get it.
 */

#ifdef NOSTDHDRS
#define X_NOT_POSIX
#define X_NOT_STDC_ENV
#endif

#ifdef sony
#ifndef SYSTYPE_SYSV
#define X_NOT_POSIX
#endif
#endif

#ifdef UTEK
#define X_NOT_POSIX
#define X_NOT_STDC_ENV
#endif

#ifdef CRAY
#define X_NOT_POSIX
#endif

#ifdef vax
#ifndef ultrix			/* assume vanilla BSD */
#define X_NOT_POSIX
#define X_NOT_STDC_ENV
#endif
#endif

#ifdef luna
#define X_NOT_POSIX
#define X_NOT_STDC_ENV
#endif

#ifdef Mips
#define X_NOT_POSIX
#define X_NOT_STDC_ENV
#endif
  
#ifdef USL
#ifdef SYSV /* (release 3.2) */
#define X_NOT_POSIX
#define X_NOT_STDC_ENV
#endif
#endif

#ifdef SYSV386
#ifdef SYSV
#define X_NOT_POSIX
#define X_NOT_STDC_ENV
#endif
#endif

#ifdef MOTOROLA
#ifdef SYSV
#define X_NOT_STDC_ENV
#endif
#endif

#endif /* _XOSDEFS_H_ */
