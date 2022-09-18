/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*      Copyright (c) 1989 AT&T */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T     */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */

#ident	"@(#)libiaf:common/lib/libiaf/iaf/setava.c	1.3.1.2"
#ident  "$Header: setava.c 1.2 91/06/25 $"
/*
 *	IAF routine to  send AVA list to IAF STREAMS module
 */
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/conf.h>
#include <unistd.h>
#include <iaf.h>

int
setava(int fd, char **argv)
{
	struct strioctl strioctl;
	struct iaf *iaf;
	unsigned int size = AVASIZ;
	int len, rval;
	char *bufp,	/* current data pointer	*/
	     *endp;	/* end of data pointer	*/
	char modname[FMNAMESZ+1]; 

	/*	NULL pointer means pop the module ...	*/
	/*	if is on the top of the STREAM.		*/

	if (argv == NULL) {
		/* if the module is not there, its OK	*/
		if (ioctl(fd, I_FIND, IAFMOD) == 0)
			return(0);
		if (ioctl(fd, I_LOOK, &modname) == 0)
			if (strcmp(IAFMOD, modname) == 0)
				if (ioctl(fd, I_POP, 0) == 0)
					return(0);
		return(-1);
	}
		
	/*	make sure the AVA module is there ...	*/

	if (ioctl(fd, I_FIND, IAFMOD) != 1)
		if (ioctl(fd, I_PUSH, IAFMOD) < 0)
			return(-1);

	/*	get some space to start with ...	*/

	if ( (iaf = (struct iaf *)malloc(size)) == NULL )
		return(-1);

	iaf->count = 0;
	iaf->size = 0;
	*iaf->data = '\0';
	bufp = iaf->data;
	endp = (char *) iaf + size;

	while (*argv) {
		len = strlen(*argv) + 1;
		if ( bufp + len > endp ) {
			size += AVASIZ;
			if ( (iaf = (struct iaf *)realloc(iaf, size)) == NULL )
				return(-1);
			bufp = iaf->data + iaf->size;
			endp = (char *) iaf + size;
			continue;
		}
		iaf->count++;
		iaf->size += len;
		(void) strcpy(bufp, *argv++);
		bufp = iaf->data + iaf->size;
	}

	strioctl.ic_cmd = SETAVA;
	strioctl.ic_timout = INFTIM;
	strioctl.ic_len = 2 * sizeof(int) + iaf->size * sizeof(char);
	strioctl.ic_dp = (char *) iaf;

	rval = ioctl(fd, I_STR, &strioctl);
	free(iaf);
	return(rval);
}
