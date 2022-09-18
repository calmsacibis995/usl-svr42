/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/INIT/et3k_256i.c	1.3"

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

#include <stdio.h>
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

static int x2debug =1;

/*	EXTERNALS */
extern	struct	kd_memloc vt_map;

extern	int	vt_fd;			/* file descriptor for the vt used */

extern	unchar	*screen_buffer;		/* pointer to saved video buffer */

/* 
 *	need to save and restore this
 * because the kernel doesn't do 
 * it right			 
 */
extern	unchar	saved_misc_out;		

/*
 *	need to do this one also! 
 */
extern	unchar	saved_rascas;

/* STATICS */
/*
 *	V256 REGISTER INITIAL VALUES 
 */
static struct v256_regs inittab[] = 
{ 
	/*
	 *	TYPE 0, ET3000 V256 640x480 256 COLORS
	 */
	{
		/*
		 *	sequencer
		 */
		{
			0x00, 0x01, 0x0F, 0x00, 0x06
		},
		/*
		 * misc 
		 */
		0xE3,
		/*
		 * CRTC 
		 */
		{
			 0x5F, 	/* 0 : horizontal total */
			 0x4F, 	/* 1 : horizontal display end */
			 0x50, 	/* 2 : start horizontal blank */
			 0x01, 	/* 3 : end horizontal blank */
			 0x55, 	/* 4 : start horizontal retrace */
			 0x9F, 	/* 5 : end horizontal retrace */
			 0x0B, 	/* 6 : vertical total */
			 0x3E,	/* 7 : overflow */
			 0x00, 	/* 8 : preset row scan */
			 0x40, 	/* 9 : max scan line */
			 0x00, 	/* 10 : cursor start */
			 0x00, 	/* 11 : cursor end */
			 0x00, 	/* 12 : start address H */
			 0x00, 	/* 13 : start address L */
			 0x00, 	/* 14 : cursor location H */
			 0x00,	/* 15 : cursor location L */
			 0xEB, 	/* 16 : vertical retrace start */
			 0x2D, 	/* 17 : vertical retrace end */
			 0xDF, 	/* 18 : vertical display end */
			 0x28, 	/* 19 : offset */
			 0x00, 	/* 20 : underline location */
			 0xE4, 	/* 21 : start vertical blank */
			 0x08, 	/* 22 : end vertical blank */
			 0xC3, 	/* 23 : mode control */
			 0xFF 	/* 24 : line compare */
		},
	},
	/* CHECK THESE VALUES:SHOULD BE REPLACED WITH THE ONES FROM THE MAN */
	/*
	 *	TYPE 1, ET3000 V256 800x600 256 COLORS
	 */
	{
		/*
		 *	sequencer 
		 */
		{
			0x00, 0x01, 0x0F, 0x00, 0x06
		},
		/*
		 * 	misc 
		 */
		0xE7,
		/*
		 *	CRTC 
		 */
		{
			0x7F,	/* 0 : horiz total */
			0x63,	/* 1 : horiz disp end */
			0x64,	/* 2 : start horiz blank */
			0x0d,	/* 3 : end horiz blank */
			0x68,	/* 4 : start horiz retrace */	
			0xbb,	/* 5 : end horiz retrace */
			0x78,	/* 6 : vertical total */
			0xF0,	/* 7 : overflow */
			/* 0x55,	 7 : overflow */
			0x00,	/* 8 : preset row scan */
			0x60,	/* 9 : max scan line */
			0x00,	/* 10 : cursor start */
			0x00,	/* 11 : cursor end */
			0x00,	/* 12 : start address H */
			0x00,	/* 13 : start address L */
			0x00,	/* 14 : cursor location H */
			0x00,	/* 15 : cursor location L */
			0x60,	/* 16 : vertical retrace start */
			0x22,	/* 17 : vertical retrace end */
			0x57,	/* 18 : vertical display end */
			0x32,	/* 19 : offset */
			0x00,	/* 20 : underline location */
			0x5b,	/* 21 : start vertical blank */
			0x76, 	/* 22 : end vertical blank */
			0xC3,	/* 23 : mode control */
			0xFF	/* 24 : line compare */
		}
#ifdef	KOSHY
		{
			/* MODE 3O */
			0x7F,	/* 0 : horiz total */
			0x63,	/* 1 : horiz disp end */
			0x64,	/* 2 : start horiz blank */
			0x02,	/* 3 : end horiz blank */
			0x64,	/* 4 : start horiz retrace */	
			0x17,	/* 5 : end horiz retrace */
			0x77,	/* 6 : vertical total */
			0xF0,	/* 7 : overflow */
			0x00,	/* 8 : preset row scan */
			0x00,	/* 9 : max scan line */
			0x00,	/* 10 : cursor start */
			0x00,	/* 11 : cursor end */
			0x00,	/* 12 : start address H */
			0x00,	/* 13 : start address L */
			0x00,	/* 14 : cursor location H */
			0x00,	/* 15 : cursor location L */
			0x60,	/* 16 : vertical retrace start */
			0x22,	/* 17 : vertical retrace end */
			0x57,	/* 18 : vertical display end */
			0x32,	/* 19 : offset */
			0x00,	/* 20 : underline location */
			0x5B,	/* 21 : start vertical blank */
			0x75, 	/* 22 : end vertical blank */
			0xC3,	/* 23 : mode control */
			0xFF	/* 24 : line compare */
		}
#endif	/* KOSHY */
	}
};

