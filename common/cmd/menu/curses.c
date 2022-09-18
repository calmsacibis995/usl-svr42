/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)menu.cmd:curses.c	1.2"
#ident	"$Header: $"

#include "menu.h"
#include <sys/termios.h>

static	void go_away();
static	void set_color();

static	struct keywords errors;		/* Hold error messages */

extern char	*output_file;

void
abort()
{
	clear();
	refresh();
	end_curses(0);
	exit(66);
}

void
go_away(signo)
int	signo;
{
	abort();
}

void
resize(signo)
int	signo;
{
	struct	winsize	winsize;
	char	buf[80];

	if (!isatty(1)) {
		fprintf(stderr, "Not running on a tty\n");
		exit(ENOTTY);
	}

	if (ioctl(1, TIOCGWINSZ, &winsize) == -1) {
		fprintf(stderr, "Can't handle resize request\n");
		exit(ENOTTY);
	}

	LINES=winsize.ws_row;
	COLS=winsize.ws_col;

	sprintf(buf, "LINES=%d", LINES);
	putenv(buf);
	sprintf(buf, "COLUMNS=%d", COLS);
	putenv(buf);
	
	endwin();
	initscr();
	do_form();
}

void
start_curses () {			/* curses initialization */
	struct keywords *kwp;		/* ptr to keyword for error msgs */
	char *terminal;			/* terminal type */
	char	buf[256];		/* buffer for err filename */
	int	vid_fd;			/* /dev/video file descriptor */

	/*
	 *  First thing we do is check to make sure TERM environment
	 *  variable is set.  If not, force it to "ansi".  If this
	 *  doesn't work correctly, then at least curses will start
	 *  and give the user a chance to Cancel.
	 */
	if ( (terminal = (char *)getenv("TERM")) == NULL ) {
		if ((vid_fd = open("/dev/video", O_RDWR)) < 0) {
			putenv("TERM=ansi");
		} else {
			putenv("TERM=AT386-M");
		}
	}

	def_shell_mode();
	initscr();

	/*
	 *  This looks silly, but togging the cursor guarantees
	 *  it will look like we need it to look.
	 */
	curs_set(1);
	refresh();
	curs_set(0);
	refresh();
	curs_set(1);
	refresh();
	flushinp();

	w1 = newwin(LINES*3, COLS, 0, 0);
	(void)sigset(SIGTERM, go_away);
	(void)sigset(SIGINT, abort);

	keypad(w1, TRUE);
	nonl();
	cbreak();
	noecho();  
	set_color();
	wbkgdset(w1, ' ' | regular_attr);
	
	/*
	 *  Now suck in the displayable error messages (do it here
	 *  to keep error strings local, as this is the only place
	 *  they're used...
	 */
	locale_ify(ERROR_FMT, buf);
	kwp = &errors;
	io_redir(buf, kwp);

	wclear(w1);
}

void
end_curses(rflag)
int	rflag;			/* retain this screen? */
{
	char	spaces[132];
	int	where;

	
	if (!rflag) {
		wbkgdset(w1, ' ' | 0);
		wclear(w1);
		wrefresh(w1);

		/*
		 *  Make sure cursor is left at the TOP of the screen.
		 */
		endwin();
		initscr();

		move(0,0);
		refresh();
		curs_set(0);
		refresh();
		curs_set(1);
		refresh();
		reset_shell_mode();

	} else {
		wattron(w1, regular_attr);
		if (keywords[KW_LR].buffer) {	/* nullptr */
			where = COLS-2-strlen(keywords[KW_LR].buffer);
			mvwaddch(w1, LINES-3, where-2, ACS_HLINE);
			mvwaddch(w1, LINES-1, where-2, ACS_HLINE);
		}

		wattron(w1, input_attr | A_BLINK);

		memset(spaces, ' ', COLS-2);
		spaces[COLS-2]='\0';

		mvwaddstr(w1, LINES-2, 1, spaces);
		if (keywords[KW_WORKING].buffer) {	/* nullptr */
			mvwaddstr(w1, LINES-2, (COLS - (int)strlen(keywords[KW_WORKING].buffer))/2, keywords[KW_WORKING].buffer);
		}

		wrefresh(w1);
		curs_set(0);
		refresh();
		curs_set(1);
		move(LINES-2, (COLS + (int)strlen(keywords[KW_WORKING].buffer))/2);
		refresh();
		reset_shell_mode();
	}
}

