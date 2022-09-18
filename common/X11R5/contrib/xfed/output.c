/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xfed:output.c	1.1"
/*
 * Copyright 1988 by Siemens
 *		All Rights Reserved
 * 
 * written 16-17 Mar 1988 by claus gittinger
 *
 * produce a new .bdf file 
 *
 */

#include "defs.h"
#include <stdio.h>
#include <errno.h>

extern int errno;

output(font, filename)
struct font *font;
char *filename;
{
	FILE *out;
	int nchars;
	int i;
	struct character *charp;

	out = fopen(filename, "w");
	if (out == NULL) {
	    perror("xfed");
	    fprintf(stderr, "cannot open %s for writing\n", filename);
	    return;
	}
	fprintf(out, "STARTFONT %d.%d\n", font->rev, font->subrev);
	/* !! Makes all comments one contiguous block */
	for(i = 0; i < font->ncomments; i++) {
	    fprintf(out, "COMMENT %s\n", font->comments[i]);
	}
	fprintf(out, "FONT %s\n", font->fontname);
	fprintf(out, "SIZE %d %d %d\n",
				font->sizes[0], font->sizes[1],
				font->sizes[2]);
	fprintf(out, "FONTBOUNDINGBOX %d %d %d %d\n",
				font->boundingbox.w, font->boundingbox.h,
				font->boundingbox.x, font->boundingbox.y);
	fprintf(out, "STARTPROPERTIES %d\n", font->nprops);
	for(i = 0; i < font->nprops; i++) {
	    fprintf(out, "%s\n", font->props[i]);
	}
	fprintf(out, "ENDPROPERTIES\n");
	fprintf(out, "CHARS %d\n", font->nchars);

	nchars = font->nchars;
	charp = font->characters;
	while (nchars--)
	    outputchar(out, charp++);

	fprintf(out, "ENDFONT\n");
	fclose(out);
}

outputchar(out, charp)
FILE *out;
struct character *charp;
{
	int i;

	fprintf(out, "STARTCHAR %s\n", charp->charId);
	if (charp->notadobe == 0)
	    fprintf(out, "ENCODING %d\n", charp->encoding);
	else
	    fprintf(out, "ENCODING -1 %d\n", charp->encoding);
	fprintf(out, "SWIDTH %d %d\n", charp->swidth[0], charp->swidth[1]);
	fprintf(out, "DWIDTH %d %d\n", charp->dwidth[0], charp->dwidth[1]);
	fprintf(out, "BBX %d %d %d %d\n",
			    charp->bbx.w, charp->bbx.h,
			    charp->bbx.x, charp->bbx.y);
	fprintf(out, "BITMAP\n");
	for (i=0; i<charp->nrows; i++) {
#ifdef CLEARBITS
	    char *rowbits;
	    int col, bits, j;
	    rowbits = charp->rows[i];
	    for (col = 0; *rowbits; rowbits++) {
	        if ((bits = *rowbits - '0') > 9 || bits < 0) {
		    if (*rowbits >= 'A' && *rowbits <= 'F')
		        bits = *rowbits - 'A' + 10;
		    else if (*rowbits >= 'a' && *rowbits <= 'f')
		        bits = *rowbits - 'a' + 10;
	        }
	        if (col++ >= charp->bbx.w)
		    bits &= ~8;
	        if (col++ >= charp->bbx.w)
		    bits &= ~4;
	        if (col++ >= charp->bbx.w)
		    bits &= ~2;
	        if (col++ >= charp->bbx.w)
		    bits &= ~1;
		*rowbits = bits + ((bits < 10) ? '0' : 'a' - 10);
	    }
#endif /* CLEARBITS */
	    fprintf(out, "%s\n", charp->rows[i]);
	}
	fprintf(out, "ENDCHAR\n");
}
