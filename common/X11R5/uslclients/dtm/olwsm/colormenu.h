/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/colormenu.h	1.8.2.2"
#endif

#ifndef _COLORMENU_H
#define _COLORMENU_H

#define BW_BG			0x0001
#define BW_FG			0x0002
#define COLOR_WORKSPACE		0x0001
#define COLOR_BG0		0x0002
#define COLOR_BG1		0x0003
#define COLOR_INPUT_WINDOW	0x0004
#define COLOR_INPUT_FOCUS	0x0005
#define COLOR_TEXT_BG		0x0006
#define COLOR_HELP_FG		0x0007
#define CONTRAST		0x1000

#define COLOR_SET_NONE	0
#define COLOR_SET_ONE	1
#define COLOR_SET_MANY	2

extern String		DefaultColor OL_ARGS((
	unsigned int		color_flags,
	unsigned int		bw_flags
));
extern String		DefaultColorTupleList OL_ARGS((
	void
));
extern String		PixelToString OL_ARGS((
	Widget			w,
	Pixel			pixel
));
extern Pixel		StringToPixel OL_ARGS((
	Widget			w,
	String			color
));
extern Widget		CreateColorMenu OL_ARGS((
	Widget			parent,
	void			(*color_cb)( Pixel ),
	void			(*bw_cb)( Pixel , Pixel )
));
extern void		SetColorMenu OL_ARGS((
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
));

#endif
