/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/INIT/et4k_256i.c	1.10"

/*
 *	Copyright (c) 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#ifndef VGA_PAGE_SIZE 
#define VGA_PAGE_SIZE 	(64 * 1024)
#endif

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

#define SET_KEY()    { \
	outb(0x3bf, 3);\
	if (v256_is_color)\
		outb(0x3d8, 0xa0);\
	else\
		outb(0x3b8, 0xa0);\
}

/*
 * This table has the mode specific data
 */
struct v256_regs inittab256[] = { 	/* V256 register initial values */
/* Type 0, Tseng Labs ET4000 VGA 1024x768 256 colors non-interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x04, 0x89, 0x9f, 0x26, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff,

/* Type 1, Tseng Labs ET4000 VGA 1024x768 256 colors, interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x27,
	/* CRTC */
	0x99, 0x7f, 0x7f, 0x1d, 0x83, 0x17, 0x2f, 0xf5,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0xff, 0x80, 0x60, 0xff, 0x30, 0xab, 0xff,

/* Type 2, Tseng Labs ET4000 VGA 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* 
	 * Changed this from 0xEF Hsync and Vsync polarity was changed
	 * Vsync + and Hsync -.  for 400 lines.
	 */   
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff,

/* Type 3, Tseng Labs ET4000 VGA 800x600 256 colors alternate */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xeb,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9c, 0x78, 0xf0,
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 4, Tseng Labs ET4000 VGA 800x600 256 colors CRYSTALSCAN monitor */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x6b,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9c, 0x78, 0xf0,
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 5, Tseng Labs ET4000 VGA 800x600 256 colors for interlaced monitors */ 
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x27,
	/* CRTC */
	0x7a, 0x64, 0x64, 0x1d, 0x70, 0x9a, 0x78, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 6, Tseng Labs ET4000 VGA 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* select clock 0( 25 MHz), enable RAM, I/O select, Odd/even mode, 
	 * horizontal and vetical retrace polality  
	 */
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xFF,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

int  et4k_init();
int  et4k_restore();
int  et4k_selectpage();
int  vt_is_ET4000;

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
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"SPEEDSTAR","NEC5D",ET4000256_1ni,1024,768,NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[0]),
	et4k_init, et4k_restore, et4k_selectpage,

	"SPEEDSTAR","INTERLACED", ET4000256_1, 1024,768, NULL,12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[1]),  
	et4k_init, et4k_restore, et4k_selectpage,

	"SPEEDSTAR","NEC5D", ET4000256_8, 800,600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[2]),
	et4k_init, et4k_restore, et4k_selectpage,

	"SPEEDSTAR","MULTISYNC",ET4000256_8a, 800,600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[3]),
	et4k_init, et4k_restore, et4k_selectpage,

	"SPEEDSTAR","CRYSTALSCAN",ET4000256_1ni, 800,600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[4]),
	et4k_init, et4k_restore, et4k_selectpage,

	"SPEEDSTAR","INTERLACED", ET4000256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[5]),
	et4k_init, et4k_restore, et4k_selectpage,

	"SPEEDSTAR","STDVGA", ET4000256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[6]),
	et4k_init, et4k_restore, et4k_selectpage,

	/*
	 * entries for ORCHID Prodesigner II
	 */
	"PRODESII","NEC5D",ET4000256_1ni,1024,768,NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[0]),
	et4k_init, et4k_restore, et4k_selectpage,

	"PRODESII","INTERLACED", ET4000256_1, 1024,768, NULL,12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[1]),  
	et4k_init, et4k_restore, et4k_selectpage,

	"PRODESII","NEC5D", ET4000256_8, 800,600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[2]),
	et4k_init, et4k_restore, et4k_selectpage,

	"PRODESII","MULTISYNC",ET4000256_8a, 800,600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[3]),
	et4k_init, et4k_restore, et4k_selectpage,

	"PRODESII","CRYSTALSCAN",ET4000256_1ni, 800,600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[4]),
	et4k_init, et4k_restore, et4k_selectpage,

	"PRODESII","INTERLACED", ET4000256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[5]),
	et4k_init, et4k_restore, et4k_selectpage,

	"PRODESII","STDVGA", ET4000256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[6]),
	et4k_init, et4k_restore, et4k_selectpage,
};

