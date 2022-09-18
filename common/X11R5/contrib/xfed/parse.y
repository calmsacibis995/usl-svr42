/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ident	"@(#)r4xfed:parse.y	1.1"
/*
 * Copyright 1988 by Siemens
 *		All Rights Reserved
 *
 * 
 * written 16-17 Mar 1988 by claus gittinger
 *
 * q & d hacked grammar for .bdf files.
 *
 */
/*
 * Modified by Mark Moraes @ the University of Toronto to make it handle
 * a larger subset of BDF. It now reads in all the font files in X.V11R3
 * without complaints. Comments all get gathered in at the beginning
 * thanks to the kludgey way in which they're now stored -- fixes
 * welcome.
 */
#include <stdio.h>
#include "defs.h"

#define MAXROWS		512
#define INC_COMMENTS	32	/* size in which comments array grows */

extern int lineno;		/* counts lines in .bdf input file */
extern int stringMode;		/* kludge in scan.l */
extern int minx, miny, maxx, maxy;
static struct character *charp;	/* current character */
static char *bits[MAXROWS];	/* that good for long characters ... */
static int nrows;
static int npropCounted;

%}

%union {
	int ival;
	char *sval;
}

%token NEWLINE DOT
%token STARTFONT SIZE FONTBOUNDINGBOX STARTPROPERTIES
%token ENDPROPERTIES
%token CHARS ENCODING SWIDTH DWIDTH BBX BITMAP
%token ENDCHAR ENDFONT
%token <sval> FONT PROPERTY STARTCHAR COMMENT STRING IDENTIFIER
%token <ival> NUMBER

%%

file:           definitions characters ENDFONT NEWLINE
		    {
			struct character *charp;
			int i;

			/*
			 * find max box for window size
			 */
			font.maxbbx.w = font.maxbbx.h = 0;
			for (i=0, charp=font.characters; i<font.nchars; i++, charp++) {
			    if (charp->bbx.w > font.maxbbx.w)
				font.maxbbx.w = charp->bbx.w;
			    if (charp->bbx.h > font.maxbbx.h)
				font.maxbbx.h = charp->bbx.h;
			}
		    }
		;

definitions:    startfont fontdefs 
		;

startfont:      STARTFONT NUMBER DOT NUMBER NEWLINE
		    {
			font.rev = $2;
			font.subrev = $4;
			font.szcomments = INC_COMMENTS;
			font.ncomments = 0;
			font.comments = (char **) malloc(font.szcomments
							 * sizeof(char *));
			if (font.rev != 2 || font.subrev != 1)
			    fprintf(stderr, "WARNING: font is not rev 2.1\n");
		    }
		;

fontdefs:	fontdef
		| fontdefs fontdef
		;

fontdef:	comment
		| font
		| size
		| boundingbox
		| properties
		;

comment:        COMMENT NEWLINE
		    {
			if (font.ncomments == font.szcomments) {
			    font.szcomments += INC_COMMENTS;
			    font.comments = (char **) realloc(
				(char *) font.comments,
				font.szcomments * sizeof(char *));
			}
			font.comments[font.ncomments++] = $1;
		    }
		;

font:           FONT NEWLINE
		    {
			font.fontname = $1;
		    }
		;

size:           SIZE NUMBER NUMBER NUMBER NEWLINE
		    {
			font.sizes[0] = $2;
			font.sizes[1] = $3;
			font.sizes[2] = $4;
		    }
		;

boundingbox:    FONTBOUNDINGBOX NUMBER NUMBER NUMBER NUMBER NEWLINE
		    {
			font.boundingbox.w = $2;
			font.boundingbox.h = $3;
			font.boundingbox.x = $4;
			font.boundingbox.y = $5;
		    }
		;

properties:     startprop props endprop
		;

startprop:      STARTPROPERTIES NUMBER NEWLINE
		    {
			font.nprops = $2;
			font.props = (char **)
			    malloc(font.nprops * sizeof(char *));
			npropCounted = 0;
		    }
		;

props:		prop
		| props prop
		;

