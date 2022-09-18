/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/INIT/t89_16i.c	1.5"

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

#define STD_VGA  	0
#define VSYNC72_800	1
#define VSYNC56_800	2
#define T89_1024NI	3
#define T89_1024I	4

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

/* Type 1 (VSYNC72_800), T89VGA 800x600 16 colors: 72Hz vert sync monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2b,
	/* CRTC */
	/* entries for 72 Hz Vertical refresh */
	0x7b, 0x63, 0x64, 0x81, 0x6b, 0x18, 0x99, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6e, 0x84, 0x57, 0x32, 0x00, 0x5e, 0x93, 0xe3, 0xff,

/* Type 2 (VSYNC56_800), T89VGA_56 800x600 16 colors: 56Hz vert sync monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	/* entries for 56 Hz Vertical refresh */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x8f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x8a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,

/* Type 3 (T89_1024NI), T89VGA 1024x768 16 colors: non interlaced monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x27,
	/* CRTC */
	0xa2, 0x7f, 0x80, 0x85, 0x87, 0x90, 0x2c, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x81, 0xff, 0x40, 0x00, 0x84, 0x98, 0xe3, 0xff,

/* Type 4 (T89_1024I), T89VGAi 1024x768 16 colors: interlaced monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2b,
	/* CRTC */
	0x99, 0x7f, 0x81, 0x1b, 0x83, 0x10, 0x9d, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x84, 0x81, 0x7f, 0x80, 0x00, 0x84, 0x98, 0xe3, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern no_ext ();
extern trident_init ();
extern trident_rest ();

/*
 * The VGA entry that supports 640x480 is standard on all boards, so this
 * entry should be present for all individual drivers.
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"VGA", "STDVGA", VT_VGA, 1, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[STD_VGA]),

	"T8900C", "MULTISYNC", VT_T89_8a,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE,trident_init, trident_rest, &(inittab[VSYNC72_800]),

	"T8900C", "CRYSTALSCAN", VT_T89_8,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, trident_init, trident_rest, &(inittab[VSYNC56_800]),

	"T8900C", "MULTISYNC", VT_T89_1ni,1, 0, 1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, trident_init, trident_rest, &(inittab[T89_1024NI]),

	"T8900C", "INTERLACED", VT_T89_1,1, 0, 1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, trident_init, trident_rest, &(inittab[T89_1024I]),
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

static unchar trident_mode2;
static unchar trident_mode2_new;
static unchar trident_test;

/*
 *	trident_init(mode)	-- initialize a Trident VGA to one of
 *				it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
trident_init(mode)
int mode;
{
	static int inited = 0;
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

		if (vga_is_color) {
			in_reg(&regtab[I_EGACOLOR], 0x1e, trident_test);
		}
		else {
			in_reg(&regtab[I_EGAMONO], 0x1e, trident_test);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		in_reg(&regtab[I_SEQ], 0xd, trident_mode2);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		in_reg(&regtab[I_SEQ], 0xd, trident_mode2_new);
	}

	switch(mode) {
	case VT_T89_8a:
		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0x0);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		out_reg(&regtab[I_SEQ], 0xd, 1);
		break;

	case VT_T89_1:
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x84);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x84);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0);
		break;

	case VT_T89_1ni:
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x80);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		out_reg(&regtab[I_SEQ], 0xd, 1);
		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	trident_rest(mode)	-- restore a Trident VGA from one 
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
trident_rest(mode)
int mode;
{
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x1e, trident_test);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x1e, trident_test);
	}

	out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
	out_reg(&regtab[I_SEQ], 0xd, trident_mode2);
	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
	out_reg(&regtab[I_SEQ], 0xd, trident_mode2_new);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}

