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

#ident	"@(#)libiaf:common/lib/libiaf/iaf/retava.c	1.3.1.2"
#ident  "$Header: retava.c 1.2 91/06/25 $"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <iaf.h>

extern void *malloc();

char **
retava(int fd)
{
	struct strioctl strioctl;
	struct iaf *iaf;
	char ** avap;
	char ** temp;
	char *datap;
	int rval;

	/*	see if the AVA module is there...		*/

	if (ioctl(fd, I_FIND, IAFMOD) != 1)
		return(NULL);

	/*	first allocate some space ...			*/

	if ( (iaf = (struct iaf *) malloc(AVASIZ)) == NULL )
		return(NULL);

	iaf->count = 0;
	iaf->size = AVASIZ - 2 * sizeof(int);
	*iaf->data = '\0';

	/*	Get the AVA's from the streams module...	*/

	strioctl.ic_cmd = GETAVA;
	strioctl.ic_timout = INFTIM;
	strioctl.ic_len = sizeof(struct iaf);
	strioctl.ic_dp = (char *) iaf;

	while ( (rval = ioctl(fd, I_STR, &strioctl) ) != 0) {
		/* it had enough space, there's some other problem */
		if (iaf->size >= rval) {
			free(iaf);
			return(NULL);
		}
		/* otherwise allocate more space and try again */
		if ((iaf=(struct iaf *)malloc(rval + 2 * sizeof(int))) == NULL)
			return(NULL);
		iaf->size = rval;
		strioctl.ic_dp = (char *) iaf;
	}

	if ( iaf->count == 0 ) {
		free(iaf);
		return(NULL);
	}

	/* need to allocate 1 extra for NULL entry */

	avap = (char **) malloc((iaf->count + 1) * sizeof(char *));
	if (avap == NULL) {
		free(iaf);
		return(NULL);
	}

	/* fill in the return pointers to the data */

	datap = iaf->data;
	for (temp = avap; iaf->count > 0; iaf->count--) {
		*(temp++) = datap;
		datap += strlen(datap) + 1;
	}
	*(temp) = NULL;

	return(avap);
}
