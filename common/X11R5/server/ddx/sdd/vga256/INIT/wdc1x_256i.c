/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/INIT/wdc1x_256i.c	1.4"

/*
 *	Copyright (c) 1991, 1992 USL
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
/* Type 0, WD90C1x (ie: WDC90C10 and WDC90C11 chipsets) 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x63,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0xbf, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9c, 0x0e, 0x8f, 0x50, 0x40, 0x96, 0xb9, 0xa3, 0xFF,

/*
 * NOTE: There is no info on this chipset (6/20/91); this data seems to
 * 	 be working for both 640x480 and 800x600 modes. After getting the
 * 	 actual data, check it, especially the 'misc' register data....
 */
/* Type 1, WD90C11 (ie: WDC90C10 and WDC90C11 chipsets) 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x40, 0xe7, 0x04, 0xe3, 0xFF,

/* Type 1, WD90C11 : 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern wdc_init ();
extern wdc_restore ();
extern wdc_selectpage ();

#define WDC_400 0
#define WDC_600	1

struct	at_disp_info	disp_info[] = {	/* display info for support adapters */

	"WDC11", "MULTISYNC", WDC_600, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[2]),
	wdc_init, wdc_restore, wdc_selectpage,

	"WDC11", "STDVGA", WDC_600, 640, 480, NULL, 8*64*1024, 64*1024,
	0x3D4, 640, &(inittab[1]),
	wdc_init, wdc_restore, wdc_selectpage,

	"WDC1x", "STDVGA", WDC_400, 640, 400, NULL, 8*64*1024, 64*1024,
	0x3D4, 640, &(inittab[0]),
	wdc_init, wdc_restore, wdc_selectpage,
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


static unchar pr0a;
static unchar pr4;
static unchar pr16;
static struct reginfo *wdc_ptr;
/*
 *	wdc_init(mode)	-- initialize a VT&T VDC600U VGA board to
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
wdc_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;

		out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
		switch(mode) {
		case WDC_400:
		case WDC_600:
			if (v256_is_color)
				wdc_ptr = &regtab[I_EGACOLOR];
			else
				wdc_ptr = &regtab[I_EGAMONO];
	
			out_reg(wdc_ptr, 0x29, 0x85);	/* unlock regs */

			in_reg(wdc_ptr, 0x2f, pr16);
			in_reg(&regtab[I_GRAPH], 0x09, pr0a);
			in_reg(&regtab[I_GRAPH], 0x0e, pr4);
			break;
		}
	}

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */

	switch(mode) {
	case WDC_400:
	case WDC_600:
		out_reg(wdc_ptr, 0x29, 0x85);	/* unlock regs */
		out_reg(wdc_ptr, 0x2f, 0);
		out_reg(&regtab[I_GRAPH], 0x09, 0);
		out_reg(&regtab[I_GRAPH], 0x0e, 1);
		break;
	}
		
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	


/*
 *	wdc_restore(mode) -- restore a WDC90C1x (ex: AT&T VDC600U) VGA board 
 *				from one of it's "extended" modes.  This takes 
 *				care of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
wdc_restore(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */

	switch(mode) {
	case WDC_400:
	case WDC_600:
		out_reg(wdc_ptr, 0x29, 0x85);	/* unlock regs */
		out_reg(wdc_ptr, 0x2f, pr16);
		out_reg(&regtab[I_GRAPH], 0x09, pr0a);
		out_reg(&regtab[I_GRAPH], 0x0e, pr4);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * wdc_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
wdc_selectpage(j)
register unsigned long j;
{
	v256_endpage = j | 0xffff;
	j >>= 16;
	if (j == v256_page)
		return;

	v256_page = j;

	out_reg(&regtab[I_GRAPH], 0x09, j << 4);
}

