/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/doscp.c	1.4.1.3"
#ident  "$Header: doscp.c 1.1 91/07/03 $"
/*	@(#) doscp.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 * Copyright (C) Microsoft Corporation, 1983.
 *
 *	doscp - copy files between Unix and MS-DOS
 *
 *	Usage:	1.  doscp unxfile dosfile
 *		2.  doscp dosfile unxfile
 *		3.  doscp dosfile [ dosfile ... ] unxdir
 *		4.  doscp unxfile [ unxfile ... ] unxdir
 *
 *	"-r" flag indicates raw copy, don't map CR during transfers.
 *	"-m" flag indicates forced CR mapping.
 *	Default is to map CR only if a file is printable.
 *
 *	MODIFICATION HISTORY
 *
 *	M000	barrys	Aug 30/84
 *	- A check is now made to ensure that the user has not tried to
 *	  copy directly from one DOS disk to another.
 *	M001	barrys	Sept 19/84
 *	- If the target is a Unix file and the file exists, then the
 *	  file will be truncated when it is opened.
 *	M002	ericc	Dec 7, 1984			16 bit FAT Support
 *	- rewrote the program, allowing lowercase characters to alias
 *	  device names and preventing concurrent access to a DOS disk.
 *	M003	ericc	Feb 13, 1985
 *	- mkcopy() now verifies that every component but the last of a DOS
 *	  path is a DOS directory.
 *	M004	ncm	Mar 28, 1986
 *	- added -m flag to force <cr><lf> mapping to occur, bypassing
 *	  the printable() check, since it is sometimes wrong.
 *	M005	gregj   Oct 15, 1986
 *	- the disk is now open for sync write in cpxd()
 *	M006	buckm	Sep 21, 1987
 *	- add call to setup_perms() to deal with possibly being
 *	  a setuid program.
 *	L007	scol!bijan	Mar 08 1988
 *	- Internationalisation Phase II.
 *	- Include use of 'isprint' and 'isspace' macro.
 */

#include	<sys/types.h>
#include	<unistd.h>
#include	<sys/errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"dosutil.h"
#ifdef	INTL
#include 	<ctype.h>						/*L007*/
#endif

#define		DOSDIR		1	/* return codes from whattarget() */
#define		UNXDIR		2
#define		DOSFILE		4
#define		UNXFILE		8

#define		EXISTS		1	/* return codes from mkcopy() */
#define		NOSPACE		2
#define		NOPATH		4

struct format	 disk;			/* details of current DOS disk */
struct format	*frmp = &disk;
struct dosseg	 seg;			/* locations on current DOS disk */
struct dosseg	*segp = &seg;
char	*buffer  = NULL;		/* buffer for DOS clusters */
char	errbuf[BUFMAX];			/* error message string	*/
char	*fat;				/* FAT of current DOS disk */
char	*f_name  = "";			/* name of this command */
int	bigfat;				/* 16 or 12 bit FAT */
int	dirflag  = TRUE;		/* FALSE if directories not allowed */
int	exitcode = 0;			/* 0 if no error, 1 otherwise */
int	fd;				/* file descr of current DOS disk */
int	flag  = UNKNOWN;		/* check for <cr><lf> conversion M004 */




/* Possible forms of the command line are:
 *
 *	doscp xf dev:path		(copy Unix file to dos file)
 *	doscp xf [...] dev[:dir]	(copy Unix files to dos dir)
 *	doscp dev:path xf		(copy dos file to Unix file)
 *	doscp dev:path [...] xd		(copy dos files to Unix dir)
 *
 * This is all particularly grungy to decode. We look at the
 * last argument given and decide which form to use based on
 * its type.
 */

