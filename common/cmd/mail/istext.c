/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/istext.c	1.7.2.2"
#ident "@(#)istext.c	1.9 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	istext - check a line for text, non-text characters

    SYNOPSIS
	t_Content istext(unsigned char *line, int size, t_Content cur_content)

    DESCRIPTION
	istext() looks at the "size" characters within "line"
	for non-text characters. The definition of text
	characters when sending is based on the MTA spec and is
	specifically the 7-bit ASCII printable characters. Generic
	text characters are those which are printable according
	to the current locale.
	Printable text is defined by isprint(), white space (as defined by
	isspace()) and backspaces.

    RETURNS
	C_Text - all characters are 7-bit ASCII
	C_GText - all characters are text according to the current locale
	C_Binary - at least one non-text character was found
*/

static const char is7bitprintable[] =
    {
    /* nul */ 0, /* soh */ 0, /* stx */ 0, /* etx */ 0, /* eot */ 0, /* enq */ 0, /* ack */ 0, /* bel */ 0,
    /* bs  */ 1, /* ht  */ 1, /* nl  */ 1, /* vt  */ 1, /* np  */ 1, /* cr  */ 1, /* so  */ 0, /* si  */ 0,
    /* dle */ 0, /* dc1 */ 0, /* dc2 */ 0, /* dc3 */ 0, /* dc4 */ 0, /* nak */ 0, /* syn */ 0, /* etb */ 0,
    /* can */ 0, /* em  */ 0, /* sub */ 0, /* esc */ 0, /* fs  */ 0, /* gs  */ 0, /* rs  */ 0, /* us  */ 0,
    /* sp  */ 1, /* !   */ 1, /* "   */ 1, /* #   */ 1, /* $   */ 1, /* %   */ 1, /* &   */ 1, /* '   */ 1,
    /* (   */ 1, /* )   */ 1, /* *   */ 1, /* +   */ 1, /* ,   */ 1, /* -   */ 1, /* .   */ 1, /* /   */ 1,
    /* 0   */ 1, /* 1   */ 1, /* 2   */ 1, /* 3   */ 1, /* 4   */ 1, /* 5   */ 1, /* 6   */ 1, /* 7   */ 1,
    /* 8   */ 1, /* 9   */ 1, /* :   */ 1, /* ;   */ 1, /* <   */ 1, /* =   */ 1, /* >   */ 1, /* ?   */ 1,
    /* @   */ 1, /* A   */ 1, /* B   */ 1, /* C   */ 1, /* D   */ 1, /* E   */ 1, /* F   */ 1, /* G   */ 1,
    /* H   */ 1, /* I   */ 1, /* J   */ 1, /* K   */ 1, /* L   */ 1, /* M   */ 1, /* N   */ 1, /* O   */ 1,
    /* P   */ 1, /* Q   */ 1, /* R   */ 1, /* S   */ 1, /* T   */ 1, /* U   */ 1, /* V   */ 1, /* W   */ 1,
    /* X   */ 1, /* Y   */ 1, /* Z   */ 1, /* [   */ 1, /* \   */ 1, /* ]   */ 1, /* ^   */ 1, /* _   */ 1,
    /* `   */ 1, /* a   */ 1, /* b   */ 1, /* c   */ 1, /* d   */ 1, /* e   */ 1, /* f   */ 1, /* g   */ 1,
    /* h   */ 1, /* i   */ 1, /* j   */ 1, /* k   */ 1, /* l   */ 1, /* m   */ 1, /* n   */ 1, /* o   */ 1,
    /* p   */ 1, /* q   */ 1, /* r   */ 1, /* s   */ 1, /* t   */ 1, /* u   */ 1, /* v   */ 1, /* w   */ 1,
    /* x   */ 1, /* y   */ 1, /* z   */ 1, /* {   */ 1, /* |   */ 1, /* }   */ 1, /* ~   */ 1, /* del */ 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x80 - 0x8F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x90 - 0x9F */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xA0 - 0xAF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xB0 - 0xBF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xC0 - 0xCF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xD0 - 0xDF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xE0 - 0xEF */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* 0xF0 - 0xFF */
    };

#ifdef SVR4_1
#include <wctype.h>
#endif

t_Content istext(s, size, cur_content)
register unsigned char	*s;
int 		size;
t_Content	cur_content;
{
    register unsigned char *ep = s + size;
    register foundnontext;

    switch (cur_content)
	{
	case C_Text:
	    /* Look for characters which aren't 7-bit ASCII printables. */
	    foundnontext = FALSE;
	    for (; s < ep; s++)
		{
		if (is7bitprintable[*s])
		    continue;
		foundnontext = TRUE;
		break;
		}
	    /* If any are found, then continue checking for non-generic-text printables */
	    if (!foundnontext)
		return C_Text;
	    /* FALLTHROUGH */

#ifdef SVR4_1
#define SS2 0x8E
#define SS3 0x8F
#define ISASCII(c) (((c)&0x80) != 0)
	case C_GText:
	    /* look for characters which aren't locale-specific printables */
	    while (s < ep)
		{
		register int c = *s;
		/* Code sets 1, 2 and 3 */
		if ((c == SS2 || c == SS3) || ISASCII(c))
		    {
		    /* Convert k bytes to a wide character. */
		    wchar_t w;
		    int k = mbtowc(&w, (char*)s, ep - s);
		    if (k == -1)
			return C_Binary;
		    s += k;
		    /* Is our wide character printable? */
		    if (!(iswprint(w) || iswspace(w)))
			return C_Binary;
		    }

		/* Code sets 0 */
		else
		    {
		    /* Is our byte printable? */
		    if (!(isprint(c) || isspace(c) || (c == '\b')))
			return C_Binary;
		    s++;
		    }
		}

	    return C_GText;
	    /* FALLTHROUGH */
#endif

	default:
	case C_Binary:
	    return C_Binary;
	}
}
