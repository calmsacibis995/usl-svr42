/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vtio_dyn.c	1.6"

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
#include "Xmd.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "sys/vt.h"
#include "sys/inline.h"
#include "vgaregs.h"

#ifdef DEBUG
extern int xdebug;
#endif

extern char * AllocSaveScreen ();
extern void FreeSaveScreen ();

extern struct vga_regs inittab[];
/*****
extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];
*****/
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
	
extern struct at_disp_info disp_info[];

/* int	vga_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info)); */
extern int	vga_num_disp;
extern struct	at_disp_info	vt_info;
extern int	vt_allplanes;
extern int 	vga_is_color;			/* true if on a color display */

/*
 * These variables are used to control the position of the visible portion
 * of the total video memory.  Normally, this just stays in the upper left
 * corner of the screen, but if we're set up for panning, it moves.
 */
int	vt_screen_w;            /* width of visible screen */
int	vt_screen_h;            /* height of visible screen */
int	vt_screen_x;            /* x position of UL corner of visible screen */
int	vt_screen_y;            /* y position of UL corner of visible screen */
int	vt_start_addr;          /* offset to start of visible screen */
int	vt_shift;               /* amount to shift visible screen */

struct	kd_memloc vt_map;
int	vt_fd;			/* file descriptor for the vt used */
int	max_planes;		/* maximum number of planes available */

unchar	*screen_buffer;		/* pointer to saved video buffer */
unchar	saved_misc_out;		/* need to save and restore this */
					/* because the kernel doesn't do */
					/* it right			 */

BYTE	*vga_write_map;			/* maps planes for writing */
BYTE	*vga_read_map;			/* maps planes for reading */
BYTE	*vga_color_map;			/* maps colors */
BYTE	*vga_attr_map;			/* maps attributes */

BYTE write_map_std[PLANES] = {	/* standard plane configuration */
		0x1, 0x2, 0x4, 0x8
	};

BYTE read_map_std[PLANES] = {	/* standard plane configuration */
		0x0, 0x1, 0x2, 0x3
	};

BYTE color_map_std[COLORS] = {	/* standard plane configuration */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

BYTE attr_map_std[COLORS] = {	/* standard plane configuration */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

BYTE write_map_chain[PLANES] = {	/* chained plane configuration */
		0x3, 0xc, 0, 0
	};

BYTE read_map_chain[PLANES] = {	/* chained plane configuration */
		0x0, 0x2, 0x0, 0x0
	};

BYTE color_map_chain[COLORS] = {	/* chained plane configuration */
		0, 0x3, 0xc, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

BYTE attr_map_chain[COLORS] = {	/* chained plane configuration */
		0, 1, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

/*
 * Table giving the information needed to initialize the EGA/VGA registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
extern struct reginfo regtab[];


/*
 * Initialize the virtual terminal based on the mode passed in.
 */
vt_init(fd, mode, colors)
int	fd;
int	mode;
int	colors;
{
	extern char	*malloc();

	vt_fd = fd;

	if (vt_display_init(mode) == -1)	/* initialize display */
		return(-1);

	vt_info.colors = colors;

	if (vt_info.is_chained) {
		vga_write_map = write_map_chain;
		vga_read_map = read_map_chain;
		vga_color_map = color_map_chain;
		vga_attr_map = attr_map_chain;
		if (colors == 1)
			vt_allplanes = 0x300;
		else
			vt_allplanes = 0xf00;
	}
	else {
		vga_write_map = write_map_std;
		vga_read_map = read_map_std;
		vga_color_map = color_map_std;
		vga_attr_map = attr_map_std;
		vt_allplanes = (vt_info.colors - 1) << 8;
	}

	max_planes = vt_info.planes;
	switch(colors) {
		case 2:   vt_info.planes = 1; break;
		case 4:   vt_info.planes = 2; break;
		case 8:   vt_info.planes = 3; break;
		case 16:  vt_info.planes = 4; break;
		case 32:  vt_info.planes = 5; break;
		case 64:  vt_info.planes = 6; break;
		case 128: vt_info.planes = 7; break;
		case 256: vt_info.planes = 8; break;
	}

	vt_screen_w = vt_info.xpix;
	vt_screen_h = vt_info.ypix;
	vt_screen_x = 0;
	vt_screen_y = 0;
	vt_start_addr = 0;
	vt_shift = 0;

	saved_misc_out = inb(MISC_OUT_READ);
	vt_set_regs();
	if ( !(*vt_info.ext_init)(vt_info.vt_type) )
	{
		/*
		 * failed to initialize; may be failed to init extended mode
		 */
		exit ();
	}
	
	vt_set_start_addr();

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, vt_info.regs->miscreg);	/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	inb(vt_info.ad_addr + IN_STAT_1); /* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	memset(vt_info.vt_buf, 0, vt_info.map_size); 

	inb(vt_info.ad_addr + IN_STAT_1);/* turn on display */
	outb(regtab[I_ATTR].ri_address, PALETTE_ENABLE);
}



/*
 *	vt_close()	-- unmap the current vt
 */
vt_close()
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, saved_misc_out);		/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	(*vt_info.ext_rest)(vt_info.vt_type);
	if (ioctl(vt_fd, KDUNMAPDISP, 0) == -1)
		ErrorF("Unable to unmap display\n");

	if (ioctl(vt_fd, KDSETMODE, KD_TEXT0) == -1)
		ErrorF("Unable to set text mode.\n");
}



/*
 * Save the current display memory
 */
vt_display_save()
{
	vt_save_mem();

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, saved_misc_out);		/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	(*vt_info.ext_rest)(vt_info.vt_type);
	if (ioctl(vt_fd, KDUNMAPDISP, 0) == -1)
		return(-1);
	return(0);
}