#ifdef NOTNOW
	0xEB,
	/* CRTC */
	0x7F, 0x63, 0x64, 0x02, 0x65, 0x12, 0x77, 0xF0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5E, 0x21, 0x57, 0x32, 0x00, 0x59, 0x6B, 0xC3, 0xFF,

	0xEF,
	/* CRTC */
	0x8E, 0x63, 0x64, 0x11, 0x6F, 0x08, 0x77, 0xF0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x60, 0xA2, 0x57, 0x32, 0x40, 0x5b, 0x75, 0xC3, 0xFF,
#endif
extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

extern et3k_v256_init ();
extern et3k_v256_rest ();
extern et3k_v256_selectpage ();

/* 
 * 	display info for support adapters 
 */

struct	at_disp_info	disp_info[] = 
{	
	{
		"ET3000",		/* string type */
		STB_6x4_256,	/* define type */
		640,			/* pixels per scanline */
		480,			/* number of scanlines */
		NULL,			/* screen mem virtual address */
		5*64*1024,		/* size of screen memory */
		64*1024,		/* size of one plane */
		0x3D4, 			/* base register address */
		640,			/* number of bytes per scanline */
		&(inittab[0]),	/* v256 registers for mode */
		et3k_v256_init,	/* ext_init() */
		et3k_v256_rest,	/* ext_restore() */
		et3k_v256_selectpage	/* ext_selectpage */
	},
	{
		"ET3000",		/* string type */
		STB_8x6_256,	/* define type */
		800,			/* pixels per scanline */
		600,			/* number of scanlines */
		NULL,			/* screen memory virtual address */
		8*64*1024,		/* size of screen memory */
		64*1024,		/* size of a page */
		0x3D4,			/* base register address */
		800,			/* number of bytes per scanline */
		&(inittab[1]),	/* v256 registers for mode */
		et3k_v256_init,	/* ext_init */
		et3k_v256_rest,	/* ext_restore */
		et3k_v256_selectpage,	/* ext_selectpage */
	}
};

/*
 *	The number of supported resolutions
 */
int     v256_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));

/*
 *	true if on a color display 
 */
int     v256_is_color;   

/*
 * Global vt_info structure 
 */
struct  at_disp_info    vt_info;


/*
 *	this table helps us duplicate the read / write segment indices. We
 *	can't directly do a shift operation as for some VGA chipset's segment
 *	selection is really wierd !
 */

unchar	v256_et3000_tbl[8] = {0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78};

/*	
 *	pointer to segment map table above
 */
unchar	*v256_maptbl;

