/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CREGS_H	/* wrapper symbol for kernel use */
#define _PROC_CREGS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:proc/cregs.h	1.2"
#ident	"$Header: $"

/*
 * Control Register bits
 */

#define	CR0_PE		0x01		/* Protection Enable		*/
#define	CR0_MP		0x02		/* Math coprocessor Present	*/
#define	CR0_EM		0x04		/* use math Emulation		*/
#define	CR0_TS		0x08		/* Task Switched		*/
#define	CR0_ET		0x10		/* Extension Type (1->387)	*/
#define	CR0_NE		0x20		/* Numeric Error		*/

#define	CR4_VME		0x01		/* Virtual-8086 Mode Extensions	*/
#define	CR4_PVI		0x02		/* Protect-Mode Virtual Int	*/
#define	CR4_TSD		0x04		/* Time Stamp counter Disable	*/
#define	CR4_DE		0x08		/* Debugging Extensions		*/
#define	CR4_PSE		0x10		/* Page Size Extension 		*/
#define	CR4_PAE		0x20		/* Page Address Extension 	*/
#define	CR4_MCE		0x40		/* Machine Check Exceptions	*/

#endif	/* _PROC_CREGS_H */