main(argc,argv)
int	 argc;
char	*argv[];
{
	int ppid;
	int pgid;
	char *doscmd;
	int i, filecount = 0;
	char *c, *name, *temp;
	struct file file[NFILES], *last;	/* file arguments */

	f_name = basename(*argv);
	doscmd = argv[0];
	setup_perms();					/* M006 */

	while (--argc > 0){
		c = *(++argv);				/*	parse      */
		if (*c == '-')				/* command options */
			switch(*(++c)){
			case 'r':
				flag = RAW;		/* M004 */
				break;
			case 'm':			/* M004 */
				flag = MAP;		/* M004 */
				break;			/* M004 */
			default:
				sprintf(errbuf,"unknown option \"-%c\"",*c);
				fatal(errbuf,1);
			}
		else{					/* 	parse     */
			decompose(c,			/* file pathname  */
				  &(file[filecount].unx),
				  &(file[filecount].dos));
#ifdef DEBUG
			fprintf(stderr,"DEBUG xpath = %s\tdospath = %s\n",
				file[filecount].unx,file[filecount].dos);
#endif
			if (filecount++ >= NFILES)
				fatal("too many files",1);
		}
	}
	if ( (ppid = getppid()) == -1) {
		fprintf(stderr,"%s: Could not obtain parent process ID\n",doscmd);
		exit(1);
	}
	if ( (pgid = getsid(ppid)) == -1) {
		fprintf(stderr,"%s: Could not obtain process group ID for parent process\n",doscmd);
		exit(2);
	}
	if (setpgid(getpid(),pgid) == -1) {
		fprintf(stderr,"%s: Could not join process group of parent\n",doscmd);
		exit(3);
	}
	if (filecount < 2)
		usage();

	last = &(file[filecount - 1]);
	switch( whattarget(last) ){

	case DOSDIR:
		/* Check that no files are DOS files and */
		/* Fail if trying to copy DOS file to DOS dir */
		for (i = 0; i < filecount - 1; i++) 
			if (strlen(file[i].dos) != 0) {
				sprintf(errbuf, "Can't copy DOS to DOS");
				fatal(errbuf, 4);
			}
		for ( i = 0; i < filecount - 1; i++){
			temp = basename(file[i].unx);
			name = malloc(strlen(last->dos) + strlen(temp) + 2);
			sprintf(name,"%s%c%s",last->dos,DIRSEP,temp);
			cpxd(file[i].unx,last->unx,name);
		}
		break;

	case UNXDIR:
		for (i = 0; i < filecount - 1; i++){
			temp = lastname(file[i].dos);
			name = malloc(strlen(last->unx) + strlen(temp) + 2);
			sprintf(name,"%s/%s",last->unx,temp);
			cpdx(&file[i],name);
		}
		break;

	case DOSFILE:
		/* Fail if trying to copy DOS file to DOS file */
		if (strlen(file[0].dos) != 0) {
			sprintf(errbuf, "Can't copy DOS to DOS");
			fatal(errbuf, 4);
		}
		if (filecount > 2){
			sprintf(errbuf,"%s:%s not a DOS directory",
							last->unx,last->dos);
			fatal(errbuf,1);
		}
		cpxd(file[0].unx,last->unx,last->dos);
		break;

	case UNXFILE:
		if (filecount > 2){
			sprintf(errbuf,"%s not a directory",last->unx);
			fatal(errbuf,1);
		}
		cpdx(&file[0],last->unx);
 		break;
	}
	exit(exitcode);
}

/*
 *	cpdx()  --  copy from a DOS file to a Unix file.
 */

cpdx(source,dest)
struct file *source;
char *dest;
{
	FILE *out;
	char *dev, dirent[DIRBYTES], *filename;

	dev      = source->unx;
	filename = source->dos;

	zero(dirent,DIRBYTES);

	if (!(dev = setup(dev,O_RDONLY)))
		return;

	if (!seize(fd)){
		sprintf(errbuf,"can't seize %s",dev);
		fatal(errbuf,0);
		close(fd);
		return;
	}
	if (((fat    = malloc(frmp->f_fatsect * BPS))   == NULL) ||
	    ((buffer = malloc(frmp->f_sectclust * BPS)) == NULL)){
		release(fd);
		fatal("no memory for buffers",1);
	}
	if (!readfat(fat)){
		sprintf(errbuf,"FAT not recognizable on %s",dev);
		fatal(errbuf,0);
	}
	else if (*filename == (char) NULL){
		sprintf(errbuf,"can't copy from %s to another Unix file",dev);
		fatal(errbuf,0);
	}
	else if (search(ROOTDIR,filename,dirent) == NOTFOUND){
		sprintf(errbuf,"%s:%s not found",dev,filename);
		fatal(errbuf,0);
	}
	else if (dirent[ATTRIB] == SUBDIR){
		sprintf(errbuf,"%s:%s is a DOS directory",dev,filename);
		fatal(errbuf,0);
	}
	else if ((out = fopen(dest,"w")) == NULL){
		sprintf(errbuf,"can't open %s for writing",dest);
		fatal(errbuf,1);
	}
	else{
		cat( word(&dirent[CLUST]),longword(&dirent[SIZE]),out,flag );
		fclose(out);
	}
	release(fd);
	free(buffer);
	free(fat);
	close(fd);
}




/*	cpover()  --  copy a Unix file over to a DOS file, returning FALSE
 *			if unsuccessful.
 *		parclust : cluster number of immediate parent
 *		name     : name of DOS file, relative to immediate parent
 *		in       : Unix input stream
 *		flag  	 : RAW if no <cr><lf> conversion necessary
 *		 	   MAP if <cr><lf> conversion always required
 *			   UNKNOWN if file should be checked for conversion
 */

