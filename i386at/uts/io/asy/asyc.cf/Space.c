/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/asy/asyc.cf/Space.c	1.4"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/immu.h>
#include <sys/stream.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/termio.h>
#include <sys/asyc.h>
#include <sys/sysinfo.h>
#include <sys/inline.h>
#include <sys/cmn_err.h>
#include <sys/stropts.h>
#include <sys/strtty.h>
#include <sys/debug.h>
#include <sys/eucioctl.h>
#include <sys/ddi.h>
#include "config.h"
#ifdef VPIX
#include <sys/proc.h>
#include <sys/tss.h>
#include <sys/v86.h>
#endif


unsigned int asyc_num = ASYC_UNITS;
unsigned int asyc_sminor = 0; /* Starting minor number */
unsigned int asyc_bufp[sizeof(struct asyc_aux)*ASYC_UNITS];

struct asyc asyctab[ASYC_UNITS] =
	{
	0,		/* flags */
	ASYC_0_SIOA+0,	/* transmit/recv register port address */
	ASYC_0_SIOA+1,	/* interrupt control reg. port address */
	ASYC_0_SIOA+2,	/* interrupt status reg. port address */
	ASYC_0_SIOA+3,	/* line control register port address */
	ASYC_0_SIOA+4,	/* line control register port address */
	ASYC_0_SIOA+5,	/* line control register port address */
	ASYC_0_SIOA+6,	/* line control register port address */
	ASYC_0_VECT,	/* interrupt vector (machine dependent) */
	0,		/* global bp structure */
	0		/* minor device number of port */
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#ifdef ASYC_1
		,
			/* next structure */
	0,
	ASYC_1_SIOA+0,
	ASYC_1_SIOA+1,
	ASYC_1_SIOA+2,
	ASYC_1_SIOA+3,
	ASYC_1_SIOA+4,
	ASYC_1_SIOA+5,
	ASYC_1_SIOA+6,
	ASYC_1_VECT,
	0,		/* global bp structure */
	1
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#endif  /* ASYC_1 */

#ifdef ASYC_2
		,
			/* next structure */
	0,
	ASYC_2_SIOA+0,
	ASYC_2_SIOA+1,
	ASYC_2_SIOA+2,
	ASYC_2_SIOA+3,
	ASYC_2_SIOA+4,
	ASYC_2_SIOA+5,
	ASYC_2_SIOA+6,
	ASYC_2_VECT,
	0,		/* global bp structure */
	2
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#endif  /* ASYC_2 */

#ifdef ASYC_3
		,
			/* next structure */
	0,
	ASYC_3_SIOA+0,
	ASYC_3_SIOA+1,
	ASYC_3_SIOA+2,
	ASYC_3_SIOA+3,
	ASYC_3_SIOA+4,
	ASYC_3_SIOA+5,
	ASYC_3_SIOA+6,
	ASYC_3_VECT,
	0,		/* global bp structure */
	3
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#endif  /* ASYC_3 */
	};

unsigned	p_asyc0 = ASYC_0_SIOA;

#ifdef ASYC_1
unsigned        p_asyc1 = ASYC_1_SIOA;
extern int      asycinitialized;

/*
 * put a character out the second serial port.
 * Do not use interrupts.  If char is LF, put out LF, CR.
 */
int
asycputchar2(c)
unsigned char   c;
{
        if (! asycinitialized)
                asycinit();
        if (inb(p_asyc1+ISR) & 0x38)
                return;
        while((inb(p_asyc1+LSR) & LSR_XHRE) == 0) /* wait for xmit to finish */
        {
                if ((inb(p_asyc1+MSR) & MSR_DCD) == 0)
                        return;
                tenmicrosec();
        }
        outb(p_asyc1+DAT, c); /* put the character out */
        if (c == '\n')
                asycputchar2(0x0d);
}

/*
 * get a character from the second serial port.
 *
 * If no character is available, return -1.
 * Run in polled mode, no interrupts.
 */

int
asycgetchar2()
{
        if ((inb(p_asyc1+ISR) & 0x38) || (inb(p_asyc1+LSR) & LSR_RCA) == 0) {
                tenmicrosec();
                return -1;
        }
        return inb(p_asyc1+DAT);
}
#else
int
asycputchar2()
{
}
int
asycgetchar2()
{
}
#endif /* P_asyc1 */
