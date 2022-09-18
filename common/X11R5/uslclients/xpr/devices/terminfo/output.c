#ident	"@(#)xpr:devices/terminfo/output.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "memory.h"

#include "xpr_term.h"

#define UNITCPY(D,S) memcpy((char *)(D),(char *)(S),bytes_per_unit)
#define	UNITEQU(A,B) (memcmp((char *)(A),(char *)(B),bytes_per_unit) == 0)
#define UNITPUT(X,N) fwrite((char *)(X),bytes_per_unit,(N),stdout)

extern char		*tparm(),
			*strdup();

static void		optimum_unitput(),
			flush_units(),
			_putchar(),
			set_color_band();

static char		*repeat_unit();

static unsigned char	*bytes_to_hex();

#if	defined(DO_MOTION)
static int		do_hmove	= 0,
			count_bitwin,
			hpos_left,
			hpos_right,
			vpos;
#else
static int		hpos_left;
#endif

/*
 * The "output_units()" routine is called in the following way:
 *
 *	defbi(x, y, width, height, scale)
 *	for (each unit row)
 *	do
 *		output_units (unit, bytes_per_unit, width, scale)
 *	done
 *	endbi()
 *
 * The code for "output_units()" flows like this:
 *
 *	if (birep defined)
 *	then
 *		for (each run of like/unlike units)
 *		do
 *			if (run of like units)
 *			then
 *				birep(run-length, unit)
 *			else
 *				sbim(run-length)
 *				(output units)
 *				rbim()
 *			fi
 *		done
 *	else
 *		sbim(width)
 *		(output units)
 *		rbim()
 *	fi
 *	binel()
 *
 * It appears that all printers can be put into this general form,
 * perhaps by clever construction of the string capabilities.
 */

/**
 ** start_bit_image_graphics()
 ** stop_bit_image_graphics()
 **/

#if	defined(DO_MOTION)
static char		*smam;

static int		turn_on_am;
#endif

void			start_bit_image_graphics (x, y, width, height, scale, depth)
	int			x,
				y,
				width,
				height,
				scale,
				depth;
{
	/*
	 * The first four arguments have units of ``dots'', which are
	 * what the device will print for each pixel. (We may stutter
	 * and print each pixel more than once to build up larger pixels
	 * that map one-to-one to the dumped image pixels.) The device
	 * may scale these dots up by "scale", the last argument.
	 * If the device has a "defbi" capability, great--it will take
	 * all five arguments and deal with them. If not, we have to
	 * simulate it.
	 */

	if (OKAY(defbi))
		putp (tparm(defbi, x, y, width, height, scale, depth));

#if	defined(DO_MOTION)
	else {
		char			*rmam;

		int			am;


		set_motion_scale (scale);

		/*
		 * Set the margins as an attempt to corral the dot mapped
		 * image. If this works, then simply putting out a carriage
		 * return (plus vertical motion) will cause the next line
		 * to begin at the left side of the image. If we can't set
		 * the margins, that's okay. Note that in either case a
		 * call to "hmove()" with the argument "x" will work, as
		 * "hmove()" takes care of optimizing the motion.
		 */
		do_hmove = (set_lrmargins(x, x + width) == -1);
		hmove (x);
		vmove (y);

		/*
		 * Turn off the automatic right margin, if possible.
		 * This is a precaution against spurious newlines inserted
		 * by silly devices.
		 */
		tidbit ((char *)0, "am", &am);
		tidbit ((char *)0, "rmam", &rmam);
		tidbit ((char *)0, "smam", &smam);
		turn_on_am = 0;
		if (!am)
			/*
			 * It is already.
			 */
			;
		else if (!OKAY(rmam) || !OKAY(smam))
			/*
			 * It can't be turned off (or on).
			 */
			;
		else {
			putp (rmam);
			turn_on_am = 1;
		}

		/*
		 * Since "defbi" isn't defined, if "binel" is not
		 * defined then we have to simulate it. We'll need to
		 * know when to move a tiny bit (for entwining) and
		 * when to move the balance of the vertical motion.
		 * "count_bitwin" keeps track of this.
		 * We'll also need to know where to move.
		 */
		if (!OKAY(binel)) {
			count_bitwin = bitwin;
			hpos_right = x + width;
			vpos = y;
		}
	}
#endif	/* DO_MOTION */

	/*
	 * This is needed regardless of whether "defbi" is defined.
	 */
	hpos_left = x;

	return;
}

