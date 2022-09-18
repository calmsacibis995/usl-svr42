/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fsba:i386/cmd/fsba/fsba.h	1.1"
#ident	"$Header: fsba.h 1.1 91/07/08 $"

#define DEF_BLOCKSIZE	1024
#define	SECTSIZE	512	/* size of sector (physical block) */
#define	SECTPERLOG(b)	(b/SECTSIZE)
