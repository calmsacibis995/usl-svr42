/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:util/mod/mod_intr.c	1.4"
#ident "$Header: $"

#include	<util/debug.h>
#include	<util/types.h>
#include	<util/param.h>
#include	<mem/kmem.h>
#include	<svc/eisa.h>
#include	<svc/errno.h>
#include	<util/cmn_err.h>
#include	<util/mod/mod_k.h>
#include	<util/mod/moddrv.h>
#include	<util/mod/mod_intr.h>

extern	void	intnull();
extern	void	(*ivect[])();
extern	unsigned char	intpri[];
extern	struct	mod_shr_v	*mod_shr_ivect[];
extern	char	mod_iv_locks[];

extern	void	*kmem_zalloc();
extern	void	kmem_free();
extern	int	nenableint();
extern	int	ndisableint();

/*
 * Shared interrupt routine called through ivect[].
 * Supports the dynamic addition and deletion of
 * shared interrupts, required by the dynamically
 * loadable module feature.
 */
void
mod_shr_intn(int iv)
{
	struct	mod_shr_v	*svp;
	register void	(**sihp)();

	svp = mod_shr_ivect[iv];

	while(svp)	{
		sihp = svp->msv_sih;

		while(*sihp)	{
			if(*sihp != intnull)	{
				(**sihp)(iv);
			}
			sihp++;
		}
		svp = svp->msv_next;
	}
}

/*
 * Install and enable all interrupts required by a
 * given device driver.
 */
void
mod_drvattach(struct mod_drvintr *aip)
{
	register struct	intr_info	*i_infop;
	void	(*hndlr)();

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_drvattach()"));

	if(!aip || !(i_infop = aip->drv_intrinfo))	{
		return;
	}

	moddebug(cmn_err(CE_NOTE, "!MOD:    Interrupts:"));

	hndlr = aip->ihndler;

	while(i_infop->ivect_no >= 0)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:      %d", i_infop->ivect_no));
		mod_add_intr(i_infop, hndlr);
		i_infop++;
	}

	return;
}

/*
 * Remove and disable all interrupts used by a given device driver.
 */
void
mod_drvdetach(struct mod_drvintr *aip)
{
	register struct	intr_info	*i_infop;
	void	(*hndlr)();

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_drvdetach()"));

	if(!aip || !(i_infop = aip->drv_intrinfo))	{
		return;
	}

	moddebug(cmn_err(CE_NOTE, "!MOD:    Interrupts:"));

	hndlr = aip->ihndler;

	while(i_infop->ivect_no >= 0)	{
		moddebug(cmn_err(CE_NOTE, "!MOD:      %d", i_infop->ivect_no));
		mod_remove_intr(i_infop, hndlr);
		i_infop++;
	}

	return;
}

#define	LOCKED		1
#define	SLEEPING	2

#define	IV_LOCK(i, p)	{ \
	p = &mod_iv_locks[i]; \
	while(*p & LOCKED)	{ \
		moddebug(cmn_err(CE_NOTE, "!MOD: iv %d locked, sleeping", i)); \
		*p |= SLEEPING; \
		sleep(p, PZERO); \
	} \
	moddebug(cmn_err(CE_NOTE, "!MOD: locking iv %d", i)); \
	*p |= LOCKED; \
}

#define	IV_UNLOCK(i, p)	{ moddebug(cmn_err(CE_NOTE, "!MOD: unlocking iv %d", i)); \
			  *(p) &= ~LOCKED; if(*(p) & SLEEPING) { *(p)&=~SLEEPING; wakeup(p);} }

/*
 * Add the interrupt handler ihp to the vector defined by iip.
 */
