#ident	"@(#)xpr:devices/postscript/output.c	1.3"
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

#include "Xlib.h"

extern char *		hex_table_MSNFirst[];
extern char *		hex_table_LSNFirst[];

static char **		HEXsize	= hex_table_MSNFirst;
static char **		HEXdata = 0;	/* depends on endian() */

static void		optimum_output();
static void		flush_bytes();

/**
 ** ps_output_bytes() - PUT OUT A ROW OF BYTES
 **/

int			ps_output_bytes (bytes, width)
	unsigned char *		bytes;
	int			width;
{
	unsigned char *		last_byte	= 0;
	unsigned char *		pb;

	int			c;
	int			run_count	= 0;


	if (!HEXdata)
		if (endian() != MSBFirst)
			HEXdata = hex_table_LSNFirst;
		else
			HEXdata = hex_table_MSNFirst;

	/*
	 * Look for runs of identical bytes.
	 */
	for (c = 0, pb = bytes; c < width; c++, pb++) {
		if (last_byte) {
			if (*pb == *last_byte) {
				run_count++;
				continue;
			}
			optimum_output (last_byte, run_count, 0);
		}
		last_byte = pb;
		run_count = 1;
	}
	if (last_byte)
		optimum_output (last_byte, run_count, 1);

	return (1);
}

/**
 ** optimum_output() - OPTIMIZE PUTTING OUT ROW OF BYTES
 **/

/*
 * Note: It is assumed that "bytes" points into the buffer of
 * bytes. This allows the routine to just save the pointer for
 * later reference to an entire run of contiguous bytes.
 */

static void	optimum_output (bytes, run_count, flush)
	unsigned char		*bytes;
	int			run_count,
				flush;
{
	static unsigned char	*string_start	= 0;

	static int		string_length	= 0;


	/*
	 * If the run is long enough to bother with,
	 * we compute the control sequence and see if it
	 * is shorter than just sending copies of the bytes.
	 */

#define REPEAT_PATTERN	"rXXXXxx\n"
#define REPEAT_LENGTH	(sizeof(REPEAT_PATTERN)-1)

	if (run_count * 2  > REPEAT_LENGTH) {
		static char		*rep_buf = REPEAT_PATTERN;

		if (string_length) {
			flush_bytes (string_start, string_length);
			string_length = 0;
		}
			
		memcpy (rep_buf + 1, HEXsize[run_count / 256], 2);
		memcpy (rep_buf + 3, HEXsize[run_count % 256], 2);
		memcpy (rep_buf + 5, HEXdata[*bytes], 2);

		fwrite ((char *)rep_buf, REPEAT_LENGTH, 1, stdout);
		return;
	}

	/*
	 * Don't actually put anything out (unless "flush" is set).
	 * Since all the bytes before "flush" being set are to
	 * be contiguous, we need only keep track of the starting byte
	 * and the number of bytes.
	 */
	if (!string_length)
		string_start = bytes;
	string_length += run_count;

	if (flush) {
		flush_bytes (string_start, string_length);
		string_length = 0;
	}

	return;
}

/**
 ** flush_bytes() - PUT OUT STRING OF BYTES
 **/

static void		flush_bytes (start, length)
	unsigned char *		start;
	int			length;
{
	static char *		sbim_buf = "dXXXX";

	int			count;


	memcpy (sbim_buf + 1, HEXsize[length / 256], 2);
	memcpy (sbim_buf + 3, HEXsize[length % 256], 2);
	fwrite (sbim_buf, 5, 1, stdout);

	/*
	 * The PostScript program that reads the data ignores
	 * whitespace, so we can afford to break up long lines.
	 */
	for (count = 0; length--; count++) {
		if (!(count % 32))
			fputs ("\n ", stdout);
		fwrite (HEXdata[*start++], 2, 1, stdout);
	}
	fputs ("\n", stdout);

	return;
}
