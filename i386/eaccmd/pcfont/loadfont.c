/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:i386/eaccmd/pcfont/loadfont.c	1.1"
#ident	"$Header: "
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include "pcfont.h"

extern rom_font_t font_map;

int linenum = 0;    /* for error messages */

char *file_name;
char *getline();
unchar hexbyte();

/**
 *  This function has been taken with little change directly from the 
 *  ISC loadfont command.
 *
 *  The purpose of this function is to correctly read a BDF format
 *  font description file, and convert the information into the format
 *  used by the WS_PIO_ROMFONT ioctl.
 *
 *  NOTE:  Most of the BDF format information is ignored - we really
 *         only use the encoding number and the bit map.
 **/
void
load_font(font_file)
char *font_file;
{
    register int i;
	int dummy_x;
	int dummy_y;
    int	num_chars = 0;
    int	nchars;
	int nprops;
	int p_size;		/*  Point size for font  */

    char linebuf[BUFSIZ];
    char namebuf[100];
    char font_name[100];
	unchar *bmap_ptr;
	unchar *bmap_ptr2;
   
    FILE *fp;

	/*  Set file_name for the error routine  */
	file_name = font_file;

	/*  Open the font file for reading  */
    if ((fp = fopen(font_file, "r")) == NULL)
	    fatal("could not open BDF font file");

	/*  Format of BDF font file must start with
	 *      STARTFONT <version>
	 *      FONT <font_size>
	 *      SIZE <x> <y> <z>
	 *      FONTBOUNDINGBOX <a> <b> <c> <d>
	 *      STARTPROPERTIES <n>
	 *         .
	 *         .
	 *      ENDPROPERTIES
	 *      CHARS 256 (usually)
	 *
	 *  Most of this information is ignored, as it is not required for the
	 *  downloaded bitmaps.
	 */
    getline(fp, linebuf);

    if ((sscanf(linebuf, "STARTFONT %s", namebuf) != 1) ||
		strcmp(namebuf, "2.1") != 0)
		fatal("bad 'STARTFONT' in font file");

    getline(fp, linebuf);

    if (sscanf(linebuf, "FONT %[^\n]", font_name) != 1)
		fatal("bad 'FONT' in font file");

    getline(fp, linebuf);

    if (!prefix(linebuf, "SIZE"))
		fatal("missing 'SIZE' in font file");

	/*  Extract the point size from the SIZE parameters  */
	if (sscanf(linebuf, "SIZE %d %d %d", &p_size, &dummy_x, &dummy_y) != 3)
		fatal("SIZE syntax not valid");

	/*  Only 8, 14 and 16 are valid sizes  */
	if (p_size != 8 && p_size != 14 && p_size != 16)
		fatal("Invalid pointsize");

    getline(fp, linebuf);

    if (!prefix(linebuf, "FONTBOUNDINGBOX"))
		fatal("pcfont: missing 'FONTBOUNDINGBOX' in font file");

    getline(fp, linebuf);

    if (!prefix(linebuf, "STARTPROPERTIES"))
		fatal("pcfont: missing 'STARTPROPERTIES' in font file");

	if (sscanf(linebuf, "STARTPROPERTIES %d", &nprops) != 1)
	   fatal("pcfont: bad 'STARTPROPERTIES' in font file");

	getline(fp, linebuf);

	/*  Read in the properties (no processing to be done)  */
	while((nprops-- > 0) && !prefix(linebuf, "ENDPROPERTIES"))
		getline(fp, linebuf);

	if (!prefix(linebuf, "ENDPROPERTIES"))
		fatal("pcfont: missing 'ENDPROPERTIES' in font file");

	/*  Check that we got as many properties as we were told to expect  */
	if (nprops != -1)
		fatal("pcfont: %d too few properties in font file", nprops + 1);

    getline(fp, linebuf);

	/*  First really useful bit of information  */
    if (sscanf(linebuf, "CHARS %d", &nchars) != 1)
		fatal("bad 'CHARS'");

	/*  Must be at least one character, otherwise what's the point of 
	 *  this file?
	 */
    if (nchars < 1)
		fatal("pcfont: invalid number of CHARS");

	/*  Set the number of chars in the font map  */
	font_map.fnt_numchar = nchars;

    getline(fp, linebuf);

	/*  The loop that does the real work of loading in the bit maps  */
    while ((nchars-- > 0) && prefix(linebuf, "STARTCHAR"))
	{
		register int row;	/*  Loop variable for bit-map rows  */
		int i;		/*  General purpose int  */
		int	bw;		/*  bounding-box width  */
		int	bh;		/*  bounding-box height  */
		int	bl;		/*  bounding-box left  */
		int	bb;		/*  bounding-box bottom  */
		int	enc;	/*  encoding value 1  */
		int enc2;	/*  encoding value 2  */

		char *p;	/* temp pointer into linebuf */
		char char_name[100];

		if (sscanf(linebuf, "STARTCHAR %s", char_name) != 1)
			fatal("pcfont: bad character name");

		getline(fp, linebuf);

		if ((i = sscanf(linebuf, "ENCODING %d %d", &enc, &enc2)) < 1)
			fatal("pcfont: bad 'ENCODING'");

		/*  Check for invalid encoding values  */
		if ((enc < -1) || ((i == 2) && (enc2 < -1)))
			fatal("pcfont: bad ENCODING value");

		if (i == 2 && enc == -1)
			enc = enc2;

		/*  If we don't have a valid encoding value, we print a warning 
		 *  and ignore the character
		 */
		if (enc == -1)
		{
			fprintf(stderr, "pcfont: character '%s' on line %d ignored\n",
				linenum, char_name);

			do
			{
				if (!getline(fp,linebuf))
					fatal("pcfont: Unexpected EOF");

			} while (!prefix(linebuf, "ENDCHAR"));

			getline(fp, linebuf);
			continue;
		}

		/*  Check that the value is within the legal range  */
		if (enc > MAXENCODING)
			fatal("character '%s' has an encoding (%d) which is too large", char_name, enc);

		/*  Record the character encoding value  */
		font_map.fnt_chars[num_chars].cd_index = enc;

		/*  Skip over the SWIDTH and DWIDTH lines, which we don't need  */
		getline(fp, linebuf);
		getline(fp, linebuf);
		getline(fp, linebuf);

		/*  Extract bitmap parameters  */
		if (sscanf( linebuf, "BBX %d %d %d %d", &bw, &bh, &bl, &bb) != 4)
			fatal("bad 'BBX'");

		/*  Check for invalid parameters  */
		if (bh < 0)
			fatal("character '%s' has invalid sized bitmap, %dx%d", 
				  char_name, bw, bh);

		if (bw != 8)	/*  Only size supported  */
			fatal("character '%s' has invalid bitmap width %d", bw);

		getline(fp, linebuf);

		/*  Check for attributes, which again we ignore  */
		if (prefix(linebuf, "ATTRIBUTES"))
			getline(fp, linebuf);

		if (!prefix(linebuf, "BITMAP"))
			fatal("missing 'BITMAP'");

		/*  Set the pointer for where to store the bitmap  */
		switch (p_size) {
			case 8:
				bmap_ptr = font_map.fnt_chars[num_chars].cd_map_8x8;
				break;

			case 14:
				bmap_ptr = font_map.fnt_chars[num_chars].cd_map_8x14;
				break;

			case 16:
				bmap_ptr = font_map.fnt_chars[num_chars].cd_map_8x16;
				bmap_ptr2 = font_map.fnt_chars[num_chars].cd_map_9x16;
				break;
		}

		for (row = 0; row < bh; row++)
		{
			getline(fp,linebuf);

			if (strlen(linebuf) != 2)
				fatal("Illegal number of characters in hex encoding");

			*(bmap_ptr++) = hexbyte(linebuf);

			/*  Fill in 9x16 as well for 16 point size  */
			if (p_size == 16)
				*(bmap_ptr2++) = hexbyte(linebuf);
		}

		getline( fp,linebuf);

		/*  Check for ENDCHAR  */
		if (!prefix(linebuf, "ENDCHAR"))
            fatal("missing 'ENDCHAR'");

		getline(fp, linebuf);
		++num_chars;
	}

    if (nchars != -1)
        fatal("%d too few characters", nchars + 1);

    if (prefix(linebuf, "STARTCHAR"))
		fatal("more characters than specified");

    if (!prefix(linebuf, "ENDFONT"))
        fatal("missing 'ENDFONT'");

	/*  Just for lint  */
    return;
}

