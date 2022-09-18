/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256.c	1.2"

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

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
/*
#define  DEBUG
static int  xdebug = 0x10;
 */
#include "v256.h"
#include "sys/inline.h"

#include "v256spreq.h"

#ifdef	DELETE
#include <stdio.h>	/* TEMP for fprintf */
#endif	/* DELETE */

extern	SIBool	v256_plot_points();
extern	SIBool	v256_download_state();
extern	SIBool	v256_get_state();
extern	SIBool	v256_select_state();
extern	SIvoid	v256_clip();
extern	SILine	v256_getsl();
extern	void	v256_setsl();
extern	void	v256_freesl();
extern	SIBool	v256_line_onebit();
extern	SIBool	v256_lineseg_onebit();
extern	SIBool	v256_rect_onebit();
extern	SIBool	v256_fill_rect();
extern	SIBool	v256_ss_bitblt();
extern	SIBool	v256_ms_bitblt();
extern	SIBool	v256_sm_bitblt();
extern	SIBool	v256_hcurs_download();
extern	SIBool	v256_hcurs_turnon();
extern	SIBool	v256_hcurs_turnoff();
extern	SIBool	v256_hcurs_move();
extern	SIBool	v256_ms_stplblt();
extern	SIBool	v256_check_dlfont();
extern	SIBool	v256_dl_font();
extern	SIBool	v256_stpl_font();
extern	SIBool	v256_font_free();
extern	SIBool	v256_fill_spans();
extern	SIBool	v256_set_cmap();

SIVisual v256_visual = 
{
	PSEUDOCOLOR_AVAIL,	/* type */
	8,					/* visual depth */
	1,					/* color map count */
	256,				/* color map size */
	6,					/* bits per rgb */
	0, 0, 0,			/* red, green, blue masks */
	0, 0, 0				/* red, green, blue offsets */
};



/*
 *	v256_init(file, cfg, info, routines) -- Initialize machine dependent 
 *				things.  Set up graphics mode, allocate buffer,
 *				map it to the display memory, clear display.
 *
 *	Input:
 *		int	file			-- file descriptor for display
 *		SIConfigP cfg 			-- config structure
 *		SIInfoP	info			-- info	structure to be loaded
 *		ScreenInterface	**routines	-- routines pointer to return
 */
SIBool
v256_init(
	int	file,
	SIConfigP cfg,
	SIInfoP	info,
	ScreenInterface **routines
	)
{
	int type;
	extern ScreenInterface v256DisplayInterface;

	DBENTRY("v256_init()");

	if ( (type = 
		v256_config(cfg, &(info->SIxppin), &(info->SIyppin))) == -1)
	{
		return(SI_FAIL);
	}

#ifdef	DELETE
fprintf (stderr, "The 256 color VGA library you are using is UNSUPPORTED and UNTESTED in release 4i.\n");
#endif
	
	vt_init(file, type);

	/*
	 *	set  the page tables 
	 */
	if ( v256_setup_splitter(vt_info.xpix, vt_info.ypix, VGA_PAGE_SIZE ) == 0)
	{
		ErrorF("Cannot set up page tables for %dx%d resoultion \n", 
			vt_info.xpix, vt_info.ypix);
		return	SI_FAIL;
	}

	/*
	 * set up pointer to subroutine structure
	 */
	*routines = &v256DisplayInterface;

	if (!v256_is_color)
	{
		v256_visual.SVtype = GRAYSCALE_AVAIL;
	}

	/* 
	 * setup info structure
	 */
	info->SIvisualCNT = 1;
	info->SIvisuals = &v256_visual;
	info->SIlinelen = vt_info.xpix;	/* length of a scan line */
	info->SIlinecnt = vt_info.ypix;	/* number of scan lines */
	info->SIstatecnt = V256_NUMGS;	/* number of graphics states */

					/* Has bitblt routines */
	info->SIavail_bitblt =	SSBITBLT_AVAIL |
							SMBITBLT_AVAIL |
							MSBITBLT_AVAIL;
					/* Has stplblt routines */
	info->SIavail_stplblt =	MSSTPLBLT_AVAIL |
							OPQSTIPPLE_AVAIL |
							STIPPLE_AVAIL;

					/* Has polyfill routines */
	info->SIavail_fpoly =	RECTANGLE_AVAIL |
							TILE_AVAIL |
							STIPPLE_AVAIL |
							OPQSTIPPLE_AVAIL;

					/* Has pointplot routine */
	info->SIavail_point =	PLOTPOINT_AVAIL;

					/* Has line draw routines */
	info->SIavail_line =	ONEBITLINE_AVAIL |
							ONEBITSEG_AVAIL;
				
						/* Has a fake hardware cursor */
	info->SIcursortype = CURSOR_FAKEHDWR;
	info->SIcurscnt = 1;			/* # of downloadable cursors */
	info->SIcurswidth = 16;			/* Best Cursor Width */
	info->SIcursheight = 16;		/* Best Cursor Height */
	info->SIcursmask = 0x7;			/* Mask to byte boundry */

	info->SIavail_drawarc =	0;	/* doesn't have arc draw routines */
	info->SIavail_fillarc =	0;	/* doesn't have arc fill routines */

	info->SItilewidth = V256_PAT_W;		/* Best width for tile */
	info->SItileheight = V256_PAT_H;	/* Best height for tile */

	info->SIstipplewidth = V256_PAT_W/2;	/* Best width for stipple */
	info->SIstippleheight = V256_PAT_H;	/* Best height for stipple */

	info->SIavail_font =	FONT_AVAIL |	/* has downloadable fonts */
							STIPPLE_AVAIL |
							OPQSTIPPLE_AVAIL;

	info->SIfontcnt = V256_NUMDLFONTS;

	info->SIavail_spans = SPANS_AVAIL |	/* has span filling */
						  TILE_AVAIL |
						  STIPPLE_AVAIL |
						  OPQSTIPPLE_AVAIL;

	info->SIavail_exten = 0;		/* no extensions available */

	/* Set up the current (and only) state structure */
	/* The most important pieces of this are the colormap entries */

	v256_gs->pmask = V256_MAXCOLOR - 1;
	v256_gs->mode = GXcopy;
	v256_gs->stp_mode = SGFillSolidFG;
	v256_gs->fill_mode = SGFillSolidFG;
	v256_gs->fg = 0;
	v256_gs->bg = 1;

	v256_slbytes = vt_info.slbytes;
	v256_clip_x1 = 0;
	v256_clip_y1 = 0;
	v256_clip_x2 = vt_info.xpix - 1;
	v256_clip_y2 = vt_info.ypix - 1;

	return(SI_SUCCEED);

}


