/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/INIT/ati_256i.c	1.5"

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

struct v256_regs inittab[] = { /* V256 register initial values */
/* Type 0, ATI VGA Wonder+ 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x8a,
	/* misc */
	0x2f,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 1, ATI VGA Wonder+ 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 2, ATI VGA Wonder+ 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0x63,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9c, 0x0e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xe3, 0xff,

/* Type 3, ATI VGA Wonder V4, V5 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2b, 0x32, 0x0f, 0x30, 0x34, 0xe7, 0xff,

/* Type 4, ATI VGA Wonder 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern ati_init ();
extern ati_restore ();
extern ati_selectpage ();

/*
 * disp_info is known to the actual drawing library, ie: libvga256.so
 * Based on the entry from Xwinconfig, the corresponding entry is picked up
 * from this array.
 * For example, if the entry (in /usr/X/defaults/Xwinconfig) is a ET4000
 * a multisync monitor and the mode is 1024x768, the first entry in this
 * table is picked up. Here data from inittab256[0] is used to initialize the
 * grahpics, sequencer and CRTC regs. 
 * After initializing the standard regs, the function specified here,
 * et4k_init is called to initialize the extended regs. Similary to restore
 * from the extended state, the restore function (et4k_restore) is called.
 *
 * The extended register initialize and restore functions are in this file
 * and any custom extended register initialization and restoration should
 * be done in these two functions respectively.
 */

struct at_disp_info    disp_info[] = { /* display info for support adapters */
/* ATI Wonder Plus */
	"ATI_PLUS","MULTISYNC",ATIPLUS256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[0]),
	ati_init, ati_restore, ati_selectpage,

	"ATI_PLUS", "STDVGA", ATIPLUS256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab[1]),
	ati_init, ati_restore, ati_selectpage,

	"ATI_PLUS","STDVGA", ATIPLUS256_400, 640, 400, NULL, 4*64*1024, 64*1024,
	0x3D4, 640, &(inittab[2]),
	ati_init, ati_restore, ati_selectpage,

/* ATI Wonder V5 */
	"ATI_V5", "MULTISYNC", ATI2565_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[3]),
	ati_init, ati_restore, ati_selectpage,

	"ATI_V5", "STDVGA", ATI2565_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab[4]),
	ati_init, ati_restore, ati_selectpage,

/* ATI Wonder V4 */
	"ATI_V4", "MULTISYNC", ATI2564_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[3]),
	ati_init, ati_restore, ati_selectpage,

	"ATI_V4", "STDVGA", ATI2564_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab[4]),
	ati_init, ati_restore, ati_selectpage,
};


static unchar	ati0;
static unchar	ati_page;
static unchar	ati2;
static unchar	ati3;
static unchar	ati6;
static unchar	ati8;
static unchar	ati9;
static unchar	atie;

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
	 * Prior to SVR4.1 ES, you have to be root to write to 0x1ce/1cf port;
	 * to be backward compatible, check the effective userid and allow the
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
		ErrorF("Can't enable ATI extensions\n");
		return;
	  }
	}

	if (!inited) {
		outb(0x1ce, 0xbe);
		atie = inb(0x1cf);

		outb(0x1ce, 0xb9);
		ati9 = inb(0x1cf);

		outb(0x1ce, 0xb8);
		ati8 = inb(0x1cf);

		outb(0x1ce, 0xb6);
		ati6 = inb(0x1cf);

		outb(0x1ce, 0xb3);
		ati3 = inb(0x1cf);

		outb(0x1ce, 0xb2);
		ati2 = inb(0x1cf);

		outb(0x1ce, 0xb0);
		ati0 = inb(0x1cf);
		inited = 1;
	}

	ati_page = ati2;

	switch(mode) {
	case ATI256_6:
		outb(0x1ce, 0xb0);
		outb(0x1cf, 0x30);
		break;

	case ATI256_8:
		ati_page |= 0x40;
		outb(0x1ce, 0xb2);
		outb(0x1cf, ati_page);

		outb(0x1ce, 0xb0);
		outb(0x1cf, 0x38);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);
		break;

	case ATI2565_6:
		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 | 0x2);
		/* FALL THROUGH */

	case ATI2564_6:
		outb(0x1ce, 0xb0);
		outb(0x1cf, 0x30);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb8);
		outb(0x1cf, (ati8 & 0x7f) | 0x40);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);
		break;

	case ATI2565_8:
		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 & 0xfd);
		/* FALL THROUGH */

	case ATI2564_8:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x38);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);
		break;

	case EDGE256:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x26);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);
		break;

	case ATIPLUS256_400:
	case ATIPLUS256_6:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x20);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb6);
		outb(0x1cf, ati6 | 0x4);

		outb(0x1ce, 0xb8);
		outb(0x1cf, (ati8 & 0x7f) | 0x40);

		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 | 0x2);
		break;

	case ATIPLUS256_8:
		outb(0x1ce, 0xb0);
		outb(0x1cf, ati0 | 0x20);

		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xe7) | 0x10);

		outb(0x1ce, 0xb6);
		outb(0x1cf, ati6 | 0x4);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 & 0xef);

		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 & 0xfd);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	
	

