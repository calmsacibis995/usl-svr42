#ident	"@(#)xpr:devices/terminfo/xpr_term.h	1.7"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#define OKAY(S)		((S) && *(S))
#define umalloc(N)	(unsigned char *)Malloc(N)
#define ucalloc(N)	(unsigned char *)Calloc(N, 1)
#define icalloc(N)	(int *)Calloc(N, sizeof(int))

/*
 * The following is the largest number of colors we can handle
 * in an output device. The code assumes that (Terminfo) output
 * devices use real ribbons that have bands of colors. This
 * assumption leads in turn to the assumption only a few colors
 * can fit on a ribbon. Bottom line: The code assumes the output
 * device can only handle a few colors.
 */
#define MAX_COLORS	16

/*
 * The model for image printing:
 *
 *	set_data_storage	Initialize image printing routine and
 *				determine image storage format
 *	dump_image		Image printing routine
 *	does_wscale		Image printing routine can scale (width)
 *	does_hscale		Image printing routine can scale (height)
 *
 * Note that "does_[wh]scale" define a capability of our software, not
 * the printer. There are three places where scaling can be done: In
 * the "pix_convert()" routine, in the image printing routine, or in
 * the device. The full scaling may be partitioned among all three,
 * to take advantage of natural efficiencies in each place.
 */
struct model {
	void			(*set_data_storage)(),
				(*dump_image)();
	int			does_wscale,
				does_hscale;
};

/*
 * For now we use an internal table of ``X color names''.
 */
struct colorname {
	unsigned char		red,
				green,
				blue;
	char			*name1,
				*name2;
};

extern struct model	models[];

extern int		nmodels;

extern struct colorname colornames[];

extern unsigned short	tidbit_boolean;

extern short		tidbit_number;

extern char		*tidbit_string;

extern int		tidbit();

extern short		cols,
			lines,
			orc,
			orhi,
			orl,
			orvi,
			npins,
			spinh,
			spinv,	/* pin density */
			SPINV,	/* dot density */
			bitwin,
			bitype,
			colors,
			hls;

extern char		*ff,
			*porder,
			*sbim,
			*rbim,
			*defbi,
			*endbi,
			*birep,
			*binel,
			*bicr,
			*initc,
			*setcolor,
			*colornm,
			**hex_table,
			*hex_table_MSNFirst[],
			*hex_table_LSNFirst[];

extern int		convert_to_hex;

extern void		read_terminfo_database(),
			ti_prologue(),
			ti_epilogue(),
			ti_backup_epilogue(),
			set_motion_scale(),
			reset_hmove(),
			hmove(),
			reset_vmove(),
			vmove(),
			image_to_cells(),
			init_image_to_cells(),
			image_to_bits(),
			init_image_to_bits(),
			output_nel(),
			output_cr(),
			start_bit_image_graphics(),
			stop_bit_image_graphics(),
			image_bitblt(),
			pick_best_font(),
			map_colors(),
#ifndef MEMUTIL
			free(),
#endif /* MEMUTIL */
			exit(),
			_exit();

#if defined(SYSV) || defined(SVR4)
extern char		*strfld(),
			*units_to_hex();
#else
#ifndef MEMUTIL
extern char		*malloc(),
			*calloc();
#endif /* MEMUTIL */
extern char		*strfld(),
			*units_to_hex();
#endif

extern int		calc_bytes_per_line(),
			istrfld(),
			set_lrmargin(),
			output_units();

extern unsigned int	uistrfld();

#if	defined(_XLIB_H_)
extern XImage		*text_to_image();
#endif