void
set_color() {
	char *c;
	int fore_color, back_color, error_color,help_color, help_fg_color;
	int	reg_fg, reg_bg;		/* colors for regular screen */
	int	help_fg, help_bg;	/* colors for help screen */
	int	error_fg, error_bg;	/* colors for error bars */

	/*
	 *  If we have color on this terminal, then initialize
	 *  the color variables to default values, or to values
	 *  stored in the appropriate environment variables, if
	 *  applicable.
	 */
	if (( start_color()) == OK ) {

		/*
		 *  Plain vanilla default color values:
		 */
		reg_fg = COLOR_WHITE;
		reg_bg = COLOR_BLUE;
		help_fg = COLOR_BLACK;
		help_bg = COLOR_CYAN;
		error_fg = COLOR_WHITE;
		error_bg = COLOR_RED;

		/*
		 *  Backward-compatible color def env variables.
		 */
		if ( (c = (char *)getenv("FORE_COLOR")) != NULL )
			reg_fg = atoi(c);
		if ( (c = (char *)getenv("BACK_COLOR")) != NULL )
			reg_bg = atoi(c);
		if ( (c = (char *)getenv("ERROR_COLOR")) != NULL )
			error_bg = atoi(c);
		if ( (c = (char *)getenv("HELP_COLOR")) != NULL )
			help_bg = atoi(c);
		if ( (c = (char *)getenv("HELP_FG_COLOR")) != NULL )
			help_fg = atoi(c);

		/*
		 *  Color def env variables.  If these vars are set, then
		 *  override default values.
		 */
		if ( (c = (char *)getenv("REG_FG")) != NULL )
			reg_fg = atoi(c);
		if ( (c = (char *)getenv("REG_BG")) != NULL )
			reg_bg = atoi(c);

		if ( (c = (char *)getenv("ERROR_FG")) != NULL )
			error_fg = atoi(c);
		if ( (c = (char *)getenv("ERROR_BG")) != NULL )
			error_bg = atoi(c);

		if ( (c = (char *)getenv("HELP_FG")) != NULL )
			help_fg = atoi(c);
		if ( (c = (char *)getenv("HELP_BG")) != NULL )
			help_bg = atoi(c);
		init_pair(1, reg_fg, reg_bg);
		init_pair(2, error_fg, error_bg);
		init_pair(3, help_fg, help_bg);
		init_pair(4, reg_bg, reg_fg);

		regular_attr = COLOR_PAIR(1);
		help_attr = COLOR_PAIR(3);
		input_attr = COLOR_PAIR(4);
		select_attr = COLOR_PAIR(1) | A_BOLD;
		error_attr = COLOR_PAIR(2);

	} else {
	/*
	 *  If this terminal is monochromatic, then initialize
	 *  the color variables to default attributes, or to values
	 *  stored in the appropriate environment variables, if
	 *  applicable.
	 */
		regular_attr = 0;
		help_attr = 0;
		select_attr = A_BOLD;
		input_attr = A_STANDOUT;
		error_attr = A_STANDOUT;

		/*
		 *  Mono attr env variables.  If these vars are set, then
		 *  override default values.
		 */
		if ( (c = (char *)getenv("REG_ATTR")) != NULL )
			regular_attr = atoi(c);
		if ( (c = (char *)getenv("HELP_ATTR")) != NULL )
			help_attr = atoi(c);
		if ( (c = (char *)getenv("ERROR_ATTR")) != NULL )
			error_attr = atoi(c);

		/* in the case of mono, warning screens may cause
		 * the regular attr to go to A_STANDOUT. Note that
		 * input_attr will never initially be 0 since
		 * we set it to A_STANDOUT above, and it can't be
		 * overridden in menu_colors.sh. Therefore, if
		 * regular_attr == input_attr (and regular_attr
		 * is non-zero), set input_attr to 0.
		 */
		if (( regular_attr == input_attr ) && regular_attr )
			input_attr = 0;
	}


}

/*
 *  Draw screen base, the boxes and labels for the corners
 */