/*
 *	ati_restore(mode)	-- restore an ATI VGA Wonder board from
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
ati_restore(mode)
int mode;
{

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x1ce, 0xbe);
	outb(0x1cf, atie);

	outb(0x1ce, 0xb9);
	outb(0x1cf, ati9);

	outb(0x1ce, 0xb8);
	outb(0x1cf, ati8);

	outb(0x1ce, 0xb6);
	outb(0x1cf, ati6);

	outb(0x1ce, 0xb3);
	outb(0x1cf, ati3);

	outb(0x1ce, 0xb2);
	outb(0x1cf, ati2);

	outb(0x1ce, 0xb0);
	outb(0x1cf, ati0);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}


/*
 * ati_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
ati_selectpage(j)
register unsigned long j;
{
	v256_endpage = j | 0xffff;
	j >>= 16;
	if (j == v256_page)
		return;

	v256_page = j;

	outb(0x1ce, 0xb2);
	outb(0x1cf, (ati_page & 0xe1) | (j << 1));
}

#ifdef NEW
/*
 * to go into high res modes, the extended registers have to be initialized
 * the data is different depending on the combination of the monitor and
 * resolution.
 * If you are wondering why the registers are not in numerical ascending
 * order, We are not sure; the vendors tech and prog ref manuals have these
 * in the same order; it's easy for us to copy the register data 'AS IS'
 */
static unchar	ati0;
static unchar	ati1;
static unchar	atie;
static unchar	ati5;
static unchar	ati6;
static unchar	ati8;
static unchar	ati3;
static unchar	ati9;

static unchar	ati_page;
static unchar	ati2;
/*
 * mode 62 : 640x480 256 colors
 * mode 63 : 800x600 256 colors
 * mode 64 : 1024x768 256 colors
 *
 * the CRTC data for mode 63 and 64 are the same according to ATI
 *
 * monitors supported : STDVGA, 72Hz, 60Hz, 8514, TVM2A, 1430
 */
#define STD_62 		0
#define STD_63 		1
#define MSYNC72_63	2
#define MSYNC60_63	3
#define TVM2A_63	4
#define x1430_63	5
#define x8514_63	6
#define STD_64		7	
#define MSYNC72_64	8	

/*
 * MAX_MODES should be the total number of the above lines. If you add a new
 * mode, increment this line
 */
#define MAX_MODES	8

