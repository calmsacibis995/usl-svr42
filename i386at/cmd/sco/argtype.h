/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sco:argtype.h	1.1"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/* Enhanced Application Compatibility Support */
#define UNKNOWN	-1
#define COFF	0
#define XOUT	1
#define OMF	2
#define XARCH	3
#define UARCH	4
#define OPT	5
#define CREAT	6


int argtype( char * );
/* End Enhanced Application Compatibility Support */
