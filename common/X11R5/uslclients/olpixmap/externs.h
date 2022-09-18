/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:externs.h	1.14"
#endif

#ifndef _EXTERNS_H
#define _EXTERNS_H


typedef enum { Busy, Normal } StatusType;

typedef struct {
	Boolean icon_menu;
	Boolean warnings;
} AppResStruct;


extern AppResStruct	AppResources;

extern Widget		Toplevel;
extern Widget		ScrolledWindow;
extern Widget		Canvas;
extern Widget		PixmapDisplay;

extern Bool		PixmapIsDisplayed;
extern Pixmap		RealPixmap;

extern Dimension	PixmapWidth;
extern Dimension	PixmapHeight;
extern Cardinal		PixmapDepth;
extern Colormap		PixmapColormap;

extern Dimension	CanvasPixelWidth;
extern Dimension	CanvasPixelHeight;

extern Pixel		CurrentForeground;
extern Pixel		CurrentBackground;
extern int		CurrentFunction;
extern Bool		DrawFilled;
extern int		CurrentLineWidth;
extern int		CurrentLineStyle;
extern Bool		ShowGrid;

extern char *		ApplicationName;
extern GC		DrawGC;
extern Bool		Changed;


extern void		RefreshCanvas();
extern void		RefreshPixmapDisplay();
extern void		RefreshAllVisuals();
extern void		ResetAllVisuals();
extern void		ResetDrawGC();
extern void		BringDownPopup();
extern void		MakeGrid();
extern void		DrawGrid();
extern void		FillPixel();
extern void		ShrinkPixel();
extern void		ExpandPixel();
extern void		ShowPixmap();
extern void		FooterMessage();
extern Bool		OpenFile();
extern int		power();
extern unsigned int	ConvertToPosInt();
extern void		SetStatus();
extern void		ResetCursorColors();


#endif /* _EXTERNS_H */
