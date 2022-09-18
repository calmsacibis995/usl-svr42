#ident	"@(#)xpr:devices/terminfo/ti_prologue.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "signal.h"
#include "stdio.h"
#include "termio.h"

#include "xpr.h"

#include "xpr_term.h"

#define	PS_TRAILER	"%%Trailer\n"
#define PS_BOUNDINGBOX	"%%BoundingBox"
#define PS_BB_FORMAT	"%s %ld %ld %ld %ld\n"

static long		bb_ll_x,
			bb_ll_y,
			bb_ur_x,
			bb_ur_y;

static void		my_system(),
			my_signal();

static char		*get_epilogue();

static struct termio	orig_stty,
			curr_stty;

/**
 ** ti_prologue() - PUT OUT PROLOGUE (GENERAL INITIALIZATION SEQUENCES)
 **/

void			ti_prologue (TERM, fp)
	char			*TERM;
	FILE			*fp;
{
	static char		*prefix	= "tput -T",
				*suffix	= " init";

	char			*buf;

	int			fd	= fileno(fp);


	/*
	 * Since the data we'll be putting out is typically binary,
	 * turn off output post-processing by the system.
	 * Trap usual back-door exits so this can be undone.
	 */
	if (isatty(fd)) {
		ioctl (fd, TCGETA, &orig_stty);
		curr_stty = orig_stty;
		curr_stty.c_oflag &= ~OPOST;
		ioctl (fd, TCSETAW, &curr_stty);
	}
	my_signal (SIGHUP, ti_epilogue);
	my_signal (SIGINT, ti_epilogue);
	my_signal (SIGQUIT, ti_epilogue);
	my_signal (SIGTRAP, ti_epilogue);

	/*
	 * Slow but accurate. See comment in "my_system()" explaining
	 * why we don't use "system()". (It's not because we don't know
	 * if we're using stdout.)
	 */
	buf = Malloc(strlen(prefix) + strlen(TERM) + strlen(suffix) + 1);
	sprintf (buf, "%s%s%s", prefix, TERM, suffix);
	my_system (buf, fd);

#if	defined(DO_MOTION)
	reset_hmove (0);
	reset_vmove (0);
#endif

	return;
}

/**
 ** ti_epilogue() - PUT OUT EPILOGUE (CLOSING CONTROL SEQUENCES)
 **/

void			ti_epilogue (TERM, fp, x, y, width, height, scale)
	char			*TERM;
	FILE			*fp;
	int			x,
				y,
				width,
				height,
				scale;
{
	int			fd		= fileno(fp);

	char			*epilogue	= get_epilogue(TERM);


	/*
	 * The following must be the last byte(s) in the output
	 * (except for PostScript output).
	 */
	if (OKAY(epilogue))
		putp (epilogue);

	if (STREQU(TERM, "postscript")) {
		long			ll_x	= scale * x,
					ll_y	= lines * orl - scale * (y + height),
					ur_x	= scale * (x + width),
					ur_y	= scale * y;


		if (ll_x < bb_ll_x)
			bb_ll_x = ll_x;
		if (ll_y < bb_ll_y)
			bb_ll_y = ll_y;
		if (ur_x > bb_ur_x)
			bb_ur_x = ur_x;
		if (ur_y > bb_ur_y)
			bb_ur_y = ur_y;
		printf ("%s", PS_TRAILER);
		printf (
			PS_BB_FORMAT,
			PS_BOUNDINGBOX,
			bb_ll_x,
			bb_ll_y,
			bb_ur_x,
			bb_ur_y
		);
	}

	if (isatty(fd))
		ioctl (fd, TCSETAW, &orig_stty);

	return;
}

/**
 ** ti_backup_epilogue() - BACKUP OVER PREVIOUS EPILOGUE IN A FILE
 **/

void			ti_backup_epilogue (TERM, fp)
	char			*TERM;
	FILE			*fp;
{
	int			len,
				new_loc;

	char			*last_bytes,
				*epilogue	= 0;


	/*
	 * If the ``file'' isn't seekable, then it isn't capable of
	 * being changed.
	 */
	if (fseek(fp, 0L, 2) != 0)
		return;

	if (!STREQU(TERM, "postscript"))
		new_loc = ftell(fp);
	else {
		char			line[BUFSIZ];


		/*
		 * Search for the trailer, then back up over the
		 * ``epilogue''.
		 */
		rewind (fp);
	 	while ((new_loc = ftell(fp), fgets(line, BUFSIZ, fp)))
			if (STREQU(line, PS_TRAILER))
				break;
		if (feof(fp) || ferror(fp))
			return;

		/*
		 * Now search for the bounding box line, and parse the
		 * values from it.
		 */
		while (fgets(line, BUFSIZ, fp))
			if (STREQU(line, PS_BOUNDINGBOX))
				break;
		if (!feof(fp) && !ferror(fp)) {
			char			*toss	= PS_BOUNDINGBOX;


			sscanf (
				line,
				PS_BB_FORMAT,
				&toss,
				&bb_ll_x,
				&bb_ll_y,
				&bb_ur_x,
				&bb_ur_y
			);
		} else {
			bb_ll_x = cols * orc;
			bb_ll_y = lines * orl;
			bb_ur_x = 0;
			bb_ur_y = 0;
		}
	}

	epilogue = get_epilogue(TERM);
	if (!OKAY(epilogue))
		return;

	len = strlen(epilogue);
	new_loc -= len;
	if (new_loc >= 0) {
		last_bytes = Malloc(len);
		fseek (fp, new_loc, 0);
		if (
			fread(last_bytes, len, 1, fp) == 1
		     && memcmp(last_bytes, epilogue, len) == 0
		)
			fseek (fp, new_loc, 0);
		/* else
			the "fread()" moved pointer back to eof */
	}

	return;
}

/**
 ** get_epilogue() - EXTRACT THE EPILOGUE CONTROL SEQUENCES FROM TERMINFO
 **/

static char		*get_epilogue (TERM)
	char			*TERM;
{
	if (!OKAY(ff))
		tidbit (TERM, "ff", &ff);	/* might still be !OKAY */
	if (!OKAY(ff))
		ff = "";

	return (ff);
}

/**
 ** my_system() - "system()" WITH STANDARD INPUT/ERROR CLOSED
 **/

static void		my_system (command, fdout)
	char			*command;
	int			fdout;
{
	/*
	 * We close the standard input and error to avoid any
	 * confusion that might arise from them being connected
	 * to a different device than what "TERM" names.
	 * (E.g. "cols" and "lines" being determined from the window
	 * size, overriding what's in Terminfo; e.g. changing termio
	 * settings.)
	 */

	int			pid,
				w;

	void			(*istat)(),
				(*qstat)();


	if ((pid = fork()) == 0) {
		if (fdout != 1) {
			close (1);
			dup (fdout);	/* should dup to 1 */
			close (fdout);
		}
		close (0);
		close (2);
		execl ("/bin/sh", "sh", "-c", command, 0);
		_exit (127);
        }
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while ((w = wait((int *)0)) != pid && w != -1)
		;
	signal (SIGINT, istat);
	signal (SIGQUIT, qstat);
	return;
}

/**
 ** my_signal()
 **/

static void		my_signal (sig, func)
	int			sig;
	void			(*func)();
{
	if (signal(sig, SIG_IGN) != SIG_IGN)
		signal (sig, func);
	return;
}