struct v256_regs inittab[] = {
/* Type 0 (STD_62):  ATIXL 640x480 STDVGA mode: 62 */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 1 (STD_63):  ATIXL 800x600 STDVGA mode: 63 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x8a,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x8e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 2 (MSYNC72_63):  ATIXL 800x600 for 72Hz monitors: 63 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x8a,
	/* misc */
	0xef,
	/* CRTC */
	0x7e, 0x63, 0x65, 0x9d, 0x6c, 0x9b, 0x4b, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3d, 0x80, 0x2b, 0x32, 0x0f, 0x33, 0x36, 0xe7, 0xff,

/* Type 3 (MSYNC60_63):  ATIXL 800x600 for 60Hz monitors: 63 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x8a,
	/* misc */
	0xef,
	/* CRTC */
	0x7f, 0x63, 0x65, 0x9d, 0x6b, 0x9c, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2c, 0x8e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 4 (TVM2A_63):  ATIXL 800x600 for TVM2A monitor: 63 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x8a,
	/* misc */
	0xef,
	/* CRTC */
	0x74, 0x63, 0x64, 0x89, 0x66, 0x91, 0x35, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2b, 0x8c, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 5 (x1430_63):  ATIXL 800x600 for 1430 monitor */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x8a,
	/* misc */
	0xef,
	/* CRTC */
	0x7c, 0x63, 0x64, 0x80, 0x65, 0x69, 0xbc, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x62, 0x9e, 0x57, 0x32, 0x0f, 0x59, 0xb0, 0xe3, 0xff,

/* Type 6 (x8514_63):  ATIXL 800x600 for 8514 monitor: 63 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x8a,
	/* misc */
	0xef,
	/* CRTC */
	0x80, 0x63, 0x65, 0x8d, 0x70, 0x80, 0xbc, 0x0f,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x62, 0x9e, 0x57, 0x32, 0x0f, 0x59, 0xb0, 0xe3, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern atixl_init ();
extern atixl_rest ();
extern atixl_selectpage ();

/*
 * various resolutions and monitor combinations supported by ATI Wonder PLUS 
 * Try to keep the most commonly used combination in the begining of this
 * array; it will speed up the start up time, not much but anything helps....
 */
/*
 * design flaw: some special regs are being init'd in vtio_dyn.c (or vtio.c)
 * this is unfortunate; to changes this will touch lots of files, so for now,
 * live with this; 
 *	HACK:
 * 	graphics[5], and attributes[0x10] are set in vtio_dyn.c; these regs are
 *	set if the first 5 chars in vtinfo->entry matches "ATIXL"
 *	So, make sure the first 5 chars in the entry are "ATIXL",
 *	ex: ATIXLa or ATIXLbbb are valid, but TMPATIXL is not valid
 * 	jan 92
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
#ifdef NOTREADY
	"ATIXL", "MULTISYNC", STD_63, 1024, 768, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[STD_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "MULTISYNC72", MSYNC72_63, 1024, 768, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[MSYNC72_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "MULTISYNC60", MSYNC60_63, 1024, 768, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[MSYNC60_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "TVM2A", TVM2A_63, 1024, 768, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[TVM2A_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "1430", x1430_63, 1024, 768, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[x1430_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "8514", x8514_63, 1024, 768, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[x8514_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "MULTISYNC", STD_63, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[STD_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "MULTISYNC72", MSYNC72_63, 800,600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[MSYNC72_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "MULTISYNC60", MSYNC60_63, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[MSYNC60_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "TVM2A", TVM2A_63, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[TVM2A_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "1430", x1430_63, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[x1430_63]),
	atixl_init, atixl_rest, atixl_selectpage,

	"ATIXL", "8514", x8514_63, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab[x8514_63]),
	atixl_init, atixl_rest, atixl_selectpage,
#endif

	"ATIXL", "STDVGA", STD_62, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab[STD_62]),
	atixl_init, atixl_rest, atixl_selectpage,
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

/*
 *	atixl_init(mode)	-- initialize an ATI VGA Wonder board into
 *		one of it's "extended" modes.  This takes care
 *		of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
atixl_init(mode)
int mode;
{
    static int inited = 0;

    static char extregs [7][8] = {
	{0x20, 0x00, 0x10, 0x00, 0x04, 0x40, 0x00, 0x02}, /* 0 STD_62 */
	{0x20, 0x00, 0x10, 0x00, 0x04, 0x00, 0x00, 0x00}, /* 1 STD_63 */
	{0x20, 0x00, 0x10, 0x00, 0x04, 0x00, 0x00, 0x00}, /* 2 MSYNC72_63 */
	{0x20, 0x00, 0x10, 0x00, 0x04, 0x00, 0x00, 0x00}, /* 3 MSYNC60_63 */
	{0x20, 0x40, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00}, /* 4 x1430_63 */
	{0x20, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00}, /* 5 TVM2A_63 */
	{0x20, 0x40, 0x12, 0x00, 0x04, 0x00, 0x00, 0x00}, /* 6 x8514_63 */
    };

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	/* enable ATI ports */
	if (geteuid () == 0) {
	  if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x1ce) == -1) ||
	    (ioctl(vt_fd, KDADDIO, (unsigned short) 0x1cf) == -1) ||
	    (ioctl(vt_fd, KDENABIO) == -1)) {
		ErrorF("Can't enable ATI extensions\n");
		return;
	  }
	}

	if (!inited) {
		outb(0x1ce, 0xb0);
		ati0 = inb(0x1cf);

		outb(0x1ce, 0xb1);
		ati1 = inb(0x1cf);

		outb(0x1ce, 0xbe);
		atie = inb(0x1cf);

		outb(0x1ce, 0xb5);
		ati5 = inb(0x1cf);

		outb(0x1ce, 0xb6);
		ati6 = inb(0x1cf);

		outb(0x1ce, 0xb8);
		ati8 = inb(0x1cf);

		outb(0x1ce, 0xb3);
		ati3 = inb(0x1cf);

		outb(0x1ce, 0xb9);
		ati9 = inb(0x1cf);

		outb(0x1ce, 0xb2);
		ati2 = inb(0x1cf);

		inited = 1;
	}

	ati_page = ati2;