/*
 * Table giving the information needed to initialize the V256 registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
struct reginfo	regtab[] = 
{
	{ 16, 0x3b4, 0x3b5 },	/* m6845init, monochrome */
	{ 16, 0x3d4, 0x3d5 },	/* m6845init, color/graphics */
	{ 25, 0x3b4, 0x3b5 },	/* v256init, monochrome */
	{ 25, 0x3d4, 0x3d5 },	/* v256init, color */
	{ NSEQ, 0x3c4, 0x3c5 },	/* seqinit */
	{ NGRAPH, 0x3ce, 0x3cf },/* graphinit */
	{ NATTR, 0x3c0, 0x3c0 }, /* attrinit */
	{ NATTR, 0x3c0, 0x3c0 } /* attrinit */
};

static unchar tli256_seq_aux; 
static unchar tli256_attr_misc; 
static unchar tli256_crt_misc;
static unchar tli256_gdc_select;
static unchar saved_crtc24_256;
static unchar sequencer_zoom_control;

static unchar saved_crtc23_256;
static unchar saved_crtc24_256;
static unchar saved_crtc25_256;
static unchar saved_ATC_36;


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
	
static int inited = 0;

/*
 *	et3k_v256_init(mode)	-- initialize a Tseng Labs V256 board to one
 *				of it's "extended" modes.  This takes care
 *				of non-standard V256 registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
int
et3k_v256_init(mode)
int mode;
{
	int saved_miscattr;
	unchar	temp;

#ifdef  DEBUG
	  if (x2debug)	 
	    fprintf (stderr, "in init\n");
#endif
	
printf("et3k_v256_init : mode = %d\n", mode);

	set_reg(0x3c4, 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		/* enable port # 3bf ; need this to enable KEY */
	    if (geteuid () == 0) {
		if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x3bf) == -1) ||
					(ioctl(vt_fd, KDENABIO) == -1)) 
		{
			ErrorF("Can't enable ET3000 extensions.\n");
			return;
		}
	    }

		/*
		 * Set "KEY" so we can get to all regs.
		 */
		outb(0x3BF, 3);

		/*
		 * What Tseng chip am I running on ???
		 */
		outb(0x3D8, 0xA0);
		(void)inb(0x3DA);
		outb(0x3C0, 0x36);
		saved_miscattr = inb(0x3C1);
		outb(0x3C0, 0x90);

		outb(0x3C0, 0x36);


		v256_maptbl = v256_et3000_tbl;
		outb(0x3C0, saved_miscattr);

		tli256_gdc_select = inb(0x3CD);
		get_reg(0x3c4, 0x07, &tli256_seq_aux);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, tli256_attr_misc);
		if (v256_is_color) 
		{
			get_reg(0x3d4, 0x25, &tli256_crt_misc);
		}
		else 
		{
			get_reg(0x3b4, 0x25, &tli256_crt_misc);
		}

		get_reg(0x3c4, 0x06, &sequencer_zoom_control); /* zoom control */
		inited = 1;
	}

	set_reg(0x3c4, 0x06,0x00 );/* zoom control */

	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */

	in_reg(&regtab[I_ATTR+ 1], 0x36, saved_ATC_36);

    	(void)inb (vt_info.ad_addr + IN_STAT_1); /* init flip-flop */ 
	out_reg (&regtab[I_ATTR], 0x36, 0x10); 

	switch(mode) 
	{
	case STB_8x6_256:
		outb(0x3BF, 3);
		if (v256_is_color) 
		{
			outb(0x3D8, 0xA0);
		}
		else 
		{
			outb(0x3B8, 0xA0);
		}
		/* 
		 * set external clock. We do some weird stuff here to 
		 * really get 16 bit writes turned on.  (set TS7 to an
		 * initial value of 8, set bit 3 of MISC_OUT,  and then 
		 * toggle CRTC24.)
		 */
		set_reg(0x3c4, 0x07, 0x48);	/* V256 reg/32K ROM */
		outb(MISC_OUT, saved_misc_out|8);/* Set misc register */

		set_reg(0x3c4, 0, SEQ_RUN);	/* start seq */
		set_reg(0x3c4, 0, SEQ_RESET);	/* stop seq */

		if (v256_is_color) 
		{
			get_reg(0x3d4, 0x23, &saved_crtc23_256);
			get_reg(0x3d4, 0x24, &saved_crtc24_256);
			get_reg(0x3d4, 0x25, &saved_crtc25_256);

			set_reg(0x3d4, 0x23, 0xab);
			set_reg (0x3d4, 0x24, saved_crtc24_256|2);
			set_reg (0x3d4, 0x25, 0x10);

		} 
		else 
		{
			get_reg (0x3b4, 0x23, &saved_crtc23_256);
			get_reg (0x3b4, 0x24, &saved_crtc24_256);
			get_reg (0x3b4, 0x25, &saved_crtc25_256);

			set_reg(0x3b4, 0x23, 0xab);
			set_reg (0x3b4, 0x24, saved_crtc24_256|2);
			set_reg (0x3b4, 0x25, 0x10);

		}
		outb(MISC_OUT, saved_misc_out);	/* Reset misc register */
		/* FALL THROUGH */

	case STB_6x4_256:
		/* 
		set_reg(0x3c4, 0x07, 0xa8);	 V256 reg/32K ROM 
		 */
		set_reg(0x3c4, 0x07, 0x88);	/* V256 reg/32K ROM */

		(void)inb (vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		out_reg(&regtab[I_ATTR], 0x16, 0x10);
		break;
	}

	if (v256_is_color)
	{
	    outb(0x3D8, 0x29);
	}
	else
	{
	    outb(0x3B8, 0x29);
	}

	set_reg(0x3c4, 0, SEQ_RUN);		/* start seq */

}

