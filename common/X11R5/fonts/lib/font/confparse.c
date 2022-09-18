#ident	"@(#)r5fontlib:font/confparse.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <font.h>
#include <fontstruct.h>
#include <Xrenderer.h>



int 
ParseCheck4Download(font)
FontPtr font;
{
   int height = 0;

   font->info.downloadable =  0;
   if (font->rendererPtr->config.download == 3) return -1;
   if ((font->info.numChars > font->rendererPtr->config.dload_maxchars)
	 && (font->rendererPtr->config.dload_maxchars != 0)) return -1; 
   if ((font->rendererPtr->config.download  == 1) && (font->info.constantWidth != 1))
		return -1;

		/* fixed width only can be downloaded */	
   if ((font->rendererPtr->config.dload_width > 0 ) && 
   (font->info.maxbounds.characterWidth > font->rendererPtr->config.dload_width))
		 return -1;

   height = font->info.maxbounds.ascent + font->info.maxbounds.descent;
   if (height < 1) return -1;
   if ((font->rendererPtr->config.dload_height > 0) && (height > font->rendererPtr->config.dload_height))
		return -1; 

   font->info.downloadable = 1;

return 1;
}


int
ParseRendererPublic(renderer)
FontRendererPtr;
{

/* do any specific initialization of this renderers stuff
that may not be done at startup. For now all is done at startup
so just return true */

return 1;
}