prop:		comment
		| junk
		;

junk:		PROPERTY NEWLINE
		    {
			if (npropCounted < font.nprops) {
			    font.props[npropCounted] = $1;
			}			    
			npropCounted++;
		    }
		;

endprop:        ENDPROPERTIES NEWLINE
		    {
			if (font.nprops != npropCounted)
			    fprintf(stderr,
"Warning: STARTPROPERTIES said %d, counted %d properties, skipping excess\n",
				    font.nprops, npropCounted);
		    }
		;

characters:     chars chardefs
		;

chars:          CHARS NUMBER NEWLINE
		    {
			font.nchars = $2;
			font.characters = (struct character *)
				malloc(sizeof(struct character) * $2);
			charp = font.characters;
		    }
		;

chardefs:	/* empty */
		| chardefs character
		;

character:      STARTCHAR NEWLINE charprops bitmap ENDCHAR NEWLINE
		    {
			charp->charId = $1;
			charp++;
		    }
		;

charprops:	charprop
		| charprops charprop
		;

charprop:	encoding
		| swidth
		| dwidth
		| bbx
		;

encoding:       ENCODING NUMBER NEWLINE
		    {
			charp->encoding = $2;
			charp->notadobe = 0;
		    }
		| ENCODING NUMBER NUMBER NEWLINE
		    {
			if ($2 != -1) {
			    fprintf(stderr, "Bad ENCODING syntax in line %d",
			     lineno);
			}
			charp->notadobe = 1;
			charp->encoding = $3;
		    }
		;

swidth:         SWIDTH NUMBER NEWLINE
		    {
			charp->swidth[0] = $2;
			charp->swidth[1] = 0;
		    }
	        | SWIDTH NUMBER NUMBER NEWLINE
		    {
			charp->swidth[0] = $2;
			charp->swidth[1] = $3;
		    }
		;

dwidth:         DWIDTH NUMBER NEWLINE
		    {
			charp->dwidth[0] = $2;
			charp->dwidth[1] = 0;
		    }
		| DWIDTH NUMBER NUMBER NEWLINE
		    {
			charp->dwidth[0] = $2;
			charp->dwidth[1] = $3;
		    }
		;

bbx:            BBX NUMBER NUMBER NUMBER NUMBER NEWLINE
		    {
		    	int w, h;
			
			charp->bbx.w = $2;
			charp->bbx.h = $3;
			charp->bbx.x = $4;
			charp->bbx.y = $5;
			w = (charp->bbx.x < 0) ? charp->bbx.w : 
			 charp->bbx.x + charp->bbx.w;
			h = (charp->bbx.y < 0) ? charp->bbx.h :
			 charp->bbx.y + charp->bbx.h;
			if (charp->bbx.x < minx)
				minx = charp->bbx.x;
			if (w > maxx)
				maxx = w;
			if (charp->bbx.y < miny)
				miny = charp->bbx.y;
			if (h > maxy)
				maxy = h;
		    }
		;

bitmap:         bitmapstart NEWLINE numbers stringmodeoff
		    {
			int i;

			charp->nrows = nrows;
			charp->rows = (char **)malloc(sizeof(char *) * nrows);
			for (i=0; i<nrows; i++) 
			    charp->rows[i] = bits[i];
		    }
		;

bitmapstart:    BITMAP stringmodeon
		    {
			nrows = 0;
		    }
		;

numbers:        ; /* empty */
		| numbers STRING NEWLINE
		    {
			if (nrows >= MAXROWS) {
			    fprintf(stderr, "Too many rows. Max %d\n",
				    MAXROWS);
			    exit(1);
			}
			bits[nrows++] = $2;
		    }
		;

stringmodeon:   /* empty */
		    {
			/* this is a kludge - sorry */
			stringMode = 1;
		    }
		;

stringmodeoff:  /* empty */
		    {
			stringMode = 0;
		    }
		;

%%

yyerror(s) 
char *s;
{
	fprintf(stderr, "parse error in line %d: %s\n", lineno, s);
	exit(1);
}
