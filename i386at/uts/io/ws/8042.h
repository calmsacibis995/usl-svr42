/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_WS_8042_H	/* wrapper symbol for kernel use */
#define _IO_WS_8042_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/ws/8042.h	1.4"
#ident	"$Header: $"

/* defines for i8042_program() */
#define	P8042_KBDENAB	1
#define	P8042_KBDDISAB	2
#define	P8042_AUXENAB	3
#define	P8042_AUXDISAB	4

/* defines for i8042_send_cmd */
#define	P8042_TO_KBD	1
#define	P8042_TO_AUX	2


extern void	i8042_acquire();
extern void 	i8042_release();
extern int	i8042_send_cmd();
extern void	i8042_program();
extern int	i8042_aux_port();

#endif /* _IO_WS_8042_H */
