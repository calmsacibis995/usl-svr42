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

#ident	"@(#)uts-x86at:boot/at386/mip/misc386.c	1.2"
#ident	"$Header: $"

#include "util/types.h"
#include "util/inline.h"
#include "io/ansi/at_ansi.h"
#include "io/kd/kd.h"
#include "svc/bootinfo.h"
#include "boot/initprog.h"

/*
 *	This function enables the A20 gate to enable addressing
 *	above the 1MB boundary. Returns non-zero on error 
 *	though there is not much that can be done if this does
 *	not work correctly.
 */

a20()
{
	if ( flush8042() )
		return(-1);
	
	outb(KB_ICMD, 0xd1);	/* 8042 command to write output port */
	
	if ( empty8042())
		return(-1);

	outb(KB_IDAT, 0xdf);	/* address line 20 gate on */
	flush8042();
	BTEP_INFO.bootflags |= BF_A20SET;
	return(0);
}

holdit()		/* fake a delay */
{
	register int	j;

	for (j=0; j<80000; j++);
}


/*
 *	Flush the keyboard output buffer, return non-zero on error.
 */

flush8042()
{
	int	i, v;

	for( i = 0; i < 200; i += 1 ) {
		if (((v = inb(KB_STAT)) & (KB_OUTBF | KB_INBF)) == 0 )
			return 0;
		else {
			if ( v & KB_OUTBF) {	/* if output ready */
				v = inb(KB_OUT); /* clear output buffer */
				holdit();
			}
		}
	}
	return 1;
}

/*
 *	Wait for the keyboard input buffer to be empty,
 *	return non-zero on error.
 */

empty8042()
{
	register i;

	for (i = 0; i < 200; i++) {
		if ((inb(KB_STAT) & KB_INBF) == 0 )
			return(0);
	}
	return(1);
}


/*
 *	Called to repair the video vectors after shadow ram has
 *	been disabled. Need to change the segment value back to
 *	C000 vs into shadow ram. Function input is the segment
 *	value to be placed in the three standard(?) locations.
 */

#define	vect_ptr1  *(ushort *)0x42	/* int 10 vector */
#define	vect_ptr2  *(ushort *)0x7e	/* pointer, ega dot vector */
#define	vect_ptr3  *(ushort *)0x10e	/* ega font pointer */

fix_ega_vect(vid_seg)
unsigned short vid_seg;
{
	BTEP_INFO.bootflags |= BF_EGAVSET;

	vect_ptr1 = vid_seg;
	vect_ptr2 = vid_seg;
	vect_ptr3 = vid_seg;
}

/*
 *	Micro-channel set A20 function is different. Not sure why the
 *	regular AT bus one does not work but it doesn't.
 */

#define KB_MC_A20	0xd3	/* kb command with A20 bit set */
#define KB_CMDF		0x08	/* controller flag indicating next out
				/* to KB_OUT should be a command */

mc_gate20()
{
	outb(KB_IDAT, KB_MC_A20);
	holdit();
	empty8042();
}

mc_a20()
{
	unchar	s0, s1,s2,s3,s4,s;
	int	efl;

	s0 = s1 = s2 = s3 = s4 = s = 0;

	efl = intr_disable();

/*	First check to see if the controller has something to send us.
 *	If so take it to flush the controller's output buffer.
 *	Also, we cannot send a command if the input buffer is full,
 *	so wait here for the controller to flush it.
 */
	flush8042();

/*	If the controller is expecting a command, status bit 3 set,
 *	give it one, the gate20 one, to get it out of that mode.
 */
	if ( (s0 = inb(KB_STAT)) & KB_CMDF )
		mc_gate20();

/*	The controller may want to complain about the command, so make
 *	sure to check the output buffer flag again and flush if neccessary.
 */
	if ( flush8042() )
		s1 = inb(KB_STAT);

/*	Now at last we can tell it what we want to do. */
/*	So send the command to tell it to accept a command. */

	outb(KB_ICMD, KB_WOP);
	holdit();

/*	Wait for the controllers input buffer to empty and the command
 *	expected bit 3 in the status word to be true.
 */
	if ( empty8042() )
		s2 = inb(KB_STAT);

/*	Send the gate20 command. */

	if (((s3=inb(KB_STAT)) & KB_CMDF) == 0) {  /* make sue we can command */
		holdit();
		outb(KB_ICMD, KB_WOP);
		holdit();
	}
	mc_gate20();

/*	Finally, flush the output buffer again if necessary. */

	if ( flush8042() )
		s4 = inb(KB_STAT);

/*	Whew ! */

	s = inb(KB_STAT);

	if (btep->sysenvmt.machine == MPC_APRICOT) {
		outb(KB_ICMD, 0xc0);
		holdit();
	}

	intr_restore(efl);

	BTEP_INFO.bootflags |= BF_A20SET;
}