cpover(parclust,name,in,flag)
unsigned parclust;
char *name;
FILE *in;
int flag;
{
	long size, time();
	char dirent[DIRBYTES];
	int c, i, temp;
	unsigned clustno, previous, clustsize, start;

	size      = 0;
	start     = NOTFOUND;
	clustno   = FIRSTCLUST;
	clustsize = frmp->f_sectclust * BPS;

	if (flag == UNKNOWN)				/* M004 */
		flag = (printable(in) ? MAP : RAW);	/* M004 */
	rewind(in);

#ifdef DEBUG
	fprintf(stderr,"DEBUG flag = %d\n",flag);
#endif

	if (flag == RAW){				/* M004 */

		while (!feof(in)){
			if ((size += fread(buffer,1,clustsize,in)) == 0)
				break;

			previous = clustno;
			if ((clustno = clustalloc(previous)) == NOTFOUND)
				return(FALSE);

			if (start == NOTFOUND)		/* first cluster */
				start = clustno;
			else				/* other clusters */
				chain(previous,clustno);

			if (!writeclust(clustno,buffer))
				return(FALSE);
		}
	}
	else{	/* (flag == MAP) M004 */
		c = temp = (char) NULL;

		while (c != EOF){

			if ((i = size % clustsize) == 0){
				previous = clustno;
				if ((clustno = clustalloc(previous))
								== NOTFOUND)
					return(FALSE);
								/*  first  */
				if (start == NOTFOUND)		/* cluster */
					start = clustno;
				else{				/* others */
					if (!writeclust(previous,buffer))
						return(FALSE);
					chain(previous,clustno);
				}
			}
			if (temp != (char) NULL){
				*(buffer + i) = temp;
				temp          = (char) NULL;
			}
			else
				switch(c = getc(in)){

				case '\n':	temp          = '\n';
						*(buffer + i) = CR;
						break;
				case EOF:
						*(buffer + i) = DOSEOF;
						break;
				default:
						*(buffer + i) = c;
						break;
				}
			size++;
		}
		if (!writeclust(clustno,buffer))
			return(FALSE);
	}
	if (size == 0)					/* zero length file */
		start = 0;
	else
		marklast(clustno);

	forment(dirent,name,REGFILE,time((long *) 0),start,size);

	temp = TRUE;
	disable();
	if ( !dirfill(parclust,dirent) || !writefat(fat) )
		temp = FALSE;

	enable();
	return(temp);
}


/*	cpxd()  --  copy from a Unix file to a DOS file.
 *		source   : name of Unix file to be copied
 *		dev      : device on which the DOS disk is mounted
 *		dest     : full path name of DOS file to be created
 */

cpxd(source,dev,dest)
char *source, *dev, *dest;
{
	FILE *in;
	unsigned dirclust;
	struct stat statbuf;
	char  dirent[DIRBYTES];

	if ((in = fopen(source,"r")) == (char) NULL){
		sprintf(errbuf,"can't open %s for reading",source);
		fatal(errbuf,0);
		return;
	}
	if (!stat(source,&statbuf) &&
		 ((statbuf.st_mode & S_IFMT) == S_IFBLK)){

		sprintf(errbuf,"both %s and %s are DOS disks",source,dev);
		fatal(errbuf,0);
		return;
	}
	if (!(dev = setup(dev,O_RDWR | O_SYNCW)))	/* M005		*/
		return;

	if (!seize(fd)){
		sprintf(errbuf,"can't seize %s",dev);
		fatal(errbuf,0);
		close(fd);
		return;
	}
	if (((fat    = malloc(frmp->f_fatsect * BPS))   == NULL) ||
	    ((buffer = malloc(frmp->f_sectclust * BPS)) == NULL)){
		release(fd);
		fatal("no memory for buffers",1);
	}
	if (!readfat(fat)){
		sprintf(errbuf,"FAT not recognizable on %s",dev);
		fatal(errbuf,0);
	}
	else if (*dest == (char) NULL){
		sprintf(errbuf,"LOGIC ERROR missing DOS filename for %s",dev);
		fatal(errbuf,1);
	}
	else
		switch( mkcopy(ROOTDIR,dest,in) ){

		case EXISTS:
			sprintf(errbuf,"can't remove %s:%s",dev,dest);
			fatal(errbuf,0);
			break;

		case NOSPACE:
			sprintf(errbuf,"no space on %s",dev);
			fatal(errbuf,0);
			break;

		case NOPATH:
			sprintf(errbuf,"no path to %s on %s",dest,dev);
			fatal(errbuf,0);
			break;
		}
	release(fd);
	free(buffer);
	free(fat);
	fclose(in);
	close(fd);
}



