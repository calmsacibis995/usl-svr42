/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/INIT/et4k_16i.c	1.8"

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

/* Type 1, ET4000 800x600 16 colors : data from Tseng Labs ET4K data book */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
        0xef,
        /* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x64, 0x17, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x60, 0x82, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xc3, 0xff,

/* Type 2, ET4000 800x600 16 colors : alternate data for multisync monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
        0xe3,
        /* CRTC */
        0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9a, 0x78, 0xf0,
        0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x5c, 0x0f, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xc3, 0xff,

/* Type 3, ET4000 800x600 16 colors : for dual freq monitors(ex:crystal scan NI)
 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x6b,
	/* CRTC */ 
        0x7b, 0x63, 0x64, 0x1e, 0x6a, 0x93, 0x6f, 0xf0,
        0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x59, 0x8b, 0x57, 0x32, 0x00, 0x5b, 0x6c, 0xc3, 0xff,

/* Type 4, ET4000 800x600 16 colors :
 *		alternate entry for dual freq monitors(ex:crystal scan NI)
 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x6b,
	/* CRTC */ /* Crystal Scan NI, dual freq monitor */
        0x7d, 0x63, 0x64, 0x01, 0x6a, 0x19, 0x98, 0xf0,
        0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x6a, 0x80, 0x57, 0x32, 0x00, 0x59, 0x7d, 0xc3, 0xff,

/* Type 5, ET4000 800x600 16 colors : generic entry for dual freq monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */ /* Crystal Scan NI, dual freq monitor */
        0x7b, 0x63, 0x64, 0x1e, 0x6a, 0x93, 0x6f, 0xf0,
        0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x59, 0x8b, 0x57, 0x32, 0x00, 0x5b, 0x6c, 0xc3, 0xff,
};

unchar attributes[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x01, 0x00, 0x0f, 0x00, 0x00,
};

unchar graphics[] = {	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff,
};

/*
 * The base address of the adapters based on the type returned by KDDISPTYPE.
 * The 386/ix 2.0 changed the semantices of KDDISPTYPE sturcture. So we now
 * have to use these hard coded physical address values for the console and
 * use the values returned by KDDISPTYPE for other displays. The console is
 * identified by doing a KIOCINFO which returns ('k' << 8) for the console.
 */

long base_addr[] = {
	0, MONO_BASE, MONO_BASE, COLOR_BASE, EGA_BASE, VGA_BASE
};
	
extern et4k_init();
extern et4k_rest();
extern no_ext ();

#define MSYNC		1
#define MSYNCa		2
#define CSCAN		3
#define CSCANa		4
#define DUALFREQ	5

/*
 * The VGA entry that supports 640x480 is standard on all boards, so this
 * entry should be present for all individual drivers.
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"VGA", "STDVGA", VT_VGA, 1, 0,640,480,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[0]),

	"SPEEDSTAR", "NEC5D", MSYNC,1, 0,800,600,4,16,NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[MSYNC]),

	"SPEEDSTAR", "MULTISYNC", MSYNCa,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[MSYNCa]),

	"SPEEDSTAR", "CRYSTALSCAN", CSCAN,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[CSCAN]),

	"SPEEDSTAR", "CRYSTALSCANa", CSCANa,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[CSCANa]),

	"SPEEDSTAR", "DUALFREQ", DUALFREQ,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[DUALFREQ]),

	/*
	 * entries for ORCHID PRODESIGNER II, IIs
	 */
	"PRODESII", "NEC5D", MSYNC,1, 0,800,600,4,16,NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[MSYNC]),

	"PRODESII", "MULTISYNC", MSYNCa,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[MSYNCa]),

	"PRODESII", "CRYSTALSCAN", CSCAN,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[CSCAN]),

	"PRODESII", "CRYSTALSCANa", CSCANa,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[CSCANa]),

	"PRODESII", "DUALFREQ", DUALFREQ,1,0,800,600,4,16,NULL,256*1024,64*1024,
	0x3d4, 100, GR_MODE, et4k_init, et4k_rest, &(inittab[DUALFREQ]),
};

int 	vga_is_color;			/* true if on a color display */
extern unchar	saved_misc_out;		/* need to save and restore this */
static unchar et4000_34;
extern int	vt_fd;			/* file descriptor for the vt used */
struct	at_disp_info	vt_info;
int	vt_allplanes;
int	vga_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));

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


/*
 *	et4k_init(mode)	-- initialize a ET4000 based VGA board to one
 *			of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
unsigned char vga_et4_saved34;
et4k_init(mode)
int mode;
{
	static int inited = 0;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
	    inited = 1;

	/*
	 * Prior to SVR4.1 ES, you have to be root to write to 0x3bf port;
	 * to be backward compatible, check the effective userid and allow the
	 * initialization, only if the effective-userid is root; the side
	 * effect of this is the user wouldn't see the following error message
	 * in pre-SVR4ES, if he/she tries to run the server as non-root
	 * The server core dumps with the following msg:
	 *   Memory fault(coredump)
	 */
	if (geteuid () == 0) {
		/*
		 * Set "KEY" so we can get to all regs.
		 */
		/* enable port # 3bf ; need this to enabe KEY */
		if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x3bf) == -1) ||
					(ioctl(vt_fd, KDENABIO) == -1)) {
			ErrorF("Can't enable ET4000 extensions, KDADDIO Failed.\n");
			ErrorF("Probable cause : User does not have permission for this operation.\n");
			ErrorF("Try running as super user.\n");
			return (FAIL);
		}
	}

		outb(0x3bf, 3);
		if (vga_is_color) {
			outb(0x3d8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x34, vga_et4_saved34);
		}
		else {
			outb(0x3b8, 0xa0);
			in_reg(&regtab[I_EGAMONO], 0x34, vga_et4_saved34);
		}
	}

	if (vga_is_color) {
	   outb(0x3d8, 0xa0);

	   switch (mode) {
		case MSYNC:
		case MSYNCa:
		     outb (0x3d4, 0x34);
		     outb (0x3d5, 0x08);
		     break;
		case CSCAN:
		case CSCANa:
		case DUALFREQ:
		     outb (0x3d4, 0x34);
		     outb (0x3d5, 0x0a);
		     break;
		default:
		     outb (0x3d4, 0x34);
		     outb (0x3d5, 0x0a);
		     break;
	   }
	}
	else {
	   outb(0x3b8, 0xa0);
	   switch (mode) {
		case MSYNC:
		case MSYNCa:
		     outb (0x3b4, 0x34);
		     outb (0x3b5, 0x08);
		     break;
		case CSCAN:
		case CSCANa:
		case DUALFREQ:
		     outb (0x3b4, 0x34);
		     outb (0x3b5, 0x0a);
		     break;
		default:
		     outb (0x3b4, 0x34);
		     outb (0x3b5, 0x0a);
		     break;
	   }
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */
	return (SUCCESS);
}

/*
 *	et4k_rest(mode)	-- restore a ET4000 VGA from one
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
et4k_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, vga_et4_saved34);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, vga_et4_saved34);
	}
		
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
	return (SUCCESS);
} 
