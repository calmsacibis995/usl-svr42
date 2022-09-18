/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/INIT/leg_256i.c	1.2"

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

#undef LEGEND256_1ni
#undef LEGEND256_1
#undef LEGEND256_8
#undef LEGEND256_8a
#undef LEGEND256_6

#define LEGEND256_1ni	0
#define LEGEND256_1	1
#define LEGEND256_8	2
#define LEGEND256_8a	3
#define LEGEND256_6	4

/*
 * This table has the mode specific data
 */
struct v256_regs inittab256[] = { 	/* V256 register initial values */
/* Type 0, LEGEND 1024x768 256 colors non-interlaced */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x37,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x04, 0x8a, 0x9f, 0x26, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff,

/* Type 1, LEGEND 1024x768 256 colors interlaced */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x23,
	/* CRTC */
	0x99, 0x7f, 0x80, 0x1c, 0x81, 0x19, 0x2f, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x01, 0xff, 0x80, 0x60, 0x05, 0x2a, 0xab, 0xff,

/* Type 2, LEGEND 800x600 256 colors NEC5D */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x6b,
	/* CRTC */
	0x7d, 0x63, 0x64, 0x00, 0x6c, 0x1b, 0x9a, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6f, 0x05, 0x57, 0x64, 0x60, 0x5f, 0x95, 0xab, 0xff,

/* Type 3, LEGEND 800x600 256 colors multisync */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x7f, 0x63, 0x65, 0x9f, 0x70, 0x9d, 0x7f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0c, 0x57, 0x64, 0x60, 0x58, 0x73, 0xab, 0xff,

/* Type 4, LEGEND 640x480 256 colors stdvga */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xff,
};

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

int  legend_init();
int  legend_rest();
int  legend_selectpage();
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
 * legend_init is called to initialize the extended regs. Similary to restore
 * from the extended state, the restore function (legend_restore) is called.
 *
 * The extended register initialize and restore functions are in this file
 * and any custom extended register initialization and restoration should
 * be done in these two functions respectively.
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"LEGEND", "NEC5D", LEGEND256_1ni, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[LEGEND256_1ni]),
	legend_init, legend_rest, legend_selectpage,

	"LEGEND", "INTERLACED", LEGEND256_1, 1024, 768, NULL, 12*64*1024, 64*1024,
	0x3D4, 1024, &(inittab256[LEGEND256_1]),
	legend_init, legend_rest, legend_selectpage,

	"LEGEND", "NEC5D", LEGEND256_8, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[LEGEND256_8]),
	legend_init, legend_rest, legend_selectpage,

	"LEGEND", "MULTISYNC", LEGEND256_8a, 800, 600, NULL, 8*64*1024, 64*1024,
	0x3D4, 800, &(inittab256[LEGEND256_8a]),
	legend_init, legend_rest, legend_selectpage,

	"LEGEND", "STDVGA", LEGEND256_6, 640, 480, NULL, 5*64*1024, 64*1024,
	0x3D4, 640, &(inittab256[LEGEND256_6]),
	legend_init, legend_rest, legend_selectpage,
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
static unchar saved_crtc34; /* video system configuration 1 */
static unchar saved_crtc35; /* video system configuration 2 */
static unchar saved_rascas;	/* This register has been found to 
				 * to do the magic of performance 
				 * improvement */
