/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:io/mem/mem.c	1.6"
#ident	"$Header: $"


#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>	/* define before ddi.h */
#include <svc/systm.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/disp.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <proc/mman.h>
#include <mem/vmsystm.h>
#include <mem/as.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <mem/kmem.h>
#include <util/mod/ksym.h>
#include <util/mod/mod_obj.h>
#include <fs/file.h>

#ifdef __STDC__
STATIC int mmrw(dev_t, struct uio *, struct cred *, enum uio_rw);
#else
STATIC int mmrw();
#endif


#define	M_MEM		0	/* /dev/mem - physical main memory */
#define	M_KMEM		1	/* /dev/kmem - virtual kernel memory & I/O */
#define	M_PMEM		3	/* /dev/pmem - any physical memory */


int mmdevflag = 0;

/*
 * Avoid addressing invalid kernel page.  This can happen, for example,
 * if a server process issues a read or write after seeking to a bad address.
 */
extern int memprobe();

/* ARGSUSED */
int
mmopen(devp, flag, type, cr)
        dev_t *devp;
        int flag;
	int type;
        struct cred *cr;
{
        return 0;
}

/* ARGSUSED */
int
mmclose(dev, flag, cr)
        dev_t dev;
        int flag;
        struct cred *cr;
{
        return 0;
}

/* ARGSUSED */
int
mmioctl(dev, cmd, arg, flag, cr, rvalp)
	dev_t dev;
	int cmd;
	int arg;
	int flag;
	struct cred *cr;
	int *rvalp;
{
	struct mioc_rksym rks;
	int error;
	char kname[MAXSYMNMLEN];
	unsigned long kaddr;
	struct file *fp;

	switch(cmd) {
		case MIOC_READKSYM:
		case MIOC_IREADKSYM:
			if(!(flag & FREAD))
				return(EBADF);
			flag = FREAD;
			break;
		case MIOC_WRITEKSYM:
			if(!(flag & FWRITE))
				return(EBADF);
			flag = FWRITE;
			break;
		default:
			return(EINVAL);
	}

	if(getminor(dev) != M_KMEM)
		return(ENXIO);

	if((error = ucopyin((char *) arg,(char *) &rks,sizeof(struct mioc_rksym),0)) != 0)
		return(error);
	

	if((error = copyinstr(rks.mirk_symname, kname, MAXSYMNMLEN, NULL)) != 0) {
		return(error);
	}

	if((error = mod_obj_ioksym(kname, rks.mirk_buf, rks.mirk_buflen, cmd == MIOC_IREADKSYM, flag ))
								!= 0) {
		return(error);
	}
	return(0);
}
	

/* ARGSUSED */
int
mmread(dev, uiop, cr)
        dev_t dev;
        struct uio *uiop;
        struct cred *cr;
{
	return (mmrw(dev, uiop, cr, UIO_READ));
}

/* ARGSUSED */
int
mmwrite(dev, uiop, cr)
	dev_t dev;
        struct uio *uiop;
        struct cred *cr;
{
	return (mmrw(dev, uiop, cr, UIO_WRITE));
}

/* ARGSUSED */
STATIC int
mmrw(dev, uiop, cr, rw)
dev_t dev;
register struct uio *uiop;
struct cred *cr;
enum uio_rw rw;
{
	register off_t off, n;
	register unsigned long po;
	int error = 0;
	caddr_t	 addr;
	u_int	m;

	m = getminor(dev);

        while (error == 0 && uiop->uio_resid != 0) {
		/* It may take long here, so we put in a preemption point */
		PREEMPT(); 
		/*
		 * Don't cross page boundaries.  uio_offset could be
		 * negative, so don't just take a simpler MIN.
		 */
		po = (unsigned long) (off = uiop->uio_offset) % ctob(1);
                n = MIN(MIN(uiop->uio_resid, ctob(1)), ctob(1) - po);

		switch (m) {
			/*
		 	 * Get appropriate addres for /dev/mem (physical),
		 	 * /dev/kmem (virtual), or /dev/pmem (any physical).
		 	 */
			case M_MEM:
			case M_PMEM:
			case M_KMEM:
				if (m == M_KMEM)
					addr = (caddr_t)off;
				else if (m == M_PMEM && btop(off) != 0)
					addr = physmap((paddr_t)off,
							ptob(1),
							KM_SLEEP) + po;
				else
					addr = (caddr_t)xphystokv(off);
				if (memprobe(addr) ||
				    uiomove(addr, n, rw, uiop)) {
	                       		error = ENXIO;
				}
				break;

			default:
				return(ENXIO);
		}
		if (m == M_PMEM && btop(off) != 0)
			physmap_free(addr - po, ptob(1), 0);
	}
	return(error);
}


/*ARGSUSED*/
mmmmap(dev, off, prot)
dev_t dev;
register off_t off;
{
	u_int	m;

	switch (m = getminor(dev)) {

	case M_MEM:
	case M_PMEM:
		if (m == M_PMEM && btop(off) != 0) {
			caddr_t addr = physmap((paddr_t)off, 1, KM_SLEEP);
			if (memprobe(addr)) {
				physmap_free(addr, 1, 0);
				break;
			}
			physmap_free(addr, 1, 0);
		}
		else if (memprobe((caddr_t)xphystokv(off)))
			break;
		return hat_getppfnum((paddr_t)off, PSPACE_MAINSTORE);

	case M_KMEM:
		if (memprobe((caddr_t)(off)))
			break;
		return hat_getkpfnum((caddr_t)off);

	default:
		break;
	}
	return NOPAGE;
}
