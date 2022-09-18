/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/hdrs/rsmethod.h	1.1.5.2"
#ident  "$Header: rsmethod.h 1.2 91/06/21 $"

/* This file contains information about the Restore Method Description File */

/* Field Names */
#define	RSM_NAME	(unsigned char *)"name"
#define	RSM_COVERAGE	(unsigned char *)"coverage"
#define	RSM_TYPES	(unsigned char *)"types"

#define RSM_ENTRY_F	(unsigned char *)"name:coverage:types"