/*
 *	v256_restore()		-- Cleanup any machine dependent things
 */
SIBool
v256_restore()
{
	DBENTRY("v256_restore()");

	vt_close();

	return(SI_SUCCEED);

}



/*
 *	v256_savescreen()	-- Save display memory in preparation for
 *				a vt switch.
 */
SIBool
v256_savescreen()
{
	DBENTRY("v256_savescreen()");
	
	if (vt_display_save() == -1)
	{
		return(SI_FAIL);
	}

	return(SI_SUCCEED);
}


/*
 *	v256_restorescreen()	-- Restore screen to graphics state and 
 *				restore saved display memory.
 */
SIBool
v256_restorescreen()
{
	DBENTRY("v256_restorescreen()");

	if (vt_display_restore() == -1)
	{
		return(SI_FAIL);
	}

	while (!(inb(vt_info.ad_addr+STATUS_REG) & S_VSYNC))
	{
		;
	}

	v256_set_attrs();

	return(SI_SUCCEED);
}


/*
 *	v256_vb_onoff(on)	-- turn the screen on or off
 *
 *	Input:
 *		int	on	-- true for screen on, false for screen off
 */
SIBool
v256_vb_onoff(on)
int on;
{
	int	temp;

	if (on)
	{			/* turn on display */
		outb(0x3C4, 0x01);
		temp = 0xDF & inb(0x3C5);
		outb(0x3C5, temp);
		(void)inb(0x3DA);
		outb(0x3C0, PALETTE_ENABLE);
	}
	else
	{			/* turn off display */
		outb(0x3C4, 0x01);
		temp = 0x20 | inb(0x3C5);
		outb(0x3C5, temp);
	}	

	return(SI_SUCCEED);
}



/*
 *	v256_screen(screen, flag)	-- Enter/Leave a screen
 *
 *	Input:
 *		int	screen	-- which screen is being entered or left
 *		int	flag	-- whether being entered or left
 */
SIBool
v256_screen(screen, flag)
int screen;
int flag;
{
	return(SI_SUCCEED);
}



static SIBool
NoEntry()
{
	return(SI_FALSE);
}

/*
 * This structure is visible to the smart server, and is used to determine 
 * which subroutines do/don't exist.
 */
