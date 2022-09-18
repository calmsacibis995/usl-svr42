#ident	"@(#)xpr:devices/terminfo/parse_porder.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "xpr.h"

#include "xpr_term.h"

/**
 ** parse_porder()
 **/

int			parse_porder (set_vec, bit_vec, byte_vec, bytes_per_unit)
	unsigned char		**set_vec,
				*bit_vec[2];
	int			**byte_vec,
				*bytes_per_unit;
{
	char			*dots,
				*s_dot,
				*_porder	= strdup(porder);

	unsigned char		bit;

	int			dot,
				byte,
				offset;


	convert_to_hex = 0;
Switch:	switch (*_porder) {
	case 'H':
		hex_table = hex_table_MSNFirst;
		goto SetHex;

	case 'h':
		hex_table = hex_table_LSNFirst;
SetHex:		convert_to_hex = 1;
		_porder++;
		goto Switch;

	default:
		break;
	}

	dots = strfld(_porder, ";");
	offset = istrfld((char *)0, ";", 0);

	/*
	 * We don't know ahead of time just how big to
	 * make the "set_mask" array; ideally it needs to
	 * be the same size as a bit-unit, i.e. "bytes_per_unit".
	 * But we don't know that until after we've gone through
	 * the following loop. However, space savings aren't
	 * our goal here, so we'll use the upper bound of
	 * "strlen(dots)".
	 */
	*set_vec = ucalloc(strlen(dots));

	bit_vec[0] = ucalloc(npins);
	bit_vec[1] = ucalloc(npins);
	*byte_vec = icalloc(npins);

	for (
		s_dot = strfld(dots, ","), bit = 1 << 7, byte = 0;
		s_dot;
		s_dot = strfld((char *)0, ",")
	) {
		register unsigned char	ink	= bit,
					noink	= 0;


		switch (*s_dot) {

		case 'o':
		case 0:
			break;

		case 'x':
		default:
BitStuckOn:		(*set_vec)[byte] |= bit;
			break;

		case '-':
			s_dot++;
			swap (ink, noink);
			/*FALLTHROUGH*/

		case '+': /* Not specified, but just in case.	*/
/*		case '0':    Can't start with 0			*/
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			dot = atoi(s_dot);
			if (dot < 1 || npins < dot)
				/*
				 * Uh oh, the Terminfo entry
				 * is bad!
				 */
				goto BitStuckOn;

			/*
			 * Terminfo counts from 1, we count (index)
			 * from 0.
			 */
			dot--;

			/*
			 * Allow more than a single bit per pin
			 * (for printers like the 455 or Qume that
			 * need to compute the ASCII value for a
			 * character, e.g. '.', as a single dot.)
			 */
			bit_vec[0][dot] |= noink;
			bit_vec[1][dot] |= ink;

			(*byte_vec)[dot] = byte;
			break;
		}

		/*
		 * Increment the bit-mask and byte position,
		 * but save the byte position value just used
		 * as a reference to the maximum number of bytes
		 * per output cell.
		 */
		*bytes_per_unit = byte + 1;
		if (!(bit >>= 1)) {
			bit = 1 << 7;
			byte++;
		}
	}

	return (offset);
}
