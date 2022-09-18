/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/INIT/wdc1x_16i.c	1.4"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#include <fcntl.h>
#include <signal.h>
#include "sys/types.h"
#include "sys/kd.h"
#include "vtio.h"
#include "sys/vt.h"
#include "sys/inline.h"
#include "vgaregs.h"

#ifdef DEBUG
extern int xdebug;
#endif

#define STDVGA		0
#define WDC_800		1
#define WDC_1024	2

struct vga_regs inittab[] = {
/* Type 0, VGA 640x480 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 1, WDC1x 800x600 16 colors */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
        0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
        0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x58, 0x8a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,

/* Type 2, WDC1x 1024x768 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x23,
	/* CRTC */
	0x99, 0x7f, 0x7f, 0x1c, 0x83, 0x19, 0x97, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x7f, 0x83, 0x7f, 0x40, 0x00, 0x7f, 0x96, 0xe3, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern no_ext ();
extern wdc1x_init();
extern wdc1x_rest();

/*
 * The VGA entry that supports 640x480 is standard on all boards, so this
 * entry should be present for all individual drivers.
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"VGA", "STDVGA", VT_VGA, 1, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[0]),

	"WDC1x", "MULTISYNC", WDC_800, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 60*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[WDC_800]),

	"WDC11", "MULTISYNC", WDC_1024, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, wdc1x_init, wdc1x_rest, &(inittab[WDC_1024]),
};

int	vga_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));
struct	at_disp_info	vt_info;
int	vt_allplanes;
int 	vga_is_color;			/* true if on a color display */

/*
 * These variables are used to control the position of the visible portion
 * of the total video memory.  Normally, this just stays in the upper left
 * corner of the screen, but if we're set up for panning, it moves.
 */
extern int	vt_screen_w;	/* width of visible screen */
extern int	vt_screen_h;	/* height of visible screen */
extern int	vt_screen_x;	/* x position of UL corner of visible screen */
extern int	vt_screen_y;	/* y position of UL corner of visible screen */
extern int	vt_start_addr;	/* offset to start of visible screen */
extern int	vt_shift;	/* amount to shift visible screen */

extern struct	kd_memloc vt_map;
extern int	vt_fd;			/* file descriptor for the vt used */
extern int	max_planes;		/* maximum number of planes available */
extern unchar	*screen_buffer;		/* pointer to saved video buffer */
extern unchar	saved_misc_out;		/* need to save and restore this */
					/* because the kernel doesn't do */
					/* it right			 */

/*
 * Table giving the information needed to initialize the EGA/VGA registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
struct reginfo	regtab[] = {
	16, 0x3b4, 0x3b5,	/* m6845init, monochrome */
	16, 0x3d4, 0x3d5,	/* m6845init, color/graphics */
	25, 0x3b4, 0x3b5,	/* vgainit, monochrome */
	25, 0x3d4, 0x3d5,	/* vgainit, color */
	NSEQ, 0x3c4, 0x3c5,	/* seqinit */
	NGRAPH, 0x3ce, 0x3cf,	/* graphinit */
	NATTR, 0x3c0, 0x3c0,	/* attrinit */
	NATTR, 0x3c0, 0x3c1,	/* attrinit */
};

int
no_ext()
{
	return (SUCCESS);
}

static  unchar  pr2;
static  unchar  pr11;
static  unchar  pr13;
static  unchar  pr14;
static  unchar  pr15;
static  unchar  pr16;
static struct reginfo *wdptr;

/*
 *	wdc1x_init(mode)	-- initialize the Paradise VGA 1024 to one of
 *				it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
wdc1x_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		if (vga_is_color)
			wdptr = &regtab[I_EGACOLOR];
		else
			wdptr = &regtab[I_EGAMONO];

		out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
		out_reg(wdptr, 0x29, 0x85);

		in_reg(&regtab[I_GRAPH], 0x0c, pr2);
		in_reg(wdptr, 0x2a, pr11);
		in_reg(wdptr, 0x2c, pr13);
		in_reg(wdptr, 0x2d, pr14);
		in_reg(wdptr, 0x2e, pr15);
		in_reg(wdptr, 0x2f, pr16);
	}

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
	out_reg(wdptr, 0x29, 0x85);

	out_reg(&regtab[I_GRAPH], 0x0c, 0x0);
	out_reg(wdptr, 0x2a, 0x00);
	out_reg(wdptr, 0x2c, 0x34);
	out_reg(wdptr, 0x2d, 0x2a);
	out_reg(wdptr, 0x2e, 0x1b);
	out_reg(wdptr, 0x2f, 0x00);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	wdc1x_rest(mode)	-- restore the Paradise VGA 1024 from one
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
wdc1x_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
	out_reg(wdptr, 0x29, 0x85);

	out_reg(&regtab[I_GRAPH], 0x0c, pr2);
	out_reg(wdptr, 0x2a, pr11);
	out_reg(wdptr, 0x2c, pr13);
	out_reg(wdptr, 0x2d, pr14);
	out_reg(wdptr, 0x2e, pr15);
	out_reg(wdptr, 0x2f, pr16);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}