ScreenInterface v256DisplayInterface = 
{
		/* MISCELLANEOUS ROUTINES	*/  
		/*	MANDATORY		*/

	/* void	(*si_init)();			machine dependant init */
	v256_init,
	/* void	(*si_restore)();		machine dependant cleanup */
	v256_restore,
	/* void	(*si_savescreen)();		machine dependant vt flip */
	v256_savescreen,
	/* void	(*si_restorescreen)();		machine dependant vt flip */
	v256_restorescreen,
	/* SIBool (*si_vb_onoff)();		turn on/off video blank */
	v256_vb_onoff,
	/* SIBool	(*si_initcache)();	start caching requests */
	NoEntry,
	/* SIBool	(*si_flushcache)();	write caching requests */
	NoEntry,
	/* SIBool	(*si_download_state)();	Set current state info */
	v256_download_state,
	/* SIBool	(*si_get_state)();	Get current state info */
	v256_get_state,
	/* SIBool	(*si_select__state)();	Select current GS entry */
	v256_select_state,
	/* SIBool       (*si_screen)();         Enter/Leave a screen */
	v256_screen,


		/* SCANLINE AT A TIME ROUTINES	*/  /* done! */
		/*	MANDITORY		*/

	/* SILine	(*si_getsl)();		get pixels in a scanline */
	v256_getsl,
	/* void	(*si_setsl)();			set pixels in a scanline */
	v256_setsl,
	/* void	(*si_freesl)();			free scanline buffer */
	v256_freesl,

		/* COLORMAP MANAGEMENT ROUTINES */
		/* 	MANDATORY		*/

	/* SIBool  (*si_set_colormap)();	Set Colormap entries */
	v256_set_cmap,
	/* SIBool  (*si_get_colormap)();	Get Colormap entries */
	NoEntry,

		/* HARDWARE CURSOR CONTROL	*/
		/*	MANDATORY		*/
	/* SIBool	(*si_hcurs_download)();	Download a cursor */
	v256_hcurs_download,
	/* SIBool	(*si_hcurs_turnon)();	Turnon the cursor */
	v256_hcurs_turnon,
	/* SIBool	(*si_hcurs_turnoff)();	Turnoff the cursor */
	v256_hcurs_turnoff,
	/* SIBool	(*si_hcurs_move)();	Move the cursor position */
	v256_hcurs_move,


		/* HARDWARE SPANS FILLING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_fillspans)();	fill spans */
	v256_fill_spans,

		/* HARDWARE BITBLT ROUTINES	*/  /* done! */
		/*	OPTIONAL		*/

	/* SIBool	(*si_ss_bitblt)();	perform scr->scr bitblt */
	v256_ss_bitblt,
	/* SIBool	(*si_ms_bitblt)();	perform mem->scr bitblt */
	v256_ms_bitblt,
	/* SIBool	(*si_sm_bitblt)();	perform scr->mem bitblt */
	v256_sm_bitblt,

		/* HARDWARE STPLBLT ROUTINES	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_ss_stplblt)();	perform scr->scr stplblt */
	NoEntry,
	/* SIBool	(*si_ms_stplblt)();	perform mem->scr stplblt */
	v256_ms_stplblt,
	/* SIBool	(*si_sm_stplblt)();	perform scr->mem stplblt */
	NoEntry,

		/* HARDWARE POLYGON FILL	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_poly_clip)();	for polygon clip */
	v256_clip,
	/* SIBool	(*si_poly_fconvex)();	for convex polygons */
	NoEntry,
	/* SIBool	(*si_poly_fgeneral)();	for general polygons */
	NoEntry,
	/* SIBool       (*si_poly_fillrect)();  for rectangular regions */
	v256_fill_rect,

		/* HARDWARE POINT PLOTTING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_plot_points)();	*/
	v256_plot_points,

		/* HARDWARE LINE DRAWING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_line_clip)();	set line draw clip */
	v256_clip,
	/* SIBool	(*si_line_onebitline)();One bit line (connected) */
	v256_line_onebit,
	/* SIBool       (*si_line_onebitseg)(); One bit line segments */
	v256_lineseg_onebit,
	/* SIBool       (*si_line_onebitrect)();One bit line rectangles */
	NoEntry,


		/* HARDWARE ARC DRAWING		*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_drawarc_clip)();	set arc draw clip */
	(SIvoid (*)()) NoEntry,
	/* SIBool	(*si_drawarc_fill)();	draw arc */
	NoEntry,
	/* SIBool	(*si_arc_clip)();	set arc draw clip */
	(SIvoid (*)()) NoEntry,
	/* SIBool	(*si_arc_fill)();	fill arc */
	NoEntry,

		/* HARDWARE FONT CONTROL        */  /* done! */
		/*      OPTIONAL                */
	/* SIBool       (*si_font_check)();     Check font downloadability */
	v256_check_dlfont,
	/* SIBool       (*si_font_download)();  Download a font command */
	v256_dl_font,
	/* SIBool       (*si_font_free)();      free a downloaded font */
	v256_font_free,
	/* SIBool       (*si_font_clip)();      set font clip */
	v256_clip,
	/* SIBool       (*si_font_stplblt)();   stipple a list of glyphs */
	v256_stpl_font,

		/* SBDD MEMORY CACHING CONTROL  */
		/*	OPTIONAL		*/
	/* SIBool	(*si_cache_alloc)();	* allocate pixmap into cache */
	NoEntry,
	/* SIBool	(*si_cache_free)();	* remove pixmap into cache */
	NoEntry,
	/* SIBool	(*si_cache_lock)();	* lock pixmap into cache */
	NoEntry,
	/* SIBool	(*si_cache_unlock)();	* unlock pixmap from cache */
	NoEntry,

		/* SBDD EXTENSION INITIALIZATION */
		/*      OPTIONAL                */
	/* SIBool       (*si_exten_init)();  * list of extension
						initizlization routines */
	NoEntry,
};

ScreenInterface *DisplayFuncs = &v256DisplayInterface;
