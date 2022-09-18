/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/ddi386at.c	1.5"
#ident	"$Header: $"

/*            UNIX i386at-specific Device Driver Interface functions          

 * This file contains functions that constitute functionality
 * specific to the i386at version of the Device Driver Interface.
 */ 

#include <util/types.h>
#include <io/conf.h>
#include <svc/bootinfo.h>
#include <svc/sysenvmt.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#define __DDI_C__	/* to keep inline.h from redefining kvtophys */
#include <util/inline.h>


/* function: drv_gethardware()     
 * purpose:  get i386at hardware-specific information for drivers
 */

int
drv_gethardware(parm, valuep)
unsigned long parm;
void *valuep;
{
	switch (parm) {

	case PROC_INFO: { /* get processor-type info */

		extern long cpu_family, cpu_stepping, do386b1;
		register struct cpuparms *cpup;

		cpup = (struct cpuparms *) valuep;
		bzero (cpup, sizeof (struct cpuparms));

		/* take advantage of the fact that cpu_family is
		 * set to 3 for 386, 4 for 486, 5 for 586
		 */
		cpup->cpu_id = cpu_family - 2;
		cpup->cpu_step = 0; /* by default */

		if (cpup->cpu_id == CPU_i386)
			cpup->cpu_step = do386b1;
		if (cpup->cpu_id == CPU_i586)
			cpup->cpu_step = cpu_stepping;
		/* note: no detectable stepping info for 486 */
		break;
	}

	case IOBUS_TYPE: { /* are we ISA (std AT), EISA or MCA? */
		extern struct sysenvmt *sysenvmtp;

		* (unsigned long *)valuep = BUS_EISA;
		if (sysenvmtp->machflags&MC_BUS)
			* (unsigned long *)valuep = BUS_MCA;
		if (sysenvmtp->machflags&AT_BUS)
			* (unsigned long *)valuep = BUS_ISA;
		break;
	}

	case TOTAL_MEM: { /* return the number of bytes of physical memory */
		extern int physmem;
		* (unsigned long *)valuep = ctob(physmem);
		break;
	}

	case DMA_SIZE: { /* how many bits of DMA addressing have we? */
		extern int Dma_addr_28;

		if (Dma_addr_28)
			* (unsigned long *) valuep = 28;
		else
			/* 24-bit is standard for MCA, EISA and ISA */
			* (unsigned long *) valuep = 24;
		break;
	}


	case BOOT_DEV: { /* what device did we boot from */

		register struct bootdev *bdevp;

		bdevp = (struct bootdev *) valuep;

		/* the bzero will set bdevp->bdv_unit to 0.
		 * since on i386at machines, one always boots
		 * off the first hard disk or first flop,
		 * the unit number is always 0
		 */
		bzero(bdevp, sizeof(struct bootdev));
		bdevp->bdv_type = BOOT_DISK;
		if (bootinfo.bootflags & BF_FLOPPY)
			bdevp->bdv_type = BOOT_FLOPPY;
		break;
	}

	case HD_PARMS: { /* obtain parameters on 2 primary disks.
			  * Note that this function is not extendible
			  * to additional disks as information for them
			  * is only avail from driver, not bootinfo.
			  */

		unsigned long drive_num;
		struct hdparms *hdp;

		hdp = (struct hdparms *) valuep;
		if ((drive_num = hdp->hp_unit) > 1)
			return (-1);
		/* only "unit" values of 0 or 1 are OK */
		bzero(hdp, sizeof(struct hdparms));
		hdp->hp_unit = drive_num;
		hdp->hp_ncyls = bootinfo.hdparams[drive_num].hdp_ncyl;
		hdp->hp_nheads = bootinfo.hdparams[drive_num].hdp_nhead;
		hdp->hp_nsects = bootinfo.hdparams[drive_num].hdp_nsect;
		break;
	}

	default: /* bad parm value */
		return -1;

	}
	return 0; /* success */
}
