/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)truss:i386/cmd/truss/machdep.h	1.1"
#ident	"$Header: machdep.h 1.1 91/07/09 $"

#define R_1	9	/* EDX */
#define R_0	11	/* EAX */
#define	R_PC	14	/* EIP */
#define	R_PS	16	/* EFL */
#define	R_SP	17	/* UESP */

#define SYSCALL_OFF	7

typedef	long	syscall_t;
#define	SYSCALL	0x9a000000000700
#define	ERRBIT	0x1

#define PRT_SYS	prt_si86

#if	defined(__STDC__)
extern	CONST char *	si86name( int );
extern	void	prt_si86( int , int );
#else	/* defined(__STDC__) */
extern	CONST char *	si86name();
extern	void	prt_si86();
#endif