int	v256_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));
int 	v256_is_color;			/* true if on a color display */
struct	at_disp_info	vt_info;

unchar	*v256_maptbl;
unchar	v256_et4000_tbl[16]= {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
			     			  0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0};

extern	struct	kd_memloc vt_map;
extern	int	vt_fd;			/* file descriptor for the vt used */
extern	unchar	*screen_buffer;		/* pointer to saved video buffer */
extern	unchar	saved_misc_out;		/* need to save and restore this */
					/* because the kernel doesn't do */
					/* it right			 */

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

static unchar saved_crtc24_256;
static unchar saved_seg_sel; /* the memory copy of GDC segment select */
static unchar et4000256_seq_aux; 
static unchar et4000256_gdc_select;
static unchar et4000256_attr_misc; 
static unchar saved_crtc34_256; /* video system configuration 1 */
static unchar saved_crtc35_256; /* video system configuration 2 */
static unchar saved_rascas;	/* This register has been found to 
				 * to do the magic of performance 
				 * improvement */
/*
 * et4k_init: Initialize any vendor specific extended register 
 * initialization here
 *
 * Before initializing the extended registers, save the current state
 */
et4k_init(mode)
int mode;
{
	static int inited = 0;
	int saved_miscattr;
	unchar	temp;

	set_reg (0x3c4, 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {

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
		if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x3bf) == -1) ||
			(ioctl(vt_fd, KDENABIO) == -1)) {
		   ErrorF("Can't enable ET4000 extensions, KDADDIO Failed.\n");
		   ErrorF("Probable cause : User does not have permission for this operation.\n");
		   ErrorF("Try running as super user.\n");
		   return (FAIL);
		}
	}

		SET_KEY();
		v256_maptbl = v256_et4000_tbl;
		/*
		*  Save registers here. 
		*/
		et4000256_gdc_select = inb(0x3CD);
		get_reg(0x3c4, 0x07, &et4000256_seq_aux);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, et4000256_attr_misc);
		if (v256_is_color) {
			get_reg(0x3d4, 0x32, &saved_rascas);
			get_reg(0x3d4, 0x34, &saved_crtc34_256);
			get_reg(0x3d4, 0x35, &saved_crtc35_256);

		}
		else { 
			get_reg(0x3b4, 0x32, &saved_rascas);
			get_reg(0x3b4, 0x34, &saved_crtc34_256);
			get_reg(0x3b4, 0x37, &saved_crtc35_256);
		}
		inited = 1;
	}

	/* 
	 * If this is removed then , then wider display 
	 * Mode Control register index 0x10., Select graphics mode bit 1 
	 */

	(void)inb (vt_info.ad_addr + IN_STAT_1);  /* init flip-flop */
	out_reg (&regtab[I_ATTR], 0x10, 0x01);    /* Mode control -
												 Set graphics mode */
	SET_KEY();

	/*
	 *  ATC Miscellaneous register.
	 *      - Select high resolution > 45MHz (bit 4,5 -> 1,0 )
	 *      - Bypass internal pallette       ( bit 7 -> 1 ) 
	 */

	(void)inb (vt_info.ad_addr + IN_STAT_1);        /* init flip-flop */
	in_reg(&regtab[I_ATTR+1], 0x16,temp );          /* read into temp */ 
	(void)inb (vt_info.ad_addr + IN_STAT_1);        /* init flip-flop */
	out_reg (&regtab[I_ATTR+1], 0x16, 0x90) ;      /*old value-temp & 0x90*/

	SET_KEY();

	switch (mode) { 

	case ET4000256_6:
	case ET4000256_8:
	case ET4000256_8a:
			if (v256_is_color) {
				set_reg(0x3d4, 0x34, 0x08); /* 6845 Compatibility Code */
			    set_reg(0x3d4, 0x35, 0x00); /* Overflow */  
			}
			else {
				set_reg(0x3b4, 0x34, 0x08); /* 6845 Compatibiity code */ 
				set_reg(0x3b4, 0x35, 0x00); /* Overflow */
			}
			break;

	case ET4000256_1:
			if (v256_is_color) {
				set_reg(0x3d4, 0x34, 0x0a);
				set_reg(0x3d4, 0x35, 0x80);
			}
			else {
				set_reg(0x3b4, 0x34, 0x0a);
				set_reg(0x3b4, 0x35, 0x80);
			}
			break;

	case ET4000256_1ni:

		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		out_reg(&regtab[I_ATTR], 0x16, 0x90);
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x0a);
			set_reg(0x3d4, 0x35, 0x00);
		}
		else {
			set_reg(0x3b4, 0x34, 0x0a);
			set_reg(0x3b4, 0x35, 0x00);
		}
		break;

	 default:
		break;
	}

	/* 
	 * TS  index 07 - Auxilary Mode  
	 * VGA mode -bit 7
	 * Bios  ROM address 1
	 * Bit 2  - always  1 
	 */
	 /* The following values of RAS/CAS and Video Configuration 2 are 
	  * taken from register dump of system after coming up first time 
	  * Video Configuration 2 is a potentially dangerous register.  
	  */

	SET_KEY();
	set_reg(0x3d4, 0x32, 0x08); /* RAS/CAS */

	SET_KEY();
	switch(mode) {
	case ET4000256_1ni:
		set_reg(0x3c4, 0x07, 0xec);
		break;
	default:
		set_reg(0x3c4, 0x07, 0x8c);
		/*
		 * TS Auxillary mode, this register is protected by key 
		 * set VGA mode, BIOS ROM address map 1 
		 */
	}
	if (v256_is_color)
			outb(0x3D8, 0x29);
	else
			outb(0x3B8, 0x29);

	set_reg(0x3c4, 0, SEQ_RUN);		/* start seq */
}

