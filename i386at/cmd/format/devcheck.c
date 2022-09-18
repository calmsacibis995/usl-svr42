/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)format:i386at/cmd/format/devcheck.c	1.3"
#ident  "$Header: devcheck.c 1.1 91/08/08 $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mkdev.h>

/* Exit Codes */
#define	RET_OK		0	/* success */
#define	RET_USAGE	1	/* usage */
#define	RET_STAT	2	/* can't stat device */
#define	RET_OPEN	3	/* can't open device */
#define	RET_CHAR	4	/* device not character device */
#define	RET_FLOPPY	5	/* device not a floppy disk */
#define	RET_FORM0	6	/* formatted 0 tracks */

/*
 *
 * dev_check()
 *
 *	This is a useful entry to place machine dependent device checks ...
 *
 * parameter: none
 *
 * return values: none
 *
 * exit values:
 *
 *	RET_STAT	if stat failes
 *
 */
void
dev_check()
{
	struct stat statbuf;
	extern char *device;

	if ( stat(device, &statbuf) == -1 ) {
		fprintf(stderr,"format: dev_check(): Couldn't stat %s.\n",device);
		exit(RET_STAT);
	}
	
	/* Previously PARTITION macro used, due to DDI put in user lev vers. */
	if ( ((minor(statbuf.st_rdev) & 0x06) >> 1) == 2 ) {
		fprintf(stderr,"Warning: Using this device, the first cylinder \n");
		fprintf(stderr,"of the floppy will not be formatted. \n");
	}
}