/*
 * Restore the display registers and saved memory
 */
vt_display_restore()
{
	if ((ioctl(vt_fd, KDMAPDISP, &vt_map) == -1) ||
	    (ioctl(vt_fd, KDENABIO, 0) == -1))
		return(-1);

	saved_misc_out = inb(MISC_OUT_READ);
	vt_set_regs();
	(*vt_info.ext_init)(vt_info.vt_type);

	/*
	 * reset start_addr and shift to force them to be set
	 */
	vt_start_addr = 0;
	vt_shift = 0;
	vt_set_start_addr();

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, vt_info.regs->miscreg);	/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	vt_restore_mem();
	return(0);
}



/*
 * Save the current display memory
 */
static
vt_save_mem()
{
	int	i;
	unchar	*from, *to;
/*
 *      AllocSaveScreen typecasted to return a pointer to a (unsigned char)
 */
	if ((to = screen_buffer = (unsigned char *)AllocSaveScreen (vt_info.buf_size)) == NULL) {
		ErrorF("Cannot allocate memory for screen save\n");
		return;
	}

	from = vt_info.vt_buf;
	inb(vt_info.ad_addr + IN_STAT_1); 	/* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	for (i = 0; i < max_planes; i++) {	/* loop through planes */
		out_reg(&regtab[I_GRAPH], READ_MASK, vga_read_map[i]);	
		memcpy(to, from, vt_info.map_size);
		to += vt_info.map_size;
	}
	inb(vt_info.ad_addr + IN_STAT_1);
}



/*
 * Restore the saved display memory
 */
static
vt_restore_mem()
{
	int	i;
	unchar	*from;

	inb(vt_info.ad_addr + IN_STAT_1);	/* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	from = screen_buffer;
	for (i = 0; i < max_planes; i++) {	/* loop through planes */
		out_reg(&regtab[I_SEQ], MAP_MASK, vga_write_map[i]);
		memcpy(vt_info.vt_buf, from, vt_info.map_size);
		from += vt_info.map_size;
	}

	inb(vt_info.ad_addr + IN_STAT_1);	/* turn on display */
	outb(regtab[I_ATTR].ri_address, PALETTE_ENABLE);

	FreeSaveScreen (screen_buffer); 	/* free the allocated mem */
}



/*
 * vt_set_regs()	-- Set the registers according to the mode given.
 */
