/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cvtomf:coff.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*
 *	Copyright (c) Altos Computer Systems, 1987
 */


/* Enhanced Application Compatibility Support */
#define		ALIGNED(x)	(!((x) & 3))

#define		TXTFILL		(0x90)
#define		DATFILL		(0x00)
/* End Enhanced Application Compatibility Support */
