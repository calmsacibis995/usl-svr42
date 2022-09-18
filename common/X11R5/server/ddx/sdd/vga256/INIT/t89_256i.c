/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/INIT/t89_256i.c	1.8"

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

extern int v256_page;		/* current page of memory in use */
extern int v256_endpage;	/* last valid offset from v256_fb */

#ifdef DEBUG
extern int xdebug;
#endif

struct v256_regs inittab[] = {

/* Type 0, Trident VGA 8900 1024x768 (non-interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0x27,
	/* CRTC */
	0xa2, 0x7f, 0x80, 0x85, 0x87, 0x90, 0x2c, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x01, 0xff, 0x40, 0x40, 0x07, 0x26, 0xa3, 0xff,

/* Type 1, Trident VGA 8900 1024x768 (interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x99, 0x7f, 0x81, 0x1b, 0x83, 0x10, 0x9d, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x84, 0x06, 0x7f, 0x80, 0x40, 0x84, 0x98, 0xa3, 0xff,

/* Type 2, Trident VGA 8900 800x600 256 colors with multisync monitors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x8f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0a, 0x57, 0x32, 0x40, 0x58, 0x6f, 0xa3, 0xff,
#ifdef NOTNOW
	0x7e, 0x63, 0x64, 0x81, 0x6b, 0x18, 0x99, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6e, 0x04, 0x57, 0x32, 0x40, 0x5e, 0x93, 0xa3, 0xff,
#endif

/* Type 3, Trident VGA 8900 800x600 256 colors with interlaced monitors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xef,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x8f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0a, 0x57, 0x32, 0x40, 0x58, 0x6f, 0xa3, 0xff,

/* Type 4, Trident VGA 8900 640x480 256 colors : 4 & 8 DRAM configurations */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
        0xeb,
        /* CRTC */
        0xc3, 0x9f, 0xa1, 0x84, 0xa6, 0x00, 0x0b, 0x3e,
        0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xea, 0x8c, 0xdf, 0x50, 0x40, 0xe7, 0x04, 0xa3, 0xff,

/* Type 5, Trident 8900 640x480 256 colors : 8 DRAM configuration */
	/* sequencer  */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x28, 0x40, 0xe7, 0x04, 0xa3, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern t89_256_init ();
extern t89_256_rest ();
extern t89_256_selectpage ();

/*
 * various resolutions and monitor combinations supported by T8900C
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
/* 0 */ "T8900C", "MULTISYNC", T89256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab[0]),
	t89_256_init, t89_256_rest, t89_256_selectpage,

/* 1 */ "T8900C", "INTERLACED", T89256_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab[1]),
	t89_256_init, t89_256_rest, t89_256_selectpage,

/* 2 */ "T8900C", "MULTISYNC", T89256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[2]),
	t89_256_init, t89_256_rest, t89_256_selectpage,

/* 3 */ "T8900C", "INTERLACED", T89256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[3]),
	t89_256_init, t89_256_rest, t89_256_selectpage,

/* 4 */ "T8900C", "STDVGA", T89256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab[4]),
	t89_256_init, t89_256_rest, t89_256_selectpage,

/* 5 */ "T8900Ca", "STDVGA", T89256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab[5]),
	t89_256_init, t89_256_rest, t89_256_selectpage,
};

int     v256_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));
int     v256_is_color;                  /* true if on a color display */
struct  at_disp_info    vt_info;

extern	struct	kd_memloc vt_map;
extern	int	vt_fd;			/* file descriptor for the vt used */
extern	unchar	*screen_buffer;		/* pointer to saved video buffer */
extern	unchar	saved_misc_out;		/* need to save and restore this */
					/* because the kernel doesn't do */
					/* it right			 */

extern	unchar	saved_rascas;		/* need to do this one also! */


/*
 * Table giving the information needed to initialize the V256 registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
struct reginfo	regtab[] = {
	16, 0x3b4, 0x3b5,	/* m6845init, monochrome */
	16, 0x3d4, 0x3d5,	/* m6845init, color/graphics */
	25, 0x3b4, 0x3b5,	/* v256init, monochrome */
	25, 0x3d4, 0x3d5,	/* v256init, color */
	NSEQ, 0x3c4, 0x3c5,	/* seqinit */
	NGRAPH, 0x3ce, 0x3cf,	/* graphinit */
	NATTR, 0x3c0, 0x3c0,	/* attrinit */
	NATTR, 0x3c0, 0x3c1,	/* attrinit */
};


