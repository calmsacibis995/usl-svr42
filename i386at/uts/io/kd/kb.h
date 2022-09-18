/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_KD_KB_H	/* wrapper symbol for kernel use */
#define _IO_KD_KB_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/kd/kb.h	1.4"
#ident	"$Header: $"

#define SEND2KBD(port, byte) { \
	while (inb(KB_STAT) & KB_INBF) \
		; \
	outb(port, byte); \
}

#endif /* _IO_KD_KB_H */