void			stop_bit_image_graphics ()
{
	set_color_band (0);

	if (OKAY(endbi))
		putp (tparm(endbi));

#if	defined(DO_MOTION)
	if (!OKAY(defbi)) {
		if (turn_on_am)
			putp (smam);
		(void)set_lrmargins (0, cols - 1);
	}
#endif

	return;
}

/**
 ** output_units() - PUT OUT A ROW OF UNITS
 **/

int			output_units (
				band,
				bytes,
				bytes_per_unit,
				width,
				scale,
				empty_row
			)
	int			band;
	unsigned char		*bytes;
	int			bytes_per_unit,
				width,
				scale,
				empty_row;
{
	unsigned char		*last_unit	= 0,
				*pb;

	int			c,
				run_count	= 0;


	/*
	 * For some printers we must put out units even if a complete
	 * row represents no ink. In general, these are those that
	 * count the number of units sent, expecting some number
	 * of units based on width x height. For other printers, we
	 * can ignore ``empty'' rows.
	 *
	 * The way we tell what kind of printer we have at hand is
	 * to figure that those that have a real "binel" or for which
	 * we have to simulate a "binel" aren't counting.
	 */
	if (empty_row && (OKAY(binel) || !OKAY(defbi)))
		return (0);

	/*
	 * If we are putting out hex, convert to hex first.
	 */
	if (convert_to_hex) {
		bytes = (unsigned char *)units_to_hex(bytes, bytes_per_unit, width);
		bytes_per_unit *= 2;
	}

	/*
	 * Switch to a new color band, if necessary.
	 */
	set_color_band (band);

	/*
	 * If there is no bit-image repeat capability, just
	 * put out the row of units.
	 */
	if (!OKAY(birep))
		flush_units (bytes, bytes_per_unit, width, scale);

	/*
	 * If there is a bit-image repeat capability, look for
	 * runs of identical units.
	 */
	else {
		for (
			c = 0, pb = bytes;
			c < width;
			c++, pb += bytes_per_unit
		) {
			if (last_unit) {
				if (UNITEQU(pb, last_unit)) {
					run_count++;
					continue;
				}
				optimum_unitput (
					last_unit,
					run_count,
					bytes_per_unit,
					scale,
					0
				);
			}
			last_unit = pb;
			run_count = 1;
		}
		if (last_unit)
			optimum_unitput (
				last_unit,
				run_count,
				bytes_per_unit,
				scale,
				1
			);
	}

	return (1);
}

/**
 ** output_nel() - DO GRAPHICS NEWLINE
 ** output_cr() - DO GRAPHICS CARRIAGE RETURN
 **/

static void		output_cr_or_nel();

void			output_nel ()
{
	output_cr_or_nel (binel);
	return;
}

void			output_cr ()
{
	output_cr_or_nel (bicr);
	return;
}

static void		output_cr_or_nel (cap)
	char			*cap;
{
	if (OKAY(cap))
		putp (tparm(cap, hpos_left));
#if	defined(DO_MOTION)
	else if (!OKAY(defbi)) {

		if (cap == binel) {
			/*
			 * If both "binel" and "defbi" are not defined,
			 * we have to simulate "binel". (If only "binel"
			 * is not defined, then we assume that the device
			 * does an automatic "binel".)
			 *
			 * Note: The units of motion are ``dots'', which
			 * have a different meaning if the device is doing
			 * (some of) the scaling. Thus devices that scale
			 * must have "binel" defined (or must be devices
			 * that do an automatic "binel").
			 */
			if (--count_bitwin <= 0) {
				count_bitwin = bitwin;
				vpos += npins * bitwin - (bitwin - 1);
			} else
				vpos++;
			vmove (vpos);
		}

		/*
		 * Putting out the bit image moved the horizontal
		 * position, so record the new position before
		 * trying any motion.
		 */
		reset_hmove (hpos_right);
		hmove (hpos_left);
	}
#endif	/* DO_MOTION */

	return;
}

