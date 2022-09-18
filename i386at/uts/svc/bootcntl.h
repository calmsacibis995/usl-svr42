/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_BOOTCNTL_H		/* wrapper symbol for kernel use */
#define _SVC_BOOTCNTL_H		/* subject to change without notice */

#ident	"@(#)uts-x86at:svc/bootcntl.h	1.2"
#ident	"$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#define	BC_MAXARGS	4
#define	BPRG_MAGIC	0xB00BF00D	/* boot program magic number	      */

struct bootcntl {
	unsigned long	bc_magic;
	unsigned long	bc_bootflags;
	int		timeout;	/* timeout(secs) to enter kernel 
					   file path to boot                  */
	unsigned short	bc_db_flag;
	unsigned char	autoboot;	/* boolean yes/no(true/false)         */
	unsigned char	bc_memrngcnt;
	struct	bootmem bc_memrng[B_MAXMEMR];
	char		bootstring[B_PATHSIZ];	/* OS file path     	      */
	char		sip[B_PATHSIZ];		/* system init program        */
	char		mip[B_PATHSIZ];		/* machine init program       */
	char		bootmsg[B_STRSIZ];	/* Boot message string        */
						/* displayed while OS loads   */
	char		bootprompt[B_STRSIZ];	/* Boot path prompt string    */
	int		bc_argc;
	char		bc_argv[BC_MAXARGS][B_STRSIZ]; /* boot cntl arguments */
};

#endif	/* _SVC_BOOTCNTL_H */
