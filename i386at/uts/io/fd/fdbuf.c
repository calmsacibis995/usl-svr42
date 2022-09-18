/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/fd/fdbuf.c	1.5"
#ident	"$Header: $"


#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <mem/immu.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <svc/sysenvmt.h>

#include <util/cmn_err.h>
#include <io/dma.h>
#include <io/i8237A.h>
#include <io/cram/cram.h>
#include <io/fd/fd.h>

struct  fdbufstruct     fdbufstruct;
struct	fdstate		fd[NUMDRV];

int	fdloaded = 0;

fdbufinit()
{
        /*
         * If fdloaded is 1 it means that fdinit() has initialized
         * the device, so don't reset the controller.
         */
        if(!fdloaded)   {
		unsigned char	enabint;
		register int	oldpri, i;

		/* There is a small difference between the AT and the MCA controllers */
		if( sysenvmtp->machflags & MC_BUS )
			enabint = ENAB_MCA_INT;
	        else 
			enabint = ENABINT;

		oldpri = splhi();
		outb(FDCTRL, enabint);
		for(i = 0; i < 5; i++)
			tenmicrosec();
		outb(FDCTRL, enabint|NORESET);
		splx(oldpri);

		dma_init();
	}

	fdbufgrow(FDMEMSIZE, 1);
}

fdbufgrow (bytesize, sleepflag)
unsigned    bytesize;
int         sleepflag;
{
	register    unsigned    int     pageon;
	register    unsigned    int     dmapage;
	register    unsigned    int     pageswanted;
	register    unsigned    int     pagesrequested;
	register    unsigned    int     curpages;
	register    caddr_t     newbuf;
	register    unsigned    int     oldpfn;
	register    unsigned    int     newpfn;

	/* Grow or Shrink a buffer to hold  bytesize  bytes  without */
	/* crossing a dma boundary.                                  */

	/* We can't handle more than 64k, since this has to cross  a */
	/* dma boundary.                                             */

	if (bytesize > 0x10000)
	    return (EINVAL);

	/* Compute the number of pages  we  want,  as  well  as  the */
	/* number we already have, and the page frame number of the  */
	/* current buffer.                                           */

	pageswanted = btopr (bytesize);
	curpages = btopr (fdbufstruct.fbs_size);

	/* If we already have enough pages,  then  free  any  excess */
	/* pages, and update the new size.                           */

	if (pageswanted <= curpages) {
	    if(curpages == 0)	{
		return(0);
	    }
	    oldpfn = kvtopfn (fdbufstruct.fbs_addr);
	    for (pageon = pageswanted; pageon < curpages; pageon++)
		freepage (oldpfn + pageon);
	    fdbufstruct.fbs_size = ptob (pageswanted);
	    return (0);
	    }

	/* The number of pages  necessary  to  insure  that  we  get */
	/* pageswanted  pages that do not cross a dma boundary is (2 */
	/* * pageswanted - 1), as the  worst  case  is  if  the  dma */
	/* boundary  is  at  (pagewanted  -  1)  and  then  we  need */
	/* (pagewanted) pages.                                       */

	pagesrequested = 2 * pageswanted - 1;

	/* Get the contiguous pages, if we can't, print as error and */
	/* return.                                                   */

	newbuf = (caddr_t) getcpages (pagesrequested, sleepflag);
	if (newbuf == NULL) {
	    printf ("Unable to allocate memory for floppy raw buffer\n");
	    return (EAGAIN);
	    }

	newpfn = kvtopfn (newbuf);

	/* Find the spot of the dma boundary.  dmapage will  be  set */
	/* at  the  location  where the next dma boundary begins.    */

	dmapage = btopr (0x10000 - (kvtophys (newbuf) & 0xffff));

	/* Check  whether  we  found  a  boundary   in   the   first */
	/* pageswanted pages.                                        */

	if (dmapage < pageswanted) {

	    /* We found a dma  boundary  in  the  first  pageswanted */
	    /* pages.   We  will  use the pageswanted pages starting */
	    /* from the dma boundary.                                */

	    /* Free the pages that were prior to the dma boundary.   */

	    for (pageon = 0; pageon < dmapage; pageon++)
		freepage (newpfn + pageon);

	    /* Free the pages after the area we are reserving.       */

	    for (pageon = dmapage + pageswanted; pageon < pagesrequested;
	      pageon++)
		freepage (newpfn + pageon);

	    /* Point buffer at area that begins at the dma page      */

	    newbuf += ptob (dmapage);
	    }

	else {
	    /* We  did  not  find  a  dma  boundary  in  the   first */
	    /* pageswanted pages, so we will use that area.          */

	    /* Free the subsequent pages.                            */

	    for (pageon = pageswanted; pageon < pagesrequested; pageon++)
		freepage (newpfn + pageon);
	    }

	/* Free the old area that had been used.                     */

	if (curpages != 0) {
	    oldpfn = kvtopfn (fdbufstruct.fbs_addr);
	    for (pageon = 0; pageon < curpages; pageon++)
		freepage (oldpfn + pageon);
	    }

	/* Set the new area that will be used.                       */

	fdbufstruct.fbs_addr = newbuf;
	fdbufstruct.fbs_size = ptob (pageswanted);
	return (0);
}

#ifdef MERGE386
int
unix_has_floppy()
{
	register struct fdstate *f;
	int status = FALSE;
	int unit;

	if(!fdloaded)	{
		return(FALSE);
	}

	for( unit = 0; unit < NUMDRV ; unit++ ) {
		f = &fd[unit];
		if ( f->fd_status & (OPENED|OPENING|CLOSING) )
			status = TRUE;
	}
	return(status);
}
#endif /* MERGE386 */
