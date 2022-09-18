/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_QT_TAPE_H	/* wrapper symbol for kernel use */
#define _IO_QT_TAPE_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/qt/tape.h	1.5"
#ident	"$Header: $"

/*	Copyright (c) 1986, 1987  Intel Corporation	*/
/*	All Rights Reserved	*/

/* 
 * Standard Tape ioctl commands.  
 */

#define T_BASE		('[' << 8)
#define T_RETENSION	(T_BASE | 001) 
#define T_RWD		(T_BASE | 002)
#define T_ERASE		(T_BASE | 003)
#define T_WRFILEM	(T_BASE | 004)
#define T_RST		(T_BASE | 005)
#define T_RDSTAT	(T_BASE | 006)
#define T_SFF		(T_BASE | 007)
#define T_SBF		(T_BASE | 010)
#define T_LOAD		(T_BASE | 011)
#define T_UNLOAD	(T_BASE | 012)
#define T_SFREC		(T_BASE | 013)
#define T_SBREC 	(T_BASE | 014)
#define T_TINIT 	(T_BASE | 015)

/*
 *	9 track support ioctls -- 
*/

#define	T_RDBLKLEN	(T_BASE | 016)	/* Read block size		*/
#define	T_WRBLKLEN	(T_BASE | 017)	/* Set block size		*/
#define	T_PREVMV	(T_BASE | 020)	/* Prevent media removal	*/
#define	T_ALLOMV	(T_BASE | 021)	/* Allow media removal		*/

#endif	/* _IO_QT_TAPE_H */