int
mod_add_intr(struct intr_info *iip, void (*ihp)())
{
	char	*lockp;
	int	iv;
	void	(**sihp)(), (**ivp)();
	struct	mod_shr_v	*sivp, **sivpp;

	iv = iip->ivect_no;
	ivp = &ivect[iv];

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_add_intr(): %d", iv));

	IV_LOCK(iv, lockp);

	if(*ivp == intnull)	{
		/*
		 * If we get here, the interrupt vector is not
		 * currently used. We assume the interrupt is disabled.
		 */
		intpri[iv] = iip->int_pri;
		*ivp = ihp;
		if(iip->itype == 4)	{
			set_elt(iv, LEVEL_TRIG);
		}
		nenableint(iv);	/* enable the interrupt */
		IV_UNLOCK(iv, lockp);
		return(0);
	}
	else if(*ivp != mod_shr_intn)	{
		/*
		 * The interrupt is shared for the first time.
		 *
		 * Allocate memory for the first packet, assign the 2 interrupt
		 * handlers to it, and update the packet's count.
		 */
		sivp = (struct mod_shr_v *)kmem_zalloc(sizeof(struct mod_shr_v), KM_SLEEP);
		sivp->msv_sih[0] = *ivp;
		sivp->msv_sih[1] = ihp;
		sivp->msv_cnt = 2;
		mod_shr_ivect[iv] = sivp;

		/*
		 * ivect[] now points to the shared interrupt routine.
		 */
		ndisableint(iv);
		*ivp = mod_shr_intn;
		nenableint(iv);
		IV_UNLOCK(iv, lockp);
		return(0);
	}

	/*
	 * If we get here, the interrupt vector is shared.
	 * We assume the interrupt is currently enabled.
	 */

	sivpp = &mod_shr_ivect[iv];

	/*
	 * Find an empty slot in the list for the new handler.
	 */
	while(1)	{
		if((sivp = *sivpp) != NULL)	{
			if(sivp->msv_cnt == MOD_NSI)	{
				/*
				 * List is not full, we have our slot.
				 */
				sivpp = &(sivp->msv_next);
				continue;
			}
			break;
		}
		else	{
			/*
			 * No slots free, allocate a new packet.
			 */
			sivp = (struct mod_shr_v *)kmem_zalloc(sizeof(struct mod_shr_v), KM_SLEEP);
			break;
		}
	}

	/*
	 * Disable the interrupt while we add the new handler.
	 */
	ndisableint(iv);

	/*
	 * Add the newly allocated packet to the chain.
	 */
	if(*sivpp == NULL)	{
		*sivpp = sivp;
	}

	/*
	 * At this point sivp points to a packet with at least
	 * one empty slot.
	 */

	sihp = sivp->msv_sih;

	/*
	 * Find the empty slot in the packet.
	 */
	while(*sihp != NULL && *sihp != intnull)	{
		sihp++;
	}

	/*
	 * Assign the new handler to the slot and
	 * increment the packet's count.
	 */
	*sihp = ihp;
	sivp->msv_cnt++;

	/*
	 * We're done, re-enable and free the interrupt vector.
	 */
	nenableint(iv);
	IV_UNLOCK(iv, lockp);
	return(0);
}

/*
 * Remove the interrupt handler ihp from the vector defined by iip.
 */
int
mod_remove_intr(struct intr_info *iip, void (*ihp)())
{
	char	*lockp;
	int	iv;
	void	(**sihp)(), (**ivp)();
	struct	mod_shr_v	*sivp, **sivpp;

	iv = iip->ivect_no;
	ivp = &ivect[iv];

	moddebug(cmn_err(CE_NOTE, "!MOD: mod_remove_intr(): %d", iv));

	IV_LOCK(iv, lockp);

	if(*ivp != mod_shr_intn)	{
		/*
		 * The interrupt is not shared,
		 * disable the interrupt and reset ivect[].
		 */
		ndisableint(iv);
		*ivp = intnull;
		IV_UNLOCK(iv, lockp);
		return(0);
	}

	/*
	 * If we get here, the interrupt vector is shared.
	 */

	/*
	 * Disable the interrupt while we remove the handler.
	 */
	ndisableint(iv);

	sivpp = &mod_shr_ivect[iv];
	sivp = *sivpp;

	/*
	 * Search the packets for the handler.
	 */
	while(sivp)	{
		sihp = sivp->msv_sih;

		while(*sihp != NULL)	{
			if(*sihp == ihp)	{
				break;
			}
			sihp++;
		}
		if(*sihp == ihp)	{
			break;
		}

		sivpp = &(sivp->msv_next);
		sivp = sivp->msv_next;
	}

	/*
	 * At this point we should have found the handler.
	 */

	ASSERT(*sihp == ihp);
	ASSERT(sivp != NULL);

	if(--sivp->msv_cnt == 0)	{
		/*
		 * If the packet is now empty, free it.
		 */
		*sivpp = sivp->msv_next;
		kmem_free(sivp, sizeof(struct mod_shr_v));
	}
	else	{
		/*
		 * The packet is not empty, just remove the handler.
		 */
		*sihp = intnull;
	}

	/*
	 * If there is only one interrupt handler left in the list,
	 * call it directly through ivect[].
	 */
	sivp = mod_shr_ivect[iv];
	if(sivp->msv_cnt == 1 && sivp->msv_next == NULL)	{
		sihp = sivp->msv_sih;

		/*
		 * Find the remaining handler...
		 */
		while(*sihp != NULL)	{
			if(*sihp != intnull)	{
				break;
			}
			sihp++;
		}

		ASSERT(*sihp != NULL);

		*ivp = *sihp;	/* assign it to ivect[] */

		/*
		 * Free the last packet.
		 */
		kmem_free(sivp, sizeof(struct mod_shr_v));
		mod_shr_ivect[iv] = NULL;
	}

	nenableint(iv);
	IV_UNLOCK(iv, lockp);
	return(0);
}