static
vt_set_regs()
{
	int i;

	(void)inb(vt_info.ad_addr + IN_STAT_1); 	/* init flip-flop */
	outb(regtab[I_ATTR].ri_address, 0);		/* turn off display */

	doregs(vt_info.regs->seqtab, I_SEQ);		/* Reset, init and */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */

	/*
	 * Initialize CRT controller.
	 * On VGA, we clear register 0x11 which enables writing to
	 * registers 0 - 7.
	 */
	if (vga_is_color) {
		if (vt_info.is_vga)
			out_reg(&regtab[I_EGACOLOR], 0x11, 0xe);
		doregs((char *)&vt_info.regs->egatab, I_EGACOLOR);
	}
	else {
		if (vt_info.is_vga)
			out_reg(&regtab[I_EGAMONO], 0x11, 0xe);
		doregs((char *)&vt_info.regs->egatab, I_EGAMONO);
	}

	/*
	 * Initialize graphics registers.
	 */
	outb(GRAPH_1_POS, GRAPHICS1);
	outb(GRAPH_2_POS, GRAPHICS2);
	doregs(graphics, I_GRAPH);
        outw(VGA_GRAPH, vt_info.wmode);                 /* mode reg is */
                                                        /* special */

	if ((vt_info.is_vga) && (vt_info.vt_type != VT_TSL1024) &&
	    (vt_info.vt_type != VT_TSL1024ni) &&
	    (vt_info.vt_type != VT_ORVGAf1)) {
		attributes[0x11] = 0x10;
		vga_color(0x10, 0, 0, 0);		/* set overscan color */
	}

	(void)inb(vt_info.ad_addr + IN_STAT_1); 	/* init flip-flop */
	doregs(attributes, I_ATTR);			/* init attr regs */
}



/*
 * doregs(tabptr, type)	-- set EGA/VGA registers based on the type provided 
 *			and the fact that all registers of a given type are 
 *			accessed through a common I/O address with a separate 
 *			address register being used to distinguish them.
 * Input:
 *	char 	*tabptr		-- address of table to write/read data to/from
 *	int	type		-- type of registers to write/read
 */
static
doregs(tabptr, type)				
register char	*tabptr;	
int		type;
{
	register int	index, count;
	struct reginfo	*regptr;

	regptr = &regtab[type];
	count = regptr->ri_count;

	if ((type == I_ATTR) && !vt_info.is_vga) 
		count--;		/* EGA has one less attr reg than VGA */

	for (index = 0; index < count; index++, tabptr++) {
		out_reg(regptr, index, *tabptr);
	}
}



/*
 *	ega_color(i, c)	-- set the color of attribute index i, to c.
 */
ega_color(i, c)
int i;
int c;
{
	/* wait for vertical retrace */
	while (!(inb(vt_info.ad_addr+STATUS_REG) & S_VSYNC))
		;

	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	outb(0x3c0, i);
	outb(0x3c0, c);				/* change color */
	outb(0x3c0, PALETTE_ENABLE);		/* enable pallette */
}


/*
 *	vga_color(i, r, g, b) -- set the color of pallette index i, to r, g, b.
 */
vga_color(i, r, g, b)
int i;
int r, g, b;
{
	/* wait for vertical retrace */
	while (!(inb(vt_info.ad_addr+STATUS_REG) & S_VSYNC))
		;

	outb(PEL_WRITE, i);
	outb(PEL_DATA, r);			/* Red */
	outb(PEL_DATA, g);			/* Green */
	outb(PEL_DATA, b);			/* Blue */
}
		


/*
 *	vt_set_start_addr()	-- modify (if needed) the starting address
 *				(offset) of the visible portion of video
 *				memory and the amount by which it gets
 *				shifted.  This is used for panning.
 */
vt_set_start_addr()
{
	int	new_start_addr;
	int	new_shift;

	new_start_addr	= (vt_screen_y * vt_info.slbytes) + (vt_screen_x >> 3);
	new_shift	= vt_screen_x & 0x7;

	if ((new_start_addr != vt_start_addr) || (new_shift != vt_shift)) {
		vt_start_addr = new_start_addr;
		vt_shift = new_shift;

		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x0c, vt_start_addr >> 8);
			out_reg(&regtab[I_EGACOLOR], 0x0d, vt_start_addr& 0xff);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x0c, vt_start_addr >> 8);
			out_reg(&regtab[I_EGAMONO], 0x0d, vt_start_addr & 0xff);
		}

		/* wait for vertical retrace */
		while (!(inb(vt_info.ad_addr+STATUS_REG) & S_VSYNC))
			;

                (void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
                out_reg(&regtab[I_ATTR], (0x13 | PALETTE_ENABLE), vt_shift);
	}
}
	