/*
 *	et3k_v256_rest(mode)  	-- restore a Tseng Labs V256 board from one
 *			  	of it's "extended" modes.  This takes care
 *			  	of non-standard V256 registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
et3k_v256_rest(mode)
int mode;
{
	int	temp;
#ifdef  DEBUG
	fprintf (stderr, "in rest\n");
#endif

	set_reg(0x3c4, 0, SEQ_RESET);			/* reset sequencer */
	set_reg(0x3c4, 0x07, tli256_seq_aux);
	outb(0x3CD, tli256_gdc_select);			/* reset segment */
	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x16, tli256_attr_misc);
	outb(MISC_OUT, saved_misc_out & ~IO_ADDR_SEL);
	outb(0x3BF, 0x03);
	outb(MISC_OUT, saved_misc_out);
	if (v256_is_color)
	{
		set_reg(0x3d4, 0x25, tli256_crt_misc);
	}
	else
	{
		set_reg(0x3b4, 0x25, tli256_crt_misc);
	}

	set_reg(0x3b4, 0x23, saved_crtc23_256 );
	SET_KEY();	
	if ((mode != TLI_6x4_256) && (mode != STB_6x4_256)) 
	{
		if (v256_is_color) 
			set_reg(0x3d4, 0x24, saved_crtc24_256);
		else 
			set_reg(0x3b4, 0x24, saved_crtc24_256);
	}

	set_reg(0x3c4, 0, SEQ_RUN);		/* start sequencer */
}

/*
 * et3k_v256_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */

int
et3k_v256_selectpage(j)
unsigned long j;
{

#ifdef	DELETE
printf("et3k_v256_selectpage: page %d\n", j);
#endif	/* DELETE */

	
#ifdef DELETE
	v256_page = j >> 16;
#endif
	v256_page = j / VGA_PAGE_SIZE;

	/*
	 * duplicate read/write segment pointers
	 */
	v256_page = v256_maptbl[v256_page] | v256_page;

	/*
	 *	write page indices to segment select register
	 */
	outb(0x3cd, v256_page);

	/*
	 *	 store in global 
	 */
	v256_endpage = j | VIDEO_PAGE_MASK;

}

/*
 * In order to test what the bottleneck in the VGA display is
 * the following two functions were written.
 */

int
v256_reset_sequencer()
{
	unsigned int	temp;
	get_reg(0x3C4, 1, &temp);
	set_reg(0x3C4, 1, temp & ~32u);
}

int
v256_start_sequencer()
{
	unsigned int	temp;
	get_reg(0x3C4, 1, &temp);
	set_reg(0x3C4, 1, temp | 32u);

}
