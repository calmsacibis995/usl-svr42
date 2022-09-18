/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpTxtExt.c	1.3"

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
 *	_Ximp_mb_extents()
 *	_Ximp_wc_extents()
 */

{
    Ximp_XLCd lcd = (Ximp_XLCd) xfont_set->core.lcd;
    unsigned char *strptr, strbuf[BUFSIZE];
    unsigned char xchar_buf[BUFSIZE];
    XChar2b xchar2b_buf[BUFSIZE];
    FontSetRec *fontset;
    XFontStruct *font;
    int (*cnv_func)();
    int cset_num, char_length;
    int count, length, tmp_len;
    int direction, logical_ascent, logical_descent, tmp_ascent, tmp_descent;
    XCharStruct overall, tmp_overall;
    Bool first = True;

    cnv_func = lcd->ximp_lcpart->methods->CNV_FUNC;

    (*lcd->ximp_lcpart->methods->cnv_start)(lcd);

    bzero(&overall, sizeof(XCharStruct));
    logical_ascent = logical_descent = 0;

    bzero(&overall, sizeof(XCharStruct));
    logical_ascent = logical_descent = 0;

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

	    if (char_length < 2)
		XTextExtents(font, xchar_buf, tmp_len, &direction,
			     &tmp_ascent, &tmp_descent, &tmp_overall);
            else
		XTextExtents16(font, xchar2b_buf, tmp_len, &direction,
			       &tmp_ascent, &tmp_descent, &tmp_overall);

	    if (first) {	/* initialize overall */
		overall = tmp_overall;
		logical_ascent = tmp_ascent;
		logical_descent = tmp_descent;
		first = False;
	    } else {
		overall.lbearing = min(overall.lbearing,
				       overall.width + tmp_overall.lbearing);
		overall.rbearing = max(overall.rbearing,
				       overall.width + tmp_overall.rbearing);
		overall.ascent = max(overall.ascent, tmp_overall.ascent);
		overall.descent = max(overall.descent, tmp_overall.descent);
		overall.width += tmp_overall.width;
		logical_ascent = max(logical_ascent, tmp_ascent);
		logical_descent = max(logical_descent, tmp_descent);
	    }

	    strptr += count;
	    length -= count;
	}
    }

    (*lcd->ximp_lcpart->methods->cnv_end)(lcd);

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

    return overall.width;
}