static int    legend_clock30;
/*
 *	legend_init(mode)	-- initialize a Sigma VGA LEGEND to one
 *				of it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_init(mode)
int mode;
{
	static int inited = 0;
	volatile unchar temp;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		
		/* 
		 * Set "KEY" so we can get to all regs.
		 */
		SET_KEY();
		v256_maptbl = v256_et4000_tbl;
		et4000256_gdc_select = inb(0x3CD);
		get_reg(0x3c4, 0x07, &et4000256_seq_aux);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, et4000256_attr_misc);
		if (v256_is_color) {
			outb(0x3d8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x32, saved_rascas);
			in_reg(&regtab[I_EGACOLOR], 0x34, saved_crtc34);
			in_reg(&regtab[I_EGACOLOR], 0x35, saved_crtc35);
		}
		else {
			outb(0x3b8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x32, saved_rascas);
			in_reg(&regtab[I_EGACOLOR], 0x34, saved_crtc34);
			in_reg(&regtab[I_EGACOLOR], 0x37, saved_crtc35);
		}

		/*
		 * See which clock chip is in use on this board.
		 */
		legend_clock30 = saved_crtc34 & 0x50;
		inited = 1;
	}

	(void)inb (vt_info.ad_addr + IN_STAT_1);  /* init flip-flop */
	out_reg (&regtab[I_ATTR], 0x10, 0x01);    /* Mode control -
	/* Set graphics mode */
	SET_KEY();
        (void)inb (vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
        in_reg(&regtab[I_ATTR+1], 0x16,temp );   /* read into temp */
        (void)inb (vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
        out_reg (&regtab[I_ATTR+1], 0x16, 0x90); /*old value-temp & 0x90*/
	SET_KEY();

	switch(mode) {
	case LEGEND256_8:
		if (legend_clock30)
			legend_selclk(0xd);
		else {
			legend_selclk(0x5);
			vt_info.regs->miscreg = 0x63;
		}
		break;

	case LEGEND256_8a:
		legend_selclk(0x2);
		break;

	case LEGEND256_1:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x35, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x35, 0x80);
		}

		if (legend_clock30)
			legend_selclk(0xc);
		else {
			legend_selclk(0x3);
			vt_info.regs->miscreg = 0x2b;
		}
		break;

	case LEGEND256_1ni:
		if (legend_clock30) 
			legend_selclk(0xa);
		else {
			legend_selclk(0x5);
			vt_info.regs->miscreg = 0x3f;
		}
		set_reg(0x3c4, 0x07, 0xe8);
		(void)inb(vt_info.ad_addr + IN_STAT_1);
		out_reg(&regtab[I_ATTR], 0x16, 0x90);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	legend_rest(mode)	-- restore a Sigma VGA LEGEND from one
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x3CD, et4000256_gdc_select);		/* reset segment */
	set_reg(0x3c4, 0x07, et4000256_seq_aux);
	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x16, et4000256_attr_misc);

	if (legend_clock30)
		legend_selclk(0x3);
	else
		legend_selclk(0x2);

	if (v256_is_color) {
		SET_KEY();
		set_reg(0x3d4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3d4, 0x34, saved_crtc34);
		SET_KEY();
		set_reg(0x3d4, 0x35, saved_crtc35);
	}
	else {
		SET_KEY();
		set_reg(0x3b4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3b4, 0x34, saved_crtc34);
		SET_KEY();
		set_reg(0x3b4, 0x35, saved_crtc35);
	}

	set_reg (0x3c4, 0, SEQ_RUN);			/* start sequencer */
}
		

/*
 * 	legend_selclk(clk)	-- select all the clock based on the data
 *				passed in.
 *
 *	Input:
 *		BYTE clk	-- clock data:
 *					bit 0	= clksel
 *					bit 1	= cs2 (0 if set cs2)
 *					bit 2   = cs2 (1 if set cs2) 
 *					bit 3   = sense
 */
legend_selclk(clk)
BYTE clk;
{
	BYTE	misc_dat;

	/*
	 * Write clk sel and sense bits
	 */
	misc_dat = inb(MISC_OUT_READ) & 0xf3;
	misc_dat |= ((clk & 1) << 2) | (clk & 8);
	outb(MISC_OUT, misc_dat);
	legend_jtoggle();

	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, (clk & 2));
		out_reg(&regtab[I_EGACOLOR], 0x34, ((clk & 4) >> 1));
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, (clk & 2));
		out_reg(&regtab[I_EGAMONO], 0x34, ((clk & 4) >> 1));
	}
}


/*
 * legend_jtoggle()	-- perform the "J-Toggle" operation on a Sigma
 *			   VGA LEGEND board.  This is used to change the
 *			   meaning of the MISC_OUT register
 *
 */
legend_jtoggle()
{
	BYTE	crtc34;

	if (v256_is_color) {
		in_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
	}
	else {
		in_reg(&regtab[I_EGAMONO], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGAMONO], 0x34, crtc34);
	}
}

/*
 * legend_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 *
 * This routine was changed to keep the memory value of segment select 
 * register. There was no perceptible in performance. 
 */

legend_selectpage(j)
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
