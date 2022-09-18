/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/unlink.c	1.1.1.2"
#ident  "$Header: unlink.c 1.1 91/07/03 $"
/*	@(#) unlink.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 *	MODIFICATION hISTORY
 *	M000	ericc	Feb 13, 1985
 *	- The root directory may contain a volume label, which is a directory
 *	  entry with the attribute byte set to VOLUME.  dirclear() skips
 *	  over such an entry.
 */

#include	<stdio.h>
#include	"dosutil.h"


extern char	*fat;


/*	dirclear()  --  erase a directory entry.
 *		dirclust :  cluster containing the directory entry
 *		entry    :  directory entry to be erased
 */

dirclear(dirclust,entry)
unsigned dirclust;
char entry[];
{
	unsigned i;
	char *bufend, *j;

	if (dirclust == ROOTDIR){
		for (i = 0; i < frmp->f_dirsect; i++){
			readsect(segp->s_dir + i,buffer);
			for (j = buffer; j < buffer + BPS; j += DIRBYTES){
#ifdef DEBUG
/*				fprintf(stderr,"DEBUG dirclear %.11s\n",j);
 */
#endif
				if ( !strncmp(&entry[NAME], &j[NAME],
							NAMEBYTES+EXTBYTES) &&
				     !(j[ATTRIB] & VOLUME)){
					j[NAME] = WASUSED;
					writesect(segp->s_dir + i,buffer);
					return;
				}
			}
		}
	}
	else{
		bufend = buffer + (frmp->f_sectclust * BPS);
		readclust(dirclust,buffer);

		for (j = buffer; j < bufend; j += DIRBYTES){
#ifdef DEBUG
/*				fprintf(stderr,"DEBUG dirclear %.11s\n",j);
 */
#endif
			if (!strncmp(&entry[NAME],&j[NAME],NAMEBYTES+EXTBYTES)){
				j[NAME] = WASUSED;
				writeclust(dirclust,buffer);
				return;
			}
		}
	}
	sprintf(errbuf,"LOGIC ERROR %.11s not found in cluster %u",
							&entry[NAME],dirclust);
	fatal(errbuf,1);
}



/*	dosunlink()  --  erase a DOS file from a DOS disk.
 *		dirclust :  cluster containing the directory entry
 *		dirent   :  directory entry of the DOS file
 */

dosunlink(dirclust,dirent)
unsigned dirclust;
char dirent[];
{
	unsigned dataclust = word(&dirent[CLUST]);

	while ( goodclust(dataclust) ){

#ifdef DEBUG
		fprintf(stderr,"DEBUG freeing cluster %u\n",dataclust);
#endif
		dataclust = clearclust(dataclust);
	}
	disable();
	dirclear(dirclust,dirent);
	writefat(fat);
	enable();
}
