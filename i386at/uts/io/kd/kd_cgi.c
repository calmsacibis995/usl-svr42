/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/kd/kd_cgi.c	1.4"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <proc/proc.h>	
#include <proc/signal.h>
#include <svc/errno.h>
#include <proc/user.h>
#include <util/inline.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <io/ws/vt.h>
#include <io/ansi/at_ansi.h>
#include <io/uio.h>
#include <io/kd/kd.h>
#include <io/xque/xque.h>
#include <io/stream.h>
#include <io/termios.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <io/ws/ws.h>
#include <io/ws/chan.h>
#include <io/gvid/vid.h>
#include <io/gvid/vdc.h>
#include <proc/cred.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/seg_objs.h>
#include <proc/mman.h>
#include <io/ddi.h>
#include <io/kd/kd_cgi.h>

extern wstation_t  Kdws;

cgi_mapclass(chp, arg, rvalp)
int     *rvalp;
{
	int i, rv = 0;
	extern struct cgi_class cgi_classlist[];
	struct cgi_class *vcp;
#define MAXCLN	64
	char name[MAXCLN];
	faddr_t cgi_umapinit();

	for(i=0; i<MAXCLN; i++)
	{
		if(0 == (name[i] = fubyte(arg++)))
			break;
		if(-1 == name[i])
		{
			return(EFAULT);
		}
	}
	if(MAXCLN==i)
	{			/* name is garbage */
		return(EINVAL);
	}

	for(vcp=cgi_classlist; vcp->name; vcp++)
		if(!strcmp(name, vcp->name))		/* S018 */
			break;

	if(!vcp->name)
	{			/* name is not found */
		return(ENXIO);
	}

	if((*rvalp = (int) cgi_umapinit(vcp->base, vcp->size)) != (int)NULL)
		return(cgi_ioprivl(1, vcp->ports));
	return(EIO);
}

faddr_t
cgi_umapinit(chp, base, size)
channel_t	*chp;
{
 	struct kd_memloc	memloc;
	struct proc	*procp;
	faddr_t vaddr;
	struct map_info *map_p = &Kdws.w_map;

        if (chp != (channel_t *)ws_activechan(&Kdws))
		return(NULL);

	drv_getparm(UPROCP, (ulong*)&procp);
	if (map_p->m_procp && map_p->m_procp != procp ||
					 map_p->m_cnt == CH_MAPMX)
		return(NULL);

	map_addr(&vaddr, size, (off_t)0, 1);
	if(vaddr == NULL)
		return(vaddr);

	memloc.vaddr = vaddr;
	memloc.physaddr = (caddr_t)base;
	memloc.length = size;
	memloc.ioflg = 0;

	ws_mapavail(chp, map_p);
	
	if (!kdvm_map(procp, chp, map_p, &memloc))
	{
		(void) as_unmap(procp->p_as, vaddr, size);
		return(NULL);
	}
	return(vaddr);
}

/*
 * Grant or revoke permission to do
 * direct OUTs from user space.
 *
 */

cgi_ioprivl(arg, ports)
struct portrange *ports;
{
	short maxport, curport, ioports[2];	/* keep it simple for now */
	extern int enableio(), disableio();
	
	ioports[1] = 0;		/* delimit the (very short) list */
	for ( ; ports->count; ports++) {

		maxport = ports->first + ports->count;
		if (maxport > MAXTSSIOADDR) {
			return(EIO);
		}

		for (curport = ports->first; curport < maxport; curport++) {
			ioports[0] = curport;
			arg ?  enableio(ioports) : disableio(ioports);
		}
	}
	return 0;
}


/* End Enhanced Application Compatibility Support */