/*
 * Restore all the extended registers that were initialized in et4k_init
 */
et4k_restore(mode)
int mode;
{
	int	temp;

	set_reg(0x3c4, 0, SEQ_RESET);			/* reset sequencer */
	set_reg(0x3c4, 0x07, et4000256_seq_aux);
	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	SET_KEY();
	out_reg(&regtab[I_ATTR], 0x16, et4000256_attr_misc);

	if (v256_is_color) {
		SET_KEY();
		set_reg(0x3d4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3d4, 0x34, saved_crtc34_256);
		SET_KEY();
		set_reg(0x3d4, 0x35, saved_crtc35_256);
	}
	else {
		SET_KEY();
		set_reg(0x3b4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3b4, 0x34, saved_crtc34_256);
		SET_KEY();
		set_reg(0x3b4, 0x35, saved_crtc35_256);
	}

	set_reg(0x3c4, 0x07, 0x8c);
	outb (0x3CD, et4000256_gdc_select);		/* reset segment */
	set_reg (0x3c4, 0, SEQ_RUN);			/* start sequencer */
}

/*
 * et4k_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 *
 * This routine was changed to keep the memory value of segment select 
 * register. There was no perceptible in performance. 
 */

et4k_selectpage(j)
unsigned long j;
{
	v256_page = (j >> 16) & 0x0F;
#ifdef MEMCP_SEG
	if ( v256_page ^ (saved_seg_sel & 0x0F ))  {
#endif
	    v256_page = v256_maptbl[v256_page] | v256_page;
		outb(0x3cd, v256_page);
		v256_endpage = j | 0xffff;
#ifdef MEMCP_SEG
		saved_seg_sel = v256_page;
	}
#endif

}



/*
 *	set_reg(address, index, data)	-- set an index/data I/O register
 *					pair being careful to wait between
 *					the out() instructions to allow
 *					the cycle to finish.
 *
 *	Input:
 *		int	address		-- address of register pair
 *		int	index		-- index of register to set
 *		int	data		-- data to set
 */
static
set_reg(address, index, data)
int address;
int index;
int data;
{
	outb(address, index);
	asm("	jmp	.+2");		/* flush cache */
	outb(address+1, data);
	asm("	jmp	.+2");		/* flush cache */
}

	
/*
 *	get_reg(address, index, data)	-- get a register value from an 
 *					index/data I/O register pair being 
 *					careful to wait between the I/O
 *					instructions to allow the cycle 
 *					to finish.
 *
 *	Input:
 *		int	address		-- address of register pair
 *		int	index		-- index of register to set
 *		int	*data		-- place to put data
 */
static
get_reg(address, index, data)
int address;
int index;
int *data;
{
	outb(address, index);
	asm("	jmp	.+2");		/* flush cache */
	*data = inb(address+1);
	asm("	jmp	.+2");		/* flush cache */
}