char *
getline(fp,s)
FILE *fp;
char *s;
{
	int len;

    s = fgets(s, 80, fp);
    linenum++;

    while (s)
	{
		len = strlen(s);

		/*  Strip off new line  */
		if (len && s[len - 1] == '\n' || s[len - 1] == '\015')
			s[--len] = '\0';
		
		/*  Skip over comments  */
		if (len == 0 || prefix(s, "COMMENT"))
		{
			s = fgets(s, 80, fp);	/*  Grab new line  */
			linenum++;
		}
		else
			break;
    }
    return(s);
}

/*VARARGS*/
fatal(msg, p1, p2, p3, p4)
char *msg, *p1;
{
	fprintf(stderr, "pcfont: %s: ", file_name);
	fprintf(stderr, msg, p1, p2, p3, p4);

	/*  Printing the line number doesn't always make sense  */
	if (linenum != 0)
		fprintf(stderr, " at line %d\n", linenum);
	else
		fprintf(stderr, "\n");

	exit(1);
}

/*
 * return TRUE if str is a prefix of buf
 */
prefix(buf, str)
char *buf, *str;
{
    return strncmp(buf, str, strlen(str))? FALSE : TRUE;
}

/*
 * make a byte from the first two hex characters in s
 */
unsigned char
hexbyte(s)
char *s;
{
    int i;

    unsigned char b = 0;
    register char c;

    for (i = 2; i; i--)
	{
		c = *s++;

		if ((c >= '0') && (c <= '9'))
			b = (b<<4) + (c - '0');
		else if ((c >= 'A') && (c <= 'F'))
			b = (b<<4) + 10 + (c - 'A');
		else if ((c >= 'a') && (c <= 'f'))
			b = (b<<4) + 10 + (c - 'a');
		else
			fatal("bad hex char '%c'", c);
    } 
    return b;
}