/*	printable(stream)  --  returns TRUE if a Unix file stream is printable.
 *		Only the first cluster's worth of the file is examined.  The
 *		last byte of the file is allowed to be DOSEOF.  This is similar
 *		to canprint().  Printable characters are:
 *				0x07  -  0x0d
 *				0x20  -  0x7e
 *		WARNING:  This function returns without rewinding the stream !!
 */

printable(stream)
FILE *stream;
{
	int c;
	unsigned i;

	rewind(stream);

	for (i = frmp->f_sectclust * BPS; i > 0; i--){

		if ((c = getc(stream)) == EOF)
			return(TRUE);

#ifdef INTL
		if (!isprint(c) && !isspace(c))			       /*L007*/
#else
		if ((c < 0x07) || (c > 0x7e) || 
		    ((c > 0x0d) && (c < 0x20)))
#endif
				return(FALSE);
	}
	return(TRUE);
}



/*	isdosdir(target)  --  determines whether the target is a DOS file
 *		or a DOS directory.  If it is a DOS directory, its starting
 *		cluster is returned; else NOTFOUND is returned.
 */

isdosdir(target)
struct file *target;
{
	unsigned startclust;
	char *dev, dirent[DIRBYTES], *filename;

	dev      = target->unx;
	filename = target->dos;
	zero(dirent,DIRBYTES);

	if (!(dev = setup(dev,O_RDONLY))){
		sprintf(errbuf,"%s not a DOS disk",target->unx);
		fatal(errbuf,1);
	}
	if (((fat    = malloc(frmp->f_fatsect * BPS))   == NULL) ||
	    ((buffer = malloc(frmp->f_sectclust * BPS)) == NULL)){
		fatal("no memory for buffers",1);
	}
	if (readfat(fat) == FALSE){
		sprintf(errbuf,"FAT not recognizable on %s",dev);
		fatal(errbuf,1);
	}
	else if (*filename == (char) NULL)		/* DOS root directory */
		startclust = ROOTDIR;

	else if ((search(ROOTDIR,filename,dirent) == NOTFOUND) ||
		 !(dirent[ATTRIB] & SUBDIR))

		startclust = NOTFOUND;
	else
		startclust = word(&dirent[CLUST]);

	free(buffer);
	free(fat);
	close(fd);
	return(startclust);
}



/*	mkcopy()  --  copy a Unix file stream into a DOS file.
 *		parclust : cluster number of immediate parent
 *		filename : name of DOS file, relative to immediate parent
 *		in       : Unix input stream
 */

mkcopy(parclust,filename,in)
unsigned parclust;
char *filename;
FILE *in;
{
	unsigned exist;
	char attrib, dirent[DIRBYTES], *nextlevel;

	nextlevel = makename(filename,dirent);
	exist     = findent(parclust,dirent);

	if (*nextlevel == (char) NULL){
		attrib = dirent[ATTRIB];
		if (exist != NOTFOUND){
	       	  if ( attrib & (RDONLY | HIDDEN | SYSTEM | VOLUME | SUBDIR))
				return(EXISTS);
#ifdef DEBUG
			fprintf(stderr,"DEBUG unlinking %s\n",filename);
#endif
			dosunlink(parclust,dirent);
		}
		if (!cpover(parclust,filename,in,flag))
			return(NOSPACE);

		return(0);
	}
	else if ((exist == NOTFOUND) || !(dirent[ATTRIB] & SUBDIR))
		return(NOPATH);
	else
		return( mkcopy(word(&dirent[CLUST]),nextlevel,in) );
}




usage()
{
	fprintf(stderr,"Usage: %s [-rm] device:path  . . .  device:path\n",
									f_name);
	exit(1);
}



/*	whattarget()  --  returns DOSDIR, UNXDIR, DOSFILE or UNXFILE,
 *		target    : the file in question
 */

whattarget(target)
struct file *target;
{
	struct stat statbuf;

	if (*(target->dos) == (char) NULL){

		if (stat(target->unx,&statbuf) != 0)
			return(UNXFILE);
		else
			switch(statbuf.st_mode & S_IFMT){

			case S_IFBLK:
			case S_IFCHR:
				return(DOSDIR);
			case S_IFDIR:
				return(UNXDIR);
			case S_IFREG:
				return(UNXFILE);
			default:
				sprintf(errbuf,"bad file %s",target->unx);
				fatal(errbuf,1);
			}
	}
	else{
		if (isdosdir(target) == NOTFOUND)
			return(DOSFILE);
		else
			return(DOSDIR);
	}
}
