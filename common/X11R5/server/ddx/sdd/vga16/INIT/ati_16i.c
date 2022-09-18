/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/INIT/ati_16i.c	1.5"

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
#define ATI_STD		1
#define ATI_72Hz	2
#define ATI_60Hz	3
#define ATI_TVM2A	4
#define ATI_1430	5

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

/* Type 1 (ATI_STD), ATI VGA Wonder XL 800x600 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x86,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 2 (ATI_72Hz), ATI VGA Wonder XL 800x600 for 72Hz Vert sync */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x86,
	/* misc */
	0xef,
	/* CRTC */
	0x7e, 0x63, 0x65, 0x9d, 0x6c, 0x9b, 0x4b, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3d, 0x80, 0x2b, 0x32, 0x0f, 0x33, 0x36, 0xe7, 0xff,

/* Type 3 (ATI_60Hz), ATI VGA Wonder XL 800x600 for 60Hz Vert sync */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x86,
	/* misc */
	0xef,
	/* CRTC */
	0x80, 0x63, 0x65, 0x9d, 0x6b, 0x9c, 0x36, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2c, 0x8e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 4 (ATI_TVM2A), ATI VGA Wonder XL 800x600 for dual sync monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x86,
	/* misc */
	0xef,
	/* CRTC */
	0x74, 0x63, 0x64, 0x89, 0x66, 0x91, 0x35, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2b, 0x8c, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 5 (ATI_1430), ATI VGA Wonder XL 800x600 for ???? */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x86,
	/* misc */
	0xef,
	/* CRTC */
	0x7c, 0x63, 0x64, 0x80, 0x65, 0x69, 0xbc, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x62, 0x9e, 0x57, 0x32, 0x0f, 0x59, 0xb0, 0xe7, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern no_ext ();
extern ati_init ();
extern ati_rest ();

/*
 * The VGA entry that supports 640x480 is standard on all boards, so this
 * entry should be present for all individual drivers.
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"VGA", "STDVGA", VT_VGA, 1, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[STD_VGA]),

	"ATI", "MULTISYNC", VT_ATIPLUS_8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[ATI_STD]),

	"ATI", "MULTISYNC72", VT_ATIPLUS_8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[ATI_72Hz]),

	"ATI", "MULTISYNC60", VT_ATIPLUS_8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[ATI_60Hz]),

	"ATI", "TVM2A", VT_ATIPLUS_8,1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[ATI_TVM2A]),

	"ATI", "1430", VT_ATIPLUS_8,1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[ATI_1430]),
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
   return(SUCCESS);
}


static unchar	ati0;
static unchar	ati2;
static unchar	ati3;
static unchar	ati8;
static unchar	ati9;
static unchar	atie;
/*
 *	ati_init(mode)	-- initialize an ATI VGA Wonder board into
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
ati_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	/*
	 * Prior to SVR4.1 ES, you have to be root to write to 0x1ce/0x1cf ports
	 * To be backward compatible, check the effective userid and allow the
	 * initialization, only if the effective-userid is root; the side
	 * effect of this is the user wouldn't see the following error message
	 * in pre-SVR4ES, if he/she tries to run the server as non-root
	 * The server core dumps with the following msg:
	 *   Memory fault(coredump)
	 */
	if (geteuid () == 0) {
	  /* enable ATI ports */
	  if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x1ce) == -1) ||
	    (ioctl(vt_fd, KDADDIO, (unsigned short) 0x1cf) == -1) ||
	    (ioctl(vt_fd, KDENABIO) == -1)) {
		ErrorF("Can't enable ATI extensions, KDADDIO Failed.\n");
		ErrorF("Probable cause : User does not have permission for this operation.\n");
		ErrorF("Try running as super user.\n");
		return (FAIL);
	  }
	}

	if (!inited) {
		outb(0x1ce, 0xbe);
		atie = inb(0x1cf);

		outb(0x1ce, 0xb9);
		ati9 = inb(0x1cf);

		outb(0x1ce, 0xb8);
		ati8 = inb(0x1cf);

		outb(0x1ce, 0xb3);
		ati3 = inb(0x1cf);

		outb(0x1ce, 0xb2);
		ati2 = inb(0x1cf);

		outb(0x1ce, 0xb0);
		ati0 = inb(0x1cf);
		inited = 1;
	}

	switch(mode) {
	case VT_ATIPLUS_8:
		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xf7) | 0x10);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);

		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 & 0xfd);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	
	

/*
 *	ati_rest(mode)	-- restore an ATI VGA Wonder board from
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
ati_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x1ce, 0xbe);
	outb(0x1cf, atie);

	outb(0x1ce, 0xb9);
	outb(0x1cf, ati9);

	outb(0x1ce, 0xb8);
	outb(0x1cf, ati8);

	outb(0x1ce, 0xb3);
	outb(0x1cf, ati3);

	outb(0x1ce, 0xb2);
	outb(0x1cf, ati2);

	outb(0x1ce, 0xb0);
	outb(0x1cf, ati0);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}

