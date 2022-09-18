/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _UTIL_MERGE_MERGE386_H	/* wrapper symbol for kernel use */
#define _UTIL_MERGE_MERGE386_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:util/merge/merge386.h	1.2"
#ident	"$Header: $"

/***************************************************************************

       Copyright (c) 1991 Locus Computing Corporation.
       All rights reserved.
       This is an unpublished work containing CONFIDENTIAL INFORMATION
       that is the property of Locus Computing Corporation.
       Any unauthorized use, duplication or disclosure is prohibited.

***************************************************************************/


/*
**  merge386.h
**		structure definitions for Merge hooks. 
*/

struct mrg_com_data {
	int (*com_ppi_func)();		/* Pointer to a Merge function  */
					/* to call depending on the VPI */
					/* currently attached to this   */
					/* device. NULL if no VPI is    */
					/* attached.		        */
	unsigned char *com_ppi_data; 	/* Pointer to a structure to    */
					/* pass as an argument to the   */
					/* above function		*/
	int openflag;			/* Flag to set when the device  */
					/* is in use by Unix 	        */
	int baseport;			/* Base I/O port for this dev.  */
};

#endif	/* _UTIL_MERGE_MERGE386_H */