/*
 * vt_display_init()	-- Based on display type being used, set things up 
 *			as necessary so we'll be able to address video memory 
 *			and I/O ports.  Set the global variable vt_info.
 */
vt_display_init(type)
int type;
{
	struct kd_disparam dp;
	int disp_type;
	extern char *malloc();

	vt_info = disp_info[type];

	if (ioctl(vt_fd, KDDISPTYPE, &dp) == -1) {
		ErrorF("Unable to determine display type.\n");
		return(-1);
	}

	/*
	 * The display type has ('k' << 8) for the console. Other
	 * displays like the Sun River return other values (like
	 * ('s' << 8).
	 */
	if ((disp_type = ioctl(vt_fd, KIOCINFO, 0)) == -1) {
		ErrorF("Could not get display type.\n");
		return(-1);
	}
	disp_type >>= 8;

	/*
	 * set the display to graphics vt process mode 
	 */
	if (ioctl(vt_fd, KDSETMODE, KD_GRAPHICS) == -1) {
		ErrorF("Unable to set graphics mode.\n");
		return(-1);
	}
	
	/* 
	 * allocate an area of our virtual memory to map the physical
	 * screen address space to.  We have to have the address be on
	 * a page (4096) boundry, so we allocate extra and round up.
	 */
	if ((vt_info.vt_buf = (unchar *)malloc(vt_info.map_size+PG_SIZE)) == NULL) {
		ErrorF("Can't Allocate Memory for Screen Save\n");
		return(-1);
	}
	vt_info.vt_buf = (unchar *)(((long)vt_info.vt_buf & ~(PG_SIZE-1)) + 
			  PG_SIZE);

	/* 
	 * ask the display driver to map the screen memory and I/O ports 
	 */
	vt_map.vaddr = (char *)vt_info.vt_buf;
	vt_map.physaddr = (disp_type == 'k') ? (char *)base_addr[dp.type]
					     : (char *)dp.addr;
	vt_map.length = (long)vt_info.map_size;
	vt_map.ioflg = (long)1;

	if (ioctl(vt_fd, KDMAPDISP, &vt_map) == -1) {
		ErrorF("Unable to map display.\n");
		return(-1);
	}

	if (vt_info.is_vga && ((inb(MISC_OUT_READ) & IO_ADDR_SEL) == 0)) {
		vga_is_color = 0;
		vt_info.ad_addr = 0x3b4;
		vt_info.regs->miscreg &= ~IO_ADDR_SEL;
	}
	else
		vga_is_color = 1;

	if (vt_info.map_size == (128*1024))
		graphics[GR_MISC] = 1;

	if ((vt_info.vt_type == VT_V7FW1_2) ||
	    (vt_info.vt_type == VT_V7VRAM1_2)) {
		attributes[0x12] = 0x1;
		graphics[GR_MISC] = 0x3;
	}
	if ((vt_info.vt_type == VT_V7FW1_4) ||
	    (vt_info.vt_type == VT_V7VRAM1_4)) {
		attributes[0x12] = 0x5;
		graphics[GR_MISC] = 0x3;
	}
	
	return(0);
}

/* Allocate screen saver memory "on-the-fly" */

char *
AllocSaveScreen (size)
long size;
{
	char *sbrk();
	char *save_screen;

	/* Save old end of data space */
	/* and add in screen size */
	save_screen = sbrk(size);
	if (!save_screen)
		return(NULL);
	else
		return (save_screen);
}

/* Free up memory allocated for screen saver. */
/* This memory is being returned to the os */

void
FreeSaveScreen (save_screen)
char *save_screen;
{
	int brk();

	if (save_screen != (char *)0) {
		if (brk(save_screen) == -1) {
			ErrorF("Can not free memory alloc'ed for screen save area");
		}
	}
}
