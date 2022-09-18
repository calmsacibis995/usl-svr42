/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosutil.h	1.2.1.3"
#ident  "$Header: dosutil.h 1.1 91/07/03 $"
/*	@(#) dosutil.h 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 * Copyright (C) Microsoft Corporation, 1983
 */

/*
 * dosutil.h - definitions for MS-DOS File Transfer Utilities
 *
 *	created Aug 1983  -- David Basin
 *
 *	MODIFICATION HISTORY
 *	M000	Dec 6, 1984	ericc			16 bit FAT Support
 *	- Rewrote the programs, using semaphores to prevent concurrent
 *	  access to a given DOS disk.
 *	M001	Oct 22, 1986	greg
 *	- Added hiddensectors[] to structure bootsector
 */


#include	<string.h>


#define		TRUE		1
#define		FALSE		0

#define		DEFAULT		"/etc/default/msdos"
#define		SPOOL		"/tmp"

#define		BUFMAX		133	/* length of temporary buffers */
#define		NFILES		128	/* maximum number of files processed */
#define		NSEMAP		20	/* max number of opened semaphores */
#define		NTRYWAIT	10	/* number of times to try nbwaitsem() */
#define		WAITTIME	10	/* waiting time for nbwaitsem() */

#define		BPS		512	/* bytes per disk sector */

#define		CR		0x0d	/* carriage return character */
#define		DIRSEP		'/'	/* directory separator in DOS path */
#define		DOSEOF		0x1a	/* CONTROL-Z character */
#define		EXTSEP		'.'	/* extension separator in DOS name */

#define		ROOTDIR		0	/* root directory's cluster */
#define		FIRSTCLUST	2	/* first data cluster on a disk */
#define		NOTFOUND	0xffff	/* cluster not found indicator */

#define		DIRBYTES	32	/* bytes per DOS directory entry */
#define		EXTBYTES	3	/* length of DOS file name extension */
#define		NAMEBYTES	8	/* maximum length of DOS file name */

#define		NAME		0	/* offsets into a DOS directory entry */
#define		EXT		8
#define		ATTRIB		11
#define		TIME		22
#define		DATE		24
#define		CLUST		26
#define		SIZE		28

#define		REGFILE		0x00	/* DOS file attributes */
#define		RDONLY		0x01
#define		HIDDEN		0x02
#define		SYSTEM		0x04
#define		VOLUME		0x08
#define		SUBDIR		0x10
#define		ARCHIVE		0x20

#define		DIREND		0x00	/* DOS filename status */
#define		WASUSED		0xe5	/* (first byte of directory entry) */
#define		DIRECT		0x2e

#define		MAP		0	/* force <cr><lf> mapping   M004 */
#define		RAW		1	/* prevent <cr><lf> mapping M004 */
#define		UNKNOWN		2	/* map if file is printable M004 */

#define		basename(path)	child(path,'/')
#define		blank(str,n)	fill(str,' ',n)
#define		lastname(path)	child(path,DIRSEP)
#define		min(x,y)	(((x) < (y)) ? (x) : (y))
#define		zero(str,n)	fill(str,NULL,n);

#define		longword(c)	* (long *) c
#define		word(c)		* (unsigned short *) c


struct bootsector{			/* format of first sector on DOS disk */
	char	jump[3];
	char	oem[8];
	char	bytespersector[2];
	char	sectorspercluster;
	char	reservedsectors[2];
	char	numberofFATs;
	char	rootdirmax[2];		/* maximum entries in root directory  */
	char	totalsectors[2];	/* total sectors in logical image     */
	char	mediadescriptor;
	char	sectorsperFAT[2];
	char	sectorspertrack[2];
	char	numberofheads[2];
	char	hiddensectors[2];	/* M001 */
	unsigned	newtotalsectors;	/* > 65K partition support */
};

struct dosseg {				/* positions of things on DOS disk */
	unsigned s_fat;			/*	sector where FAT begins */
	unsigned s_dir;			/*	    "	root directory */
	unsigned s_data;		/*	    "   data */
};

struct file {				/* table of file arguments */
	char	*unx;			/*	Unix part of path */
	char	*dos;			/*	DOS part of path */
};

struct format {				/* details of a DOS disk */
	unsigned f_fatsect;		/*	sectors per FAT	*/
	unsigned f_dirsect;		/*	sectors for root directory */
	unsigned f_sectclust;		/*	sectors per cluster */
	unsigned f_sectors;		/*	sectors per track */
	unsigned f_clusters;		/* 	max addressable cluster */
	unsigned f_media;		/*	media byte */
	char	*f_device;		/*	device name */
};


extern char *buffer;			/* buffer for DOS clusters */
extern char errbuf[];			/* buffer for complaints */
extern struct format *frmp;		/* details of current DOS disk */
extern struct dosseg *segp;		/* positions on current DOS disk*/


extern char
	*child(),
	*forment(),
	*guessdisk(),
	*makename(),
	*parent(),
	*setup();


extern unsigned int
	findent(),
	nextclust(),
	search();

extern char *malloc();
extern long time();