#ifdef NOTNOW
{
int i;

for (i=0; i<7; i++)
{
  printf ("number, %d %4x\n", i, extregs[mode][i]);
}
}
#endif

	if ( mode < MAX_MODES ) {
		outb (0x1ce, 0xb0);
		outb (0x1cf, ati0 | extregs[mode][0]);

		outb (0x1ce, 0xb1);
		outb (0x1cf, (ati1 & 0x87) | extregs[mode][1]);

		outb (0x1ce, 0xbe);
		outb (0x1cf, (atie & 0xe5) | extregs[mode][2]);

		outb (0x1ce, 0xb5);
		outb (0x1cf, (ati5 & 0x7f) | extregs[mode][3]);

		outb (0x1ce, 0xb6);
		outb (0x1cf, (ati6 & 0xe2) | extregs[mode][4]);

		outb (0x1ce, 0xb8);
		outb (0x1cf, (ati8 & 0x3f) | extregs[mode][5]);

		outb (0x1ce, 0xb3);
		outb (0x1cf, (ati3 & 0xaf) | extregs[mode][6]);

		outb (0x1ce, 0xb9);
		outb (0x1cf, (ati9 & 0xfd) | extregs[mode][7]);
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	
/*
 *	atixl_rest(mode)	-- restore an ATI VGA Wonder board from
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
atixl_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x1ce, 0xb0);
	outb(0x1cf, (ati0 & 0xd1));

	outb(0x1ce, 0xb1);
	outb(0x1cf, ati1);

	outb(0x1ce, 0xbe);
	outb(0x1cf, atie);

	outb(0x1ce, 0xb5);
	outb(0x1cf, ati5);

	outb(0x1ce, 0xb6);
	outb(0x1cf, ati6);

	outb(0x1ce, 0xb8);
	outb(0x1cf, ati8);

	outb(0x1ce, 0xb3);
	outb(0x1cf, ati3);

	outb(0x1ce, 0xb9);
	outb(0x1cf, ati9);

	outb(0x1ce, 0xb2);
	outb(0x1cf, ati2);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}

#endif /*NEW */
