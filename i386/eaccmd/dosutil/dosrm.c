/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosrm.c	1.3.1.2"
#ident  "$Header: dosrm.c 1.1 91/07/03 $"
/*	@(#) dosrm.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 * Copyright (C) Microsoft Corporation, 1983.
 *
 *	This program can be invoked either as
 *		dosrm    --  remove DOS files
 *		dosrmdir --  remove empty DOS directories
 *
 *	NOTE:   DOS directories are not compacted; ie. after a file is removed,
 *		no attempt is made to free a cluster from its immediate parent
 *		directory, even if this were possible.
 *
 *	MODIFICATION HISTORY
 *	M000	Dec 7, 1984	ericc			16 bit FAT Support
 *	- rewrote the program, allowing lowercase characters to alias
 *	  device names and preventing concurrent access to a DOS disk.
 *	M001	Oct 15, 1986
 *	- open the device for sync writes
 *	M002	buckm	Sep 21, 1987
 *	- add call to setup_perms() to deal with possibly being
 *	  a setuid program.
 */

#include	<sys/types.h>
#include	<unistd.h>
#include	<sys/errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	"dosutil.h"
#undef		remove

#define		DOSRMDIR	"dosrmdir"

#define		NOPRIV		1	/* return codes from scomp_remove() */
#define		NOTFILE		2
#define		NOTEMPTY	3
#define		NOTDIR		4


struct format	 disk;			/* details of current DOS disk */
struct format	*frmp = &disk;
struct dosseg	 seg;			/* locations on current DOS disk */
struct dosseg	*segp = &seg;
char	*buffer  = NULL;		/* buffer for DOS clusters */
char	 errbuf[BUFMAX];		/* error message string	*/
char	*fat;				/* FAT of current DOS disk */
char	*f_name  = "";			/* name of this command */
int	bigfat;				/* 16 or 12 bit FAT */
int	dirflag  = TRUE;		/* FALSE if directories not allowed */
int	dirrem   = FALSE;		/* TRUE if directory removal enabled */
int	exitcode = 0;			/* 0 if no error, 1 otherwise */
int	fd;				/* file descr of current DOS disk */




main(argc,argv)
int	 argc;
char	*argv[];
{
	int ppid;
	int pgid;
	char *doscmd;
	int i, filecount = 0;			/* number of file arguments */
	struct file file[NFILES];		/* file arguments */

	f_name   = basename(*argv);
	doscmd = argv[0];
	dirrem   = strcmp(f_name,DOSRMDIR ) ? FALSE : TRUE;
	setup_perms();						/* M002 */

	while (--argc > 0){
		decompose(*(++argv),
				&(file[filecount].unx),
				&(file[filecount].dos));
#ifdef DEBUG
		fprintf(stderr,"DEBUG xpath = %s\tdospath = %s\n",
			file[filecount].unx,file[filecount].dos);
#endif
		if (filecount++ >= NFILES)
			fatal("too many files",1);
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
	if (filecount <= 0)
		usage();

	for (i = 0; i < filecount; i++){
		doit(file[i].unx,file[i].dos);
	}
	exit(exitcode);
}


doit(dev,filename)
char *dev,*filename;
{
	int result;
	unsigned dirclust;
	char dirent[DIRBYTES];

	zero(dirent,DIRBYTES);

	if (!(dev = setup(dev,O_RDWR | O_SYNCW)))
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
		sprintf(errbuf,"can't remove root directory on %s",dev);
		fatal(errbuf,0);
	}
	else if ((dirclust = search(ROOTDIR,filename,dirent)) == NOTFOUND){
		sprintf(errbuf,"%s:%s not found",dev,filename);
		fatal(errbuf,0);
	}
	else{
		switch( scomp_remove(dirclust,dirent) ){

		case NOPRIV:
			sprintf(errbuf,"can't remove %s:%s",dev,filename);
			fatal(errbuf,0);
			break;
		case NOTFILE:
			sprintf(errbuf,"%s:%s is a directory",dev,filename);
			fatal(errbuf,0);
			break;
		case NOTEMPTY:
			sprintf(errbuf,
				"directory %s:%s not empty",dev,filename);
			fatal(errbuf,0);
			break;
		case NOTDIR:
			sprintf(errbuf,"%s:%s isn't a directory",dev,filename);
			fatal(errbuf,0);
			break;
		}
	}
	release(fd);
	free(buffer);
	free(fat);
	close(fd);
}


/*	isempty()  --  returns TRUE if the directory is empty.  NOTE:  This
 *		routine refuses to examine the root directory !!
 *		dirclust :  starting cluster of the directory to examine
 */

isempty(dirclust)
unsigned dirclust;
{
	char *bufend, *j;

	if (dirclust == ROOTDIR)
		fatal("LOGIC ERROR isempty(root directory) !!",1);

	bufend = buffer + (frmp->f_sectclust * BPS);

	while (goodclust(dirclust)){
		if (!readclust(dirclust,buffer)){
			sprintf(errbuf,"can't read cluster %d",dirclust);
			fatal(errbuf,1);
		}
		for (j = buffer; j < bufend; j += DIRBYTES){
#ifdef DEBUG
/*				fprintf(stderr,"DEBUG isempty %.11s\n",j);
 */
#endif
			if (*j == DIREND)
				return(TRUE);
			if (((*j & 0xff) != WASUSED) && (*j != DIRECT)){
				return(FALSE);
			}
		}
		dirclust = nextclust(dirclust);
	}
	return(TRUE);
}


/*	scomp_remove(dirclust,dirent)  --  verify the situation, then remove a file.
 *		dirclust : cluster containing the directory entry
 *		dirent   : directory entry of the file
 */

scomp_remove(dirclust,dirent)
unsigned dirclust;
char dirent[];
{
	char attrib;
	unsigned start;

	attrib = dirent[ATTRIB];
	start  = word(&dirent[CLUST]);

	if (attrib & (RDONLY | HIDDEN | SYSTEM | VOLUME)){

#ifdef DEBUG
		fprintf(stderr,"%.8s %.3s\t attribute %.2x\n",
				&dirent[NAME],&dirent[EXT],attrib);
#endif
		return(NOPRIV);
	}
	if (attrib & SUBDIR){
		if (!dirrem)
			return(NOTFILE);

		if (!isempty(start))
			return(NOTEMPTY);
	}
	else if (dirrem)
		return(NOTDIR);

	dosunlink(dirclust,dirent);
	return(0);
}



usage()
{
	fprintf(stderr,"Usage: %s device:path  . . .\n",f_name);
	exit(1);
}
