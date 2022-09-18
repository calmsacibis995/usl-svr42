/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)uts-x86at:boot/at386/mip/olivetti.c	1.1"
#ident	"$Header: $"

#include "util/types.h"
#include "util/inline.h"
#include "io/ansi/at_ansi.h"
#include "io/kd/kd.h"
#include "io/cram/cram.h"
#include "svc/bootinfo.h"
#include "boot/boot.h"
#include "boot/initprog.h"
#include "boot/libfm.h"
#include "boot/mip.h"

/* Olivetti-specific keyboard controller commands/values */

#define KB_SPECIAL	0xa8	/* Enable Olivetti extended commands */
#define KB_RCONF	0x81	/* Read Configuration Port */
#define KB_WCONF	0x80	/* Write Configuration Port */
#define KB_RSEN0	0x40	/* Shadow RAM enable */

#define SPECIAL_LOC	*(unsigned short *) 0x01b6
#define OL_ALT_ID()	*(unsigned char *) 0xFFFFD

struct int_pb	intcons;
char	cons_str[] =	"  386/ix   OS";

int	olivetti_end();

olivetti(lpcbp)
struct	lpcb *lpcbp;
{
#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK)
		printf("Begin Olivetti initalization, model ");
#endif

/* 	establish potential memory availability 			*/

	btep->memrng[0].base = 0;
	btep->memrng[0].extent = 640 * 1024;	/* base 640K 		*/
	btep->memrng[0].flags = B_MEM_BASE;

	btep->memrng[1].base = 0x100000;	/* base- 1MB 		*/
	btep->memrng[1].extent = 0xF00000;	/* up to 16MB, size 15M */
	btep->memrng[1].flags = B_MEM_EXPANS;

	if ((btep->sysenvmt.m_model = OL_ALT_ID()) != OL_M380) {
/* 	Machine is either a micro-channel or one of XP4/7/9 		*/

		btep->memrngcnt = 2;
		if (btep->sysenvmt.machflags & MC_BUS)
			mc_a20();
		else
			a20();		/* set A20 to address above 1MB */
	}
	else {
		btep->memrng[2].base = MEM16M;		/* base- 16M	*/
		btep->memrng[2].extent = 48*1024*1024;	/* extent- 48M	*/
		btep->memrng[2].flags = B_MEM_EXPANS;

/*		check for 640K system					*/
		if (btep->sysenvmt.CMOSmembase > 512) {
			btep->memrng[3].base = 0x800A0000;/* hidden mem */
			btep->memrng[3].extent = 0x60000; /* extent-384K*/
		} else {
			btep->memrng[3].base = 0x80080000;/* hidden mem */
			btep->memrng[3].extent = 0x80000; /* extent-512K*/
		}
		btep->memrng[3].flags = B_MEM_SHADOW | B_MEM_NODMA;
		btep->sysenvmt.machflags |= MEM_SHADOW;

		btep->memrngcnt = 4;

		if (consenable()) {
			btep->sysenvmt.m_model = OL_XP5;
#ifdef BOOT_DEBUG
			if (btep->db_flag & BOOTTALK)
				printf("XP5, sw = %x, cons at = %x ",
					btep->sysenvmt.md.olivetti.swa13_16,
					btep->sysenvmt.md.olivetti.cons_dra);
#endif
			cons_write(cons_str, strlen(cons_str));
		}
		a20();		/* set A20 to address above 1MB 	*/

/*		set handling routine for command MIP_END		*/
		MIP_end = (ulong) olivetti_end;
	} 

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTTALK) {
		if (btep->sysenvmt.m_model == OL_P500)
			printf("P500 ");
		else if (btep->sysenvmt.m_model == OL_P800)
			printf("P800 ");
		else if (btep->sysenvmt.m_model == OL_XP479)
			printf("XP4/7/9 ");
		else if (btep->sysenvmt.m_model != OL_XP5)
			printf("undetectable ");
		printf("\n");
	}
#endif
}

olivetti_end()
{

#ifdef BOOT_DEBUG
	if (btep->db_flag & BOOTDBG)
		printf("Enabling Olivetti shadow ram area.\n");
#endif

	shadow();
 
/*	Check for an EGA or VGA and re-do the vectors 			*/

	if (((CMOSread(EB) >> VID_SHFT) & 3) == 0) {
		fix_ega_vect(EGA_SEGMENT);
		SPECIAL_LOC = EGA_SEGMENT;
	}
}


/* 
 * Disable shadow RAM 
 */

shadow()
{
	unsigned	i = 0;
	unsigned	v = 0;


	if ( flush8042())
		return 1;

	if (inb(KB_STAT) & KB_OUTBF) /* clear output buffer */
		v = inb(KB_OUT);

	bigwait();

	outb(KB_ICMD, KB_SPECIAL);

	if ( flush8042())
		return 1;

	outb(KB_ICMD, KB_RCONF); /* read config port command */

	if ( flush8042())
		return 1;

	outb(KB_IDAT, 2);	/* select config port 2 */

	for ( i = 0; i < 0x200; i++ ) {
		if ((inb(KB_STAT) & KB_OUTBF))
			break;
		bigwait();
	}

	i = 0;
	i = inb(KB_OUT); /* get config port bits */
	i &= 0xff;
	i &= ~KB_RSEN0; /* disable RAM shadowing of ROM */

	bigwait();

	if ( flush8042())
		return 1;

	outb(KB_ICMD, KB_SPECIAL);

	if ( flush8042())
		return 1;

	outb(KB_ICMD, KB_WCONF); /* set config port command */

	if ( flush8042())
		return 1;

	outb(KB_IDAT, 2);	/* select config port 2 */

	if ( flush8042())
		return 1;

	outb(KB_IDAT, i);

	flush8042();

	return 0;
}

bigwait()
{
	int	j;
	for (j=0; j<80000; j++);
}

consenable()
{
	intcons.intval = 0x10;
	intcons.ax  = 0xAA04;	/* Enable the console */
	if (doint(&intcons))
		return(0);

	intcons.ax  = 0xAA01;	/* Read Console Location */
	intcons.bx  = intcons.es  = 0;
	if (doint(&intcons))
		return(0);
	if (intcons.bx == 0 || intcons.es == 0 )
		return(0);
	btep->sysenvmt.md.olivetti.cons_dra = 
		(char *)((((uint)(intcons.es)) << 4) | intcons.bx);

	intcons.ax  = 0xAA02;	/* Read Switch Settings */
	if (doint(&intcons))
		return(0);
	btep->sysenvmt.md.olivetti.swa13_16 = (intcons.ax & 0xFF);

	intcons.ax  = 0xAA07;
	intcons.cx  = 0x83;	/* Clear it */
	if (doint(&intcons))
		return(0);

	intcons.ax  = 0xAA07;
	intcons.cx  = 0x03;	/* Clear it off */
	if (doint(&intcons))
		return(0);

	return(1);
}

cons_write(cp, len)
char	*cp;
int	len;
{
	unsigned char	pos = 15;
	int	i;

	for (i = 0; i < len; i++, pos--)
		cons_put(*cp++, pos);
}

cons_put(c, p)
char	c;
unchar	p;
{
	intcons.ax = 0xAA06;
	intcons.cx = (( c & 0x7f ) << 8) | p;
	doint(&intcons);
}