/**
 ** optimum_unitput() - OPTIMIZE PUTTING OUT ROW OF UNITS
 **/

/*
 * Note: It is assumed that "unit" points into the buffer of
 * units. This allows the routine to just save the pointer for
 * later reference to an entire run of contiguous units.
 * 
 * Note: This routine assumes that "birep" is defined for the device.
 */

static void	optimum_unitput (unit, run_count, bytes_per_unit, scale, flush)
	unsigned char		*unit;
	int			run_count,
				bytes_per_unit,
				scale,
				flush;
{
	static unsigned char	*string_start	= 0;

	static int		string_length	= 0;

	int			_run_count	= run_count * scale;


	/*
	 * If the run is long enough to bother with,
	 * we compute the control sequence and see if it
	 * is shorter than just sending copies of the bytes.
	 */
	if (_run_count > 1) {
		register char		*s	=
			repeat_unit(unit, _run_count, bytes_per_unit);


		/*
		 * In addition to checking if the repeat form is
		 * more economical, we also check to see if it doesn't
		 * work for the length at hand, or for the value of
		 * the unit (nulls are the typical problem).
		 */
		if (OKAY(s) && strlen(s) < _run_count * bytes_per_unit) {
			register char		*copy	= 0;


			if (string_length) {
				/*
				 * Unfortunately, "tparm()" returns a
				 * pointer to a static buffer, so we can't
				 * keep more than one reference without
				 * copying.
				 */
				s = copy = strdup(s);
				flush_units (
					string_start,
					bytes_per_unit,
					string_length,
					scale
				);
				string_length = 0;
			}
			tputs (s, _run_count, _putchar);
			if (copy)
				free (copy);
			return;
		}
	}

	/*
	 * Don't actually put anything out (unless "flush" is set).
	 * Since all the bytes before "flush" being set are to
	 * be contiguous, we need only keep track of the starting unit
	 * and the number of units.
	 */
	if (!string_length)
		string_start = unit;
	string_length += run_count;	/* not scaled! */

	if (flush) {
		flush_units (
			string_start,
			bytes_per_unit,
			string_length,
			scale
		);
		string_length = 0;
	}

	return;
}

/**
 ** flush_units() - PUT OUT STRING OF UNITS
 **/

static void		flush_units (start, bytes_per_unit, length, scale)
	unsigned char		*start;
	int			bytes_per_unit,
				length,
				scale;
{
	unsigned char		*unit;

	int			_scale;


	if (OKAY(sbim))
		putp (tparm(sbim, length * scale));
	if (scale == 1)
		UNITPUT (start, length);
	else
		for (unit = start; length--; unit += bytes_per_unit)
			for (_scale = scale; _scale--; )
				UNITPUT (unit, 1);
	if (OKAY(rbim))
		/*
		 * There may be delay padding in "rbim", for those
		 * devices that take awhile to print bit-image data.
		 * Thus we call "tputs()" instead of "putp()", so that
		 * we can give the correct delay factor.
		 */
		tputs (rbim, length * scale, _putchar);

	return;
}

/**
 ** repeat_unit() - CONSTRUCT REPEAT SEQUENCE FOR A UNIT
 **/

static char		*repeat_unit (unit, run_count, bytes_per_unit)
	unsigned char		*unit;
	int			run_count,
				bytes_per_unit;
{
	unsigned char		unit_buf[7];


	UNITCPY (unit_buf, unit);
	return (tparm(
		birep,
		unit,
		run_count,
		unit_buf[0],
		unit_buf[1],
		unit_buf[2],
		unit_buf[3],
		unit_buf[4],
		unit_buf[5],
		unit_buf[6]
	));
}

/**
 ** _putchar() - FUNCTION EQUIVALENT OF putchar()
 **/

static void		_putchar (c)
	int			c;
{
	putchar (c);
	return;
}

/**
 ** set_color_band()
 **/

static void		set_color_band (band)
	int			band;
{
	static int		cur_band	= -1;


	if (bitype != 3 && cur_band != band) {
		putp (tparm(setcolor, band));
		cur_band = band;
	}
	return;
}
