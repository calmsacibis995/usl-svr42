/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpTxtPer.c	1.3"

/*
	HISTORY:

	An sample implementation for Xi18n function of X11R5
	based on the public review draft 
	"Xlib Changes for internationalization,Nov.1990"
	by Katsuhisa Yano,TOSHIBA Corp. and Osamu Touma,SORD Computer Corp..

	Modification to the high level pluggable interface is done
	by Takashi Fujiwara,FUJITSU LIMITED.
*/

/*
 *	_Ximp_mb_extents_per_char()
 *	_Ximp_wc_extents_per_char()
 */

{
    Ximp_XLCd lcd = (Ximp_XLCd) xfont_set->core.lcd;
    unsigned char *strptr, strbuf[BUFSIZE];
    unsigned char *xchar_ptr, xchar_buf[BUFSIZE];
    XChar2b *xchar2b_ptr, xchar2b_buf[BUFSIZE];
    FontSetRec *fontset;
    XFontStruct *font;
    XCharStruct *def, *cs, overall;
    int (*cnv_func)();
    int cset_num, char_length;
    int count, length, tmp_len, require_num;
    int logical_ascent, logical_descent;
    Bool first = True;

    cnv_func = lcd->ximp_lcpart->methods->CNV_FUNC;

    (*lcd->ximp_lcpart->methods->cnv_start)(lcd);

    bzero(&overall, sizeof(XCharStruct));
    logical_ascent = logical_descent = require_num = *num_chars = 0;

    while (text_length > 0) {
        length = BUFSIZE;
	count = (*cnv_func)(lcd, text, text_length, strbuf, &length,
			    &cset_num, &char_length);
	if (count <= 0)
	    break;

	text += count;
	text_length -= count;

	strptr = strbuf;
	fontset = ((Ximp_XFontSet) xfont_set)->ximp_fspart->fontset + cset_num;
	if (fontset == NULL)
	    continue;
	while (length > 0) {
	    tmp_len = BUFSIZE;
	    if (char_length < 2)
		count = _Ximp_cstoxchar(fontset, strptr, length,
					xchar_buf, &tmp_len, &font);
	    else
		count = _Ximp_cstoxchar2b(fontset, strptr, length,
					xchar2b_buf, &tmp_len, &font);
	    if (count <= 0)
		break;

	    strptr += count;
	    length -= count;

	    if (require_num) {
		require_num += count;
		continue;
	    }
	    if (buf_size < tmp_len) {
		require_num = *num_chars + count;
		continue;
	    }

	    if (first) {
		logical_ascent = font->ascent;
		logical_descent = font->descent;
	    } else {
		logical_ascent = max(logical_ascent, font->ascent);
		logical_descent = max(logical_descent, font->descent);
	    }

	    strptr += count;
	    length -= count;

	    if (require_num) {
		require_num += count;
		continue;
	    }
	    if (buf_size < tmp_len) {
		require_num = *num_chars + count;
		continue;
	    }

	    if (first) {
		logical_ascent = font->ascent;
		logical_descent = font->descent;
	    } else {
		logical_ascent = max(logical_ascent, font->ascent);
		logical_descent = max(logical_descent, font->descent);
	    }

	    if (char_length < 2) {
		CI_GET_DEFAULT_INFO_1D (font, def)
		xchar_ptr = xchar_buf;
	    } else {
		CI_GET_DEFAULT_INFO_2D (font, def)
		xchar2b_ptr = xchar2b_buf;
	    }

	    while (tmp_len-- > 0) {
		if (char_length < 2) {
		    CI_GET_CHAR_INFO_1D (font, *xchar_ptr, def, cs)
		    xchar_ptr++;
		} else {
		    CI_GET_CHAR_INFO_2D (font, xchar2b_ptr->byte1,
					 xchar2b_ptr->byte2, def, cs)
		    xchar2b_ptr++;
		}
		if (cs == NULL)
		    continue;

	        ink_buf->x = overall.width + cs->lbearing;
		ink_buf->y = -(cs->ascent);
		ink_buf->width = cs->rbearing - cs->lbearing;
		ink_buf->height = cs->ascent + cs->descent;
		ink_buf++;

		logical_buf->x = overall.width;
		logical_buf->y = -(font->ascent);
		logical_buf->width = cs->width;
		logical_buf->height = font->ascent + font->descent;
		logical_buf++;

		if (first) {
		    overall = *cs;
		    first = False;
		} else {
		    overall.ascent = max(overall.ascent, cs->ascent);
		    overall.descent = max(overall.descent, cs->descent);
		    overall.lbearing = min(overall.lbearing,
					   overall.width + cs->lbearing);
		    overall.rbearing = max(overall.rbearing,
					   overall.width + cs->rbearing);
		    overall.width += cs->width;
		}

		(*num_chars)++;
	    }
	}
    }

    (*lcd->ximp_lcpart->methods->cnv_end)(lcd);

    if (require_num) {
	*num_chars = require_num;
	return 0;
    } else {
	if (overall_ink) {
	    overall_ink->x = overall.lbearing;
	    overall_ink->y = -(overall.ascent);
	    overall_ink->width = overall.rbearing - overall.lbearing;
	    overall_ink->height = overall.ascent + overall.descent;
	}

	if (overall_logical) {
	    overall_logical->x = 0;
	    overall_logical->y = -(logical_ascent);
	    overall_logical->width = overall.width;
	    overall_logical->height = logical_ascent + logical_descent;
	}
    }

    return 1;
}