void
draw_bg()
{
	int	where;
	int	attribute;	/*  display attribute */
	char	spaces[128];
	struct	keywords *kwp;
	int	c_rows; 	/* bunch of placeholders for field info. */
	int	c_cols; 	/* bunch of placeholders for field info. */
	int	c_frow; 	/* bunch of placeholders for field info. */
	int	c_fcol; 	/* bunch of placeholders for field info. */
	int	c_nrow; 	/* bunch of placeholders for field info. */
	int	c_nbuf; 	/* bunch of placeholders for field info. */
	int page = form_page(form);

	wattroff(w1, A_BOLD);
	wattroff(w1, A_STANDOUT);
	wattroff(w1, A_REVERSE);
	if (page == 0)
		attribute = regular_attr;
	else
		attribute = help_attr;

	(void)memset(spaces, ' ', COLS-2);
	spaces[COLS-2] = '\0';

	/*
	 *  Display the help bar
	 */
	wattron(w1,help_attr);
	mvwaddstr(w1, LINES-2, 1, spaces);
	switch (page) {
		case 0:	/* main menu screen */
			if (field_opts(current_field(form)) & (O_ACTIVE | O_EDIT))
				if (field_label[field_index(current_field(form))]) 	/* nullptr */
					mvwaddstr(w1, LINES-2, 1, field_label[field_index(current_field(form))]);  /*  MARGIN  */
			else
				if (keywords[KW_LL].buffer) {	/* nullptr */
					mvwprintw(w1, LINES-2, 2, "%s", keywords[KW_LL].buffer);
				}

			if (keywords[KW_LR].buffer) {	/* nullptr */
				where = COLS-2-strlen(keywords[KW_LR].buffer);
				mvwprintw(w1, LINES-2, where, "%s", keywords[KW_LR].buffer);
			}
			break;
		case 1:	/* help screen */
			if (keywords[KW_HELPINST].buffer) {	/* nullptr */
				where = COLS-2-strlen(keywords[KW_HELPINST].buffer);
				mvwprintw(w1, LINES-2, where, "%s", keywords[KW_HELPINST].buffer);
			}
			break;
		case 2:	/* help on help screen */

			/*
			 *  If there is a .hhelpinst section in the form
			 *  description file, use that, otherwise use the 
			 *  .helpinst stuff.
			 */
			if (keywords[KW_HHELPINST].buffer != NULL) {
				where = COLS-2-strlen(keywords[KW_HHELPINST].buffer);
				mvwprintw(w1, LINES-2, where, "%s", keywords[KW_HHELPINST].buffer);
			} else {
				if (keywords[KW_HELPINST].buffer) {	/* nullptr */
					where = COLS-2-strlen(keywords[KW_HELPINST].buffer);
					mvwprintw(w1, LINES-2, where, "%s", keywords[KW_HELPINST].buffer);
				}
			}

			break;
	}
	
	/*
	 *  Put upper left and upper right text on screen
	 */
	wattron(w1, attribute);
	mvwaddstr(w1, 1, 1, spaces);

	switch(page) {
	case 0:	/* main body of form */
		if (keywords[KW_UL].buffer) {	/* nullptr */
			mvwprintw(w1, 1, 2, "%s", keywords[KW_UL].buffer);
		}
		if (keywords[KW_UR].buffer) {	/* nullptr */
			mvwprintw(w1, 1, COLS-2-strlen(keywords[KW_UR].buffer), "%s", keywords[KW_UR].buffer);
		}
		break;
	case 1:	/* Help for form */
		if (keywords[KW_HELPBANNER].buffer) {	/* nullptr */
			mvwprintw(w1, 1, 2, "%s", keywords[KW_HELPBANNER].buffer);
		}
		if (keywords[KW_PAGENO].buffer) {	/* nullptr */
			(void)sprintf(spaces, keywords[KW_PAGENO].buffer, cur_pg, tot_pg);
		}
		mvwprintw(w1, 1, COLS-2-strlen(spaces), "%s", spaces);
		break;
	case 2:	/* Help for menu tool */
		if (keywords[KW_HHELP_BAN].buffer) {	/* nullptr */
			mvwprintw(w1, 1, 2, "%s", keywords[KW_HHELP_BAN].buffer);
		}
		if (keywords[KW_PAGENO].buffer) {	/* nullptr */
			(void)sprintf(spaces, keywords[KW_PAGENO].buffer, cur_pg, tot_pg);
		}
		mvwprintw(w1, 1, COLS-2-strlen(spaces), "%s", spaces);
		break;
	}

	/*
	 *  Draw the box around everything
	 */
	wattron(w1, attribute);
	wmove(w1, 2, 1);
	whline(w1, ACS_HLINE, COLS-2);
	wmove(w1, LINES - 3, 1);
	whline(w1, ACS_HLINE, COLS-2);

	/*
	 *  Window is larger than stdscr, so draw box, then draw
	 *  bottom of box manually.
	 */
	box(w1, 0, 0);
	wmove(w1, LINES-1, 1);
	whline(w1, ACS_HLINE, COLS-2);
	mvwaddch(w1, LINES-1, 0, ACS_LLCORNER);
	mvwaddch(w1, LINES-1, COLS-1, ACS_LRCORNER);
	mvwaddch(w1, LINES-3, where-2, ACS_TTEE);
	mvwaddch(w1, LINES-2, where-2, ACS_VLINE);
	mvwaddch(w1, LINES-1, where-2, ACS_BTEE);

	/*
	 *  Next, draw a little box around each of the radio buttons
	 */
	if (formtype == TYPE_FORM && page == 0) {
		field_info(fields[last_field-2], &c_rows, &c_cols, &c_frow, &c_fcol, &c_nrow, &c_nbuf);

		mvwaddch(w1, c_frow, COLS/2 - c_cols - 6, ACS_VLINE);
		mvwaddch(w1, c_frow-1, COLS/2 - c_cols - 6, ACS_ULCORNER);
		wmove(w1, c_frow-1, COLS/2 - c_cols - 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow-1, COLS/2 - 5, ACS_URCORNER);
		mvwaddch(w1, c_frow, COLS/2 - 5, ACS_VLINE);

		mvwaddch(w1, c_frow+1, COLS/2 - c_cols - 6, ACS_LLCORNER);
		wmove(w1, c_frow+1, COLS/2 - c_cols - 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow+1, COLS/2 - 5, ACS_LRCORNER);

		field_info(fields[last_field-1], &c_rows, &c_cols, &c_frow, &c_fcol, &c_nrow, &c_nbuf);

		mvwaddch(w1, c_frow, COLS/2 + 4, ACS_VLINE);
		mvwaddch(w1, c_frow-1, COLS/2 + 4, ACS_ULCORNER);
		wmove(w1, c_frow-1, COLS/2 + 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow-1, COLS/2 + 5 + c_cols, ACS_URCORNER);
		mvwaddch(w1, c_frow, COLS/2 + 5 + c_cols, ACS_VLINE);
	
		mvwaddch(w1, c_frow+1, COLS/2 + 4, ACS_LLCORNER);
		wmove(w1, c_frow+1, COLS/2 + 5);
		whline(w1, 0, c_cols);
		mvwaddch(w1, c_frow+1, COLS/2 + 5 + c_cols, ACS_LRCORNER);
	}

}

