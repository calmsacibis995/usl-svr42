/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/asy/asyhp.cf/Space.c	1.2"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/immu.h>
#include <sys/stream.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/termio.h>
#include <sys/asyhp.h>
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


unsigned int asyhp_num = ASYHP_UNITS;
unsigned int asyhp_sminor = 0; /* Starting minor number */

int	asyhp_outchars[ASYHP_UNITS]; /* Number of characters outputted to the FIFO */

struct asyhp asyhptab[ASYHP_UNITS] =
	{
	0,		/* flags */
	ASYHP_0_SIOA+0,	/* transmit/recv register port address */
	ASYHP_0_SIOA+1,	/* interrupt control reg. port address */
	ASYHP_0_SIOA+2,	/* interrupt status reg. port address */
	ASYHP_0_SIOA+3,	/* line control register port address */
	ASYHP_0_SIOA+4,	/* modem control reg. port address */
	ASYHP_0_SIOA+5,	/* line status register port address */
	ASYHP_0_SIOA+6,	/* modem status reg. port address */
	ASYHP_0_VECT,	/* interrupt vector (machine dependent) */
	0,		/* global bp structure */
	0		/* minor device number of port */
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#ifdef ASYHP_1
		,
			/* next structure */
	0,
	ASYHP_1_SIOA+0,
	ASYHP_1_SIOA+1,
	ASYHP_1_SIOA+2,
	ASYHP_1_SIOA+3,
	ASYHP_1_SIOA+4,
	ASYHP_1_SIOA+5,
	ASYHP_1_SIOA+6,
	ASYHP_1_VECT,
	0,		/* global bp structure */
	1
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#endif  /* ASYHP_1 */

#ifdef ASYHP_2
		,
			/* next structure */
	0,
	ASYHP_2_SIOA+0,
	ASYHP_2_SIOA+1,
	ASYHP_2_SIOA+2,
	ASYHP_2_SIOA+3,
	ASYHP_2_SIOA+4,
	ASYHP_2_SIOA+5,
	ASYHP_2_SIOA+6,
	ASYHP_2_VECT,
	0,		/* global bp structure */
	2
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#endif  /* ASYHP_2 */

#ifdef ASYHP_3
		,
			/* next structure */
	0,
	ASYHP_3_SIOA+0,
	ASYHP_3_SIOA+1,
	ASYHP_3_SIOA+2,
	ASYHP_3_SIOA+3,
	ASYHP_3_SIOA+4,
	ASYHP_3_SIOA+5,
	ASYHP_3_SIOA+6,
	ASYHP_3_VECT,
	0,		/* global bp structure */
	3
#ifdef MERGE386
                ,
        0,
        0
#endif /* MERGE386 */

#endif  /* ASYHP_3 */
	};

unsigned	p_asyhp0 = ASYHP_0_SIOA;

#ifdef ASYHP_1
unsigned        p_asyhp1 = ASYHP_1_SIOA;
extern int      asyhpinitialized;

/*
 * put a character out the second serial port.
 * Do not use interrupts.  If char is LF, put out LF, CR.
 */
int
asyhpputchar2(c)
unsigned char   c;
{
        if (! asyhpinitialized)
                asyhpinit();
        if (inb(p_asyhp1+ISR) & 0x38)
                return;
        while((inb(p_asyhp1+LSR) & LSR_XHRE) == 0) /* wait for xmit to finish */
        {
                if ((inb(p_asyhp1+MSR) & MSR_DCD) == 0)
                        return;
                tenmicrosec();
        }
        outb(p_asyhp1+DAT, c); /* put the character out */
        if (c == '\n')
                asyhpputchar2(0x0d);
}

/*
 * get a character from the second serial port.
 *
 * If no character is available, return -1.
 * Run in polled mode, no interrupts.
 */

int
asyhpgetchar2()
{
        if ((inb(p_asyhp1+ISR) & 0x38) || (inb(p_asyhp1+LSR) & LSR_RCA) == 0) {
                tenmicrosec();
                return -1;
        }
        return inb(p_asyhp1+DAT);
}
#else
int
asyhpputchar2()
{
}
int
asyhpgetchar2()
{
}
#endif /* P_asyhp1 */