static unchar trident_256_mode2;
static unchar trident_256_mode1_new;
static unchar trident_256_mode2_new;
static unchar trident_256_test;

/*
 *	t89_256_init(mode)	-- initialize a Trident VGA to one of
 *				it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
t89_256_init(mode)
int mode;
{
	static int inited = 0;
	int junk, VAL;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		in_reg(&regtab[I_SEQ], 0xb, junk);
/* TEMP:
 * 	there seems to be some problems with the different versions of T8900C
 *	chipsets; This code works only with 8900C; but some 8900C has
 *	problems; until this is resolved exit if it not the correct 8900C
 *
 *	should have 3C5.B = 0x4		for 8900C
 *		    3C5.B = 0x3		for 8900B
 *		    3C5.B = 0x2		for 8800CS
 */
if (junk != 0x4) {
ErrorF("Warning: The 8900C chip seem to be different from the real 8900C.\n");
ErrorF("If your screen is scrambled, you cannot use the 256 color modes.\n");
ErrorF("Trident T8900C chip id = %x\n", junk);
}

		if (v256_is_color) {
			in_reg(&regtab[I_EGACOLOR], 0x1e, trident_256_test);
		}
		else {
			in_reg(&regtab[I_EGAMONO], 0x1e, trident_256_test);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);
		in_reg(&regtab[I_SEQ], 0xd, trident_256_mode2);
		in_reg(&regtab[I_SEQ], 0xb, junk);
		in_reg(&regtab[I_SEQ], 0xe, trident_256_mode1_new);
		in_reg(&regtab[I_SEQ], 0xd, trident_256_mode2_new);
	}

	if (mode == T89256_1)
		VAL = 0x84;
	else
		VAL = 0x80;

	if (v256_is_color) { 
		out_reg(&regtab[I_EGACOLOR], 0x1e, VAL);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x1e, VAL);
	}

	switch(mode) {
		/* interlaced */
	    case T89256_8:
	    case T89256_1:
		out_reg(&regtab[I_SEQ], 0xb, junk);
		out_reg(&regtab[I_SEQ], 0xd, 0x10);
		in_reg(&regtab[I_SEQ], 0xb, junk);
		break;

		/* multisync */
	    case T89256_8a:
	    case T89256_1ni:
		out_reg(&regtab[I_SEQ], 0xb, junk);
		out_reg(&regtab[I_SEQ], 0xd, 0x10);
		in_reg(&regtab[I_SEQ], 0xb, junk);
		out_reg(&regtab[I_SEQ], 0xd, 1);
		break;
	    default:
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	t89_256_rest(mode)	-- restore a Trident VGA from one 
 *					of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
t89_256_rest(mode)
int mode;
{
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x1e, trident_256_test);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x1e, trident_256_test);
	}

	out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
	out_reg(&regtab[I_SEQ], 0xd, trident_256_mode2);
	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
	trident_256_mode1_new ^= 2;		/* flip bit two around */
	out_reg(&regtab[I_SEQ], 0xe, trident_256_mode1_new);
	out_reg(&regtab[I_SEQ], 0xd, trident_256_mode2_new);
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * t89_256_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
t89_256_selectpage(j)
register unsigned long j;
{
	v256_endpage = j | 0xffff;
	j >>= 16;
	if (j == v256_page)
		return;

	v256_page = j;

	/*
	 * have to flip the second bit around
	 */
	out_reg(&regtab[I_SEQ], 0xe, j ^ 0x2);
}

#ifdef DEBUG
toReg (port, reg_num, val)
int port;
int reg_num;
int val;
{
	int	data;

	outb (port,reg_num);
	outb ( (port+1), val);
}

fromReg (port, reg_num)
int port;
int reg_num;
{
	int	data;

	outb (port,reg_num);
	data = inb (port+1);
	printf ("data at port: %3x , register: %2x , is: %x\n", port, reg_num, data );
}

#endif