/* 
 *  Display error string in error_attr (white on red or inverse) at
 *  bottom of screen.  This gets erased after the next keystroke.
 */
void
put_err(which)
int	which;			/* What error will we display? */
{
	int	i;		/* for counting */
	char spaces[128];	/* Line of spaces to place error string in */
	char *error_string;	/* where in liked list is the right error? */
	struct keywords *kwp;	/* for traversing error list */

	kwp = &errors;

	for (i=0; i<which; i++) {
		error_string = kwp->buffer;
		kwp = kwp->next;
		if (kwp->next == NULL)
			break;
	}

	(void)memset(spaces, ' ', COLS-2);
	spaces[COLS-2] = '\0';

	wattron(w1, error_attr);
	mvwaddstr(w1, LINES-3, 1, spaces);
	mvwaddstr(w1, LINES-3, (COLS - (int)strlen(error_string))/2, error_string);

	if (form_page(form) == 0)
		wattron(w1, regular_attr);
	else
		wattron(w1, help_attr);

	error_displayed = TRUE;

	if (form)
		if ((keywords[KW_FORM].buffer != NULL)&&(form_page(form)==0))
			pos_form_cursor(form);
		else
			wmove(w1, LINES-4, COLS-2);

	beep();
	wrefresh(w1);
}
