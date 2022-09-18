/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)menu.cmd:form.c	1.5"
#ident	"$Header: $"

#include "menu.h"
#define QUIT	(MAX_COMMAND + 1)
#define DEL_PRESSED	(MAX_COMMAND + 2)
#define ONEPAGE	(LINES - 6)
#define DELIM 50
#define	APPLY keywords[KW_BUTTONS].buffer
#define	RESET keywords[KW_BUTTONS].next->buffer
#define	DFLT_APPLY "APPLY"	/* In case anybody forgot these */
#define	DFLT_RESET "RESET"	/* In case anybody forgot these */

static	int	count_lines();
static	FIELD * make_label();
static	int my_form_driver();
static	int get_request();
static	FIELD * fill_help_field();
static	int create_menu();
static	int create_form();
static	void adjust_fields();
static	char * field_env_vble();
static	void write_output();
static	void reset_form();

PTF_void after_field();		/* post validation processing */
static	FIELD	*holdfld;	/* hold our place while in help. */
int	null_validation=TRUE;	/* do we check for NULL field? */
int	redo_field=FALSE;	/* did we get an err in after_field()? */

static  char	*field_vble[50];

static	int	num_scr_dump;
static	void	secret_screen_dump();

extern	char	*menu_file; /* form description file */

char	**choices[50];		/* Array of strings for ENUM_TYPE */
char	**tags[50];			/* Array of strings for ENUM_TYPE */

int	form_ret;		/* return value from form driver */

int
do_form(output_file)
char *output_file;		/* Name of file we place out results in */
{
	int	rc;			/* for holding return values */
	int	fpage;			/* current pg of form (help or no) */
	int	done = 0;		/* are we finished yet? */
	int	row, col;		/* row, colunm placeholders */
	int	num_lines;		/* number of lines in section */
	int	f_num = 0;		/* which field number is this */
	int	indx;			/* index for field number */
	char	*cp;			/* tmp pointer for char */
	struct	keywords *kwp;		/* tmp ptr to manage keyword list */
	FIELD	*fld_ptr;		/* tmp ptr to a field */
	char	buf[80];		/* tmp buffer for strings */


	/*
	 *  Set up the screen for a regular screen.
	 */
	wbkgdset(w1, ' ' | regular_attr);
	wattron(w1, regular_attr);
	wmove(w1, 0, 0);
	wclrtobot(w1);
	col = 2;  /* MARGIN */
	row = 3;

	/*
	 *  Make label fields out of the .top section of the form
	 *  description file.
	 */
	kwp = &keywords[KW_TOP];
	if ((kwp->buffer) && (*(kwp->buffer) != NULL)) /* nullptr */
		fields[f_num++] = make_label(&row, col, kwp->buffer);
	while(kwp->next != NULL) {
		kwp = kwp->next;
		if ((kwp->buffer) && (*(kwp->buffer) != NULL)) /* nullptr */
			fields[f_num++] = make_label(&row, col, kwp->buffer);
	}

	row++;

	/*
	 *  Create the form or menu.  Determine which and do the right one.
	 */
	kwp = &keywords[KW_FORM];

	/*
	 *  Skip any null/blank lines.
	 */
	while ((kwp->buffer && (*(kwp->buffer)==NULL)) && (kwp->next != NULL)) { /* nullptr */
		kwp = kwp->next;
		keywords[KW_FORM].buffer = kwp->buffer;
		keywords[KW_FORM].next = kwp->next;
	}

	/*
	 *  Which type of form is this?  Menu, form or ENTER?
	 */
	if ((kwp->buffer) && (*(kwp->buffer) != NULL)) { /* nullptr */

		/*
		 *  If there's a // delimiter, it's a form
		 */
		if ( ((cp = (char *)strchr(kwp->buffer, '/')) != NULL)
		&& (*(cp+1) == '/') )
			f_num = create_form(f_num, kwp, row);
		else
			f_num = create_menu(f_num, kwp, LINES - 4 - count_lines(KW_BOTTOM));
	} else
		/*
	 	 *  If empty or missing .form section, its an ENTER screen.
		 */
		formtype = TYPE_ENTER;

	/*
	 *  Make label fields out of the .bottom section of the form
	 *  description file.
	 */
	num_lines = count_lines(KW_BOTTOM);
	kwp = &keywords[KW_BOTTOM];
	row = LINES - 3 - num_lines;
	if ((kwp->buffer) && (*(kwp->buffer) != NULL)) /* nullptr */
		fields[f_num++] = make_label(&row, col, kwp->buffer);
	while(kwp->next != NULL) {
		kwp = kwp->next;
		if ((kwp->buffer) && (*(kwp->buffer) != NULL)) /* nullptr */
			fields[f_num++] = make_label(&row, col, kwp->buffer);
	}

	/*
	 *  Populate the help pages and help-within-help pages.
	 */
	fields[f_num++] = fill_help_field(KW_HELP);
	fields[f_num++] = fill_help_field(KW_HELPHELP);

	fields[f_num] = (FIELD *)0;

	/*
	 *  Create the new form
	 */
	form = new_form(fields);
	scale_form(form, &row, &col);
	set_form_win(form, w1);

	/*
	 *  Set processing for after field validation.  after_field()
	 *  is where we check for null fields.  Letting ETI check
	 *  in its own validation routines by turning off NULLOK and
	 *  PASSOK for fields does not let user get help if needed.
	 */
	set_field_term(form, after_field);

	post_form(form);
	draw_bg();

	/*
	 *  If this is a form or menu with input, position the cursor
	 *  at the appropriate field, otherwise cursor belongs in 
	 *  the lower right-hand corner.
	 */
	if ((keywords[KW_FORM].buffer != NULL)&&(form_page(form)==0))
		pos_form_cursor(form);
	else
		wmove(w1, LINES-4, COLS-2);

	wrefresh(w1);

	/*
	 *  If -r and no form, go right to Working... flash
	 */
	if ((keywords[KW_FORM].buffer == NULL)&&(rflag))
		done = TRUE;

	/*
	 *  Process user's keystrokes
	 */
	while (!done) {
		/*
		 *  get_request() converts keystrokes into ETI requests
		 */
		form_ret = get_request(w1);

		/*
		 *  menu tool's secret power - pressing CTRL-O dumps ASCII
		 *  version of screen to /tmp
		 */
		if (form_ret == 0x0f) {
			secret_screen_dump();
			continue;
		}
		error_displayed = FALSE;

		/*
		 *  Just to make sure any error attribute is off...
		 */
		fpage = form_page(form);
		if (fpage)
			wattron(w1, help_attr);
		else
			wattron(w1, regular_attr);
		draw_bg();

		if ((keywords[KW_FORM].buffer != NULL)&&(form_page(form)==0))
			pos_form_cursor(form);
		else
			wmove(w1, LINES-4, COLS-2);
		wrefresh(w1);

		/*
	 	 *  Generally, we want to check for null fields
		 *  in after_field()...
		 */
		null_validation=TRUE;

		/*
	 	 *  Special processing if we're going for help.
		 */
		if (form_ret == REQ_NEXT_PAGE) {
			if (fpage == 2) {
				put_err(ERR_NOHELP);
				continue;
			}
			/*
			 *  ...but not if we're going for help...
			 */  
			null_validation=FALSE;
			cur_pg = 1;
			if (fpage == 1)
				tot_pg = ((LINES-6)+count_lines(KW_HELPHELP))/
					(LINES-5);
			if (fpage == 0) {
				/*
				 *  Going for help?  Save current field!
				 */
				holdfld = current_field(form);  /* jay */
				tot_pg = ((LINES-6)+count_lines(KW_HELP))/
					(LINES-5);
			}
		} else
		if (form_ret == REQ_PREV_PAGE) {
			if (fpage == 0) {
				put_err(ERR_BADKEY);
				continue;
			}
			null_validation=FALSE;
			cur_pg = 1;
			if (fpage == 2)
				tot_pg = ((LINES-6)+count_lines(KW_HELP))/
					(LINES-5);
		}

		/*
		 *  Manage the page numbering
	 	 */
		if (fpage) {
			if (form_ret ==	REQ_SCR_FPAGE)
				if (cur_pg < tot_pg)
					cur_pg++;
				else {
					put_err(ERR_LASTPG);
					continue;
				}
			if (form_ret ==	REQ_SCR_BPAGE)
				if (cur_pg >= 2)
					cur_pg--;
				else {
					put_err(ERR_FIRSTPG);
					continue;
				}
		}

		/*
		 *  If it's a menu, we think we're ready to leave.
		 *  validate the field for good measure.
	 	 */
		if ((formtype == TYPE_MENU) && (form_ret == QUIT)) { 

			/*
			 *  If this field wasn't valid, give that error
			 */
			if (form_driver(form, REQ_NEXT_FIELD) == E_INVALID_FIELD) {
				put_err(ERR_BADDATA);
				done=FALSE;
				continue;
			} else {
				/*
				 *  Otherwise, error from after_field()
				 */
				if (redo_field) {
					done=FALSE;
					continue;
				}
				/*
				 *  And if this field is OK, we can leave.
				 */
				done=TRUE;
			}
		}

		/*
		 *  form_driver() is the ETI engine that makes use
		 *  of the ETI requests returned from get_request().
		 */
		switch (rc = form_driver(form, form_ret)) {
			case E_OK:
				/*
				 *  If returning from help, restore current
				 *  field to the one we saved above.
				 */
				if (form_ret == REQ_PREV_PAGE && fpage == 1)
					set_current_field(form, holdfld);
				break;
			case E_INVALID_FIELD:
				done = FALSE;
				put_err(ERR_BADDATA);
				break;
			case E_UNKNOWN_COMMAND:
			default:
				done = my_form_driver(form, form_ret);
				break;
		}

		/*
		 *  If we got here after after_field(), and there was an
		 *  error (like a null field or incomplete form), go back
		 *  to the bad field and redo it.
		 */
		if (redo_field) {
			/*
			 *  If we came down to here, go back up.
			 */
			if ((form_ret == REQ_NEXT_FIELD) || (form_ret == REQ_NEW_LINE))
				form_driver(form, REQ_PREV_FIELD);
			/*
			 *  If we came up to here, go back down.
			 */
			if (form_ret == REQ_PREV_FIELD)
				form_driver(form, REQ_NEXT_FIELD);
			redo_field = FALSE;
		}


		/*
		 *  If we are deleting a character, make sure there's
		 *  no "widow" under the cursor.
		 */
		if ((form_ret == REQ_DEL_CHAR)) {
			form_driver(form, REQ_PREV_CHAR); 
			form_driver(form, REQ_CLR_EOL); 
		}

		if (!error_displayed) {
			draw_bg();
			/*
			 *  If this is a form or menu with input, position the
			 *  cursor at the appropriate field, otherwise cursor 
			 *  belongs in the lower right-hand corner.
			 */
			if ((keywords[KW_FORM].buffer != NULL)&&(form_page(form)==0))
				pos_form_cursor(form);
			else
				wmove(w1, LINES-4, COLS-2);
			wrefresh(w1);
		}

		/* 
		 *  If we're on a radio button (ie: Apply, Reset) then
		 *  highlight that button.
		 */
		indx = field_index(current_field(form));

		set_field_fore(fields[last_field-1], regular_attr);
		set_field_back(fields[last_field-1], regular_attr);
		set_field_fore(fields[last_field-2], regular_attr);
		set_field_back(fields[last_field-2], regular_attr);
		wrefresh(w1);

		/*
		 *  On a radio button - turn off cursor & hilight text
		 */
		if (indx==(last_field-1)) {
			curs_set(0);
			set_field_back(fields[indx], input_attr);
			set_field_fore(fields[indx], input_attr);
			wrefresh(w1);
		} 

		/*
		 *  On a radio button - turn off cursor & hilight text
		 */
		if (indx==(last_field-2)) {
			curs_set(0);
			set_field_back(fields[indx], input_attr);
			set_field_fore(fields[indx], input_attr);
			wrefresh(w1);
		}

		/*
		 *  Not on a radio button - turn on cursor
		 */
		if (indx < (last_field-2))
			curs_set(1);

	}

	/*
	 *  Once form processing is complete, write the output file.
	 */
	write_output(output_file);
	return(0);
}

/*
 *  Extra field validation we do after ETI is done.  This is a less
 *  dictatorial check for a null field than turning off NULLOK and PASSOK
 *  and letting ETI do the work.  It allows us to control whether or not
 *  we can leave the field, for example, to get help.
 */
PTF_void
after_field()
{
	char	spaces[64];	/* spaces for checking field contents */

	memset(spaces, ' ', 64);

	/*
	 *  If we're not checking (like for a help request), go away.
	 *  If it wasn't necessary to check this (i.e., we did not set
	 *  buffer 2 of the current field to "Y"), go away.
	 */
	if (null_validation==FALSE)
		return;
	if ( strncmp( field_buffer(current_field(form), 2), "Y", 1) )
		return;

	/*
	 *  It is a NULL field?
	 */
	if ( !strncmp( field_buffer(current_field(form), 0), spaces, strlen(field_buffer(current_field(form), 0))) ) {
		redo_field=TRUE;
		if (form_ret == QUIT)
			put_err(ERR_INCFORM);
		else
			put_err(ERR_NULLFLD);
	}
}

/*
 *  Count the number of lines in a .keyword section
 */
int
count_lines(keyword)	
int	keyword;			/* Which .keyword are we checking? */
{
	register struct keywords*kwp;	/* tmp ptr to manage keyword list */
	register int	line_count = 0;	/* Number of lines in this list */

	kwp = &keywords[keyword];
	while(kwp->next != NULL) {
		kwp = kwp->next;
		line_count++;
	}
	return(line_count);
}

/*
 *  Make a 1-row label.  This will be non-active, non-editable, and in the
 *  regular display attribute.
 */
FIELD *
make_label(frow, fcol, label)
int	*frow;		/*  first row  */
int	fcol;		/*  first column  */
char	*label;		/*  label to put in form */
{
	FIELD	*f;

	f= new_field(1, strlen(label), *frow, fcol, 0, 1);
	if (f) {
		set_field_buffer(f, 0, label);
		set_field_buffer(f, 1, label);
		set_field_back(f, regular_attr);
		set_field_fore(f, regular_attr);
		field_opts_off(f, (OPTIONS)O_ACTIVE);
		field_opts_off(f, (OPTIONS)O_EDIT);
	}
	(*frow)++;
	return f;
}

/*
 *  Local processing
 */
int
my_form_driver(my_form, form_ret)
FORM *my_form;
int	form_ret;
{
	int	f_cnt;		/* for counting current field */
	int	maxfld;		/* the max field were going to check */

	if (form_ret == QUIT) {

		/*
	 	 *  If its a form, we only check fields before the radio
	 	 *  buttons.  Otherwise, look at them all.
		 */
		if (formtype == TYPE_FORM)
			maxfld = last_field-2;
		else
			maxfld = field_count(form);

		/*
		 *  We think we're leaving the form.  Let's make sure
		 *  it's really complete before we go...
		 *  Run through every field we ever allocated for this form,
		 *  even though some are just labels.  See next comment
		 */
		for (f_cnt=0; f_cnt<maxfld; f_cnt++) {

			/*
			 *  If this was an ACTIVE, EDITable field, then
			 *  validate this field..
			 */  
			if (field_opts(fields[f_cnt]) & (O_ACTIVE | O_EDIT)) {
				set_current_field(my_form, fields[f_cnt]);
				if (form_driver(my_form, REQ_VALIDATION) == E_OK) {
					/*
					 *  If we uncovered a problem in
					 *  after_field(), back up to the
					 *  problem field so we can redo it
					 *  in the main loop of do_form().
					 */
					if (redo_field) {
						form_driver(form, REQ_PREV_FIELD);
						break;
					}
					continue;
				} else {
					/*
					 *  If the form is incomplete, we put
					 *  that error up and break, which
					 *  leaves the user on the field that
					 *  needs filling in!
					 */
					if (formtype == TYPE_FORM)
						put_err(ERR_INCFORM);
				}
				return FALSE;
			}
		}
	/*
	 *  If all went well, set error_displayed to TRUE to keep draw_bg()
	 *  from being called when we return.  We don't want an incorrect
	 *  help bar given to us because of the last set_current_field we
	 *  did while checking validity of fields.
	 */
	error_displayed = TRUE;
	} else {
		/*
		 *  If the DELETE key was pressed, and we didn't get a 
		 *  SIGINTR, we assume that Cancel is disabled here.
		 */
		if (form_ret == DEL_PRESSED) {
			put_err(ERR_NOCANCEL);
			return FALSE;
		} else {
			put_err(ERR_BADKEY);
			return FALSE;
		}
	}
}

/*
 *  Get keystrokes and translate them into ETI requests.
 */
int
get_request(w)
WINDOW	*w;
{
	int		c = wgetch(w);
	int		indx;			/* index of current field */

	/*
	 *  Specific processing for help screens.
	 */
	if (form_page(form) > 0) {
		switch(c) {
			case 0x7f:			/* DELETE */
				return DEL_PRESSED;
			case	'1':
			case	KEY_NPAGE:		/* PageDown */
				return REQ_SCR_FPAGE;
			case	'2':
			case	KEY_PPAGE:		/* PageUp */
				return REQ_SCR_BPAGE;
			case	0x1b:			/* Escape key */
				return REQ_PREV_PAGE;
			case	'?':
			case	KEY_F(1):		/* 'F1' key */
				return REQ_NEXT_PAGE;
			default:	
				return c;
		}
	}

	/*
	 *  If we have a null form, enter/return will handle it.
	 */
	if (keywords[KW_FORM].buffer == NULL)
		if (c == 0x0d || c == 0x0a)
			return QUIT;

	/*
	 *  Of this is an ENUM field, then these keys select next and
	 *  previous choices.
	 */
	if (field_type(current_field(form)) == TYPE_ENUM) {
                if (c == '+' || c == KEY_RIGHT)
                        return REQ_NEXT_CHOICE;
                if (c == '-' || c == KEY_LEFT)
                        return REQ_PREV_CHOICE;
        }

	/*
	 *  If we're on the last field of the form, if the buffer
	 *  contains the string we use to exit, then return QUIT,
	 *  otherwise return NEXT_FIELD, so we stay on the form.
	 */
	indx = field_index(current_field(form));
	if (indx==(last_field-1) || indx==(last_field-2))
                if ( (c == '\n') || (c == '\r') ) {

			/*
			 *  APPLY button QUITs this form
			 */
			if ( strncmp(field_buffer(current_field(form), 0), APPLY, strlen(APPLY) ) == 0)
				return QUIT;

			/*
			 *  RESET button puts starting values back in fields
			 */
			if ( strncmp(field_buffer(current_field(form), 0), RESET, strlen(RESET) ) == 0)
				reset_form();
				return REQ_NEXT_FIELD;
		} else
			switch (c)
			{
				case 0x7f:			/* DELETE */
					return DEL_PRESSED;
				case '?':
				case KEY_F(1):			/* 'F1' key */
				case 0x06:			/* ctl-F */
					return REQ_NEXT_PAGE;
				case KEY_HOME:			/* Home key */
					return REQ_FIRST_FIELD;
				case KEY_LL:			/* End key */
					return REQ_LAST_FIELD;
				case 0x09:			/* Tab key */
				case KEY_DOWN:			/* Curs Down */
					return REQ_NEXT_FIELD;
				case KEY_BTAB:			/* backtab */
				case KEY_UP:			/* Curs Up */
					return REQ_PREV_FIELD;
				case 0x0a:			/* Enter key */
				case 0x0d:			/* Return key */
					return REQ_NEW_LINE;
				case 0x08:			/* Backspace */
					return REQ_DEL_CHAR;
				/*
				 *  If keystroke does not map to an ETI
				 *  request, return the keystroke so that it
				 *  can be processed by the form_driver()
				 *  (ETI decides if its valid or in error)
				 */
				default:
					return c;
			}

	/*
	 *  If this is a numberes menu, there is ony one input field,
	 *  so we know we're done when a value is entered.
	 */
	if ((formtype == TYPE_MENU) && ((c == 0x0d)||(c == 0x0a)) )
		return QUIT;

	switch (c) {
		case 0x7f:			/* DELETE */
			return DEL_PRESSED;
		case '?':
		case KEY_F(1):			/* 'F1' key */
		case 0x06:			/* ctl-F */
			return REQ_NEXT_PAGE;
		case KEY_HOME:			/* Home key */
			return REQ_FIRST_FIELD;
		case KEY_LL:			/* End key */
			return REQ_LAST_FIELD;
		case 0x09:			/* Tab key */
		case KEY_DOWN:			/* Curs Down */
			return REQ_NEXT_FIELD;
		case KEY_BTAB:			/* backtab */
		case KEY_UP:			/* Curs Up */
			return REQ_PREV_FIELD;
		case 0x0a:			/* Enter key */
		case 0x0d:			/* Return key */
			return REQ_NEW_LINE;
		case 0x08:			/* Backspace */
			return REQ_DEL_CHAR;
		/*
		 *  If keystroke does not map to an ETI
		 *  request, return the keystroke so that it
		 *  can be processed by the form_driver()
		 *  (ETI decides if its valid or in error)
		 */
		default:
			return c;
	}
}

/*
 *  Populate the help field; used for help and help-within-help screens.
 */
FIELD *
fill_help_field(keyword)
int	keyword;
{
	FIELD	*f;			/* What field we're working with */
	struct keywords	*kwp;		/* tmp ptr to manage keyword list */
	int	num_lines;		/* Number of lines in help */
	int	row, col;		/* What row and column to display at */
	int	rc;			/* Return code for commands */
	int	offscreen;		/* How much data is kept offscreen */
	char	*cp;	 		/* Pointer for working w/help text */
	char	*help_buf;		/* Array for working w/help text */

	num_lines = count_lines(keyword);

	/* 
	 *  Make sure that we have at least 1 page to display.
	 */
	if (num_lines < ONEPAGE)
		num_lines = ONEPAGE;

	/* 
	 *  Make sure that the field ends on a page boundary so
	 *  we scroll nicely.
	 */
	while (num_lines%ONEPAGE)
		num_lines++;


	help_buf = (char *)malloc((num_lines)*80 + 1);
	if (help_buf == NULL) {
		(void)fprintf(stderr, "Couldn't malloc help buffer\n");
		curs_set(0);
		refresh();
		curs_set(1);
		refresh();
		reset_shell_mode();
		exit(ENOMEM);
	}
	(void)memset(help_buf, ' ', (num_lines)*80);
	*(help_buf+(num_lines)*80+1) = '\0';

	/*
	 *  Fill help buffer with information from help section
	 */
	kwp = &keywords[keyword];
	row = 3;
	col = 1; /* MARGIN */
	cp = help_buf;
	if ( kwp->buffer != NULL ) { /* nullptr */
		rc = strlen(kwp->buffer);
		if (rc>=1) {
			*cp=' ';
			*(cp+1)='\0';
			(void)strncpy(cp+1, kwp->buffer, rc);
		}
		cp += (COLS-2);	/* MARGIN */
	}
	
	while(kwp->next != NULL) {
		kwp = kwp->next;
		if ( kwp->buffer != NULL ) { /* nullptr */
			rc = strlen(kwp->buffer);
			if (rc>=1) {
				*cp=' ';
				*(cp+1)='\0';
				(void)strncpy(cp+1, kwp->buffer, rc);
			}
			cp += (COLS-2);  /* MARGIN */
		}
	
	}

	offscreen = num_lines - (ONEPAGE);
	if (offscreen < 0)
		offscreen = 0;

	if (f = new_field(ONEPAGE, COLS-2, row, col, offscreen, 1)) { /* MARGIN */
		rc =set_field_buffer(f, 0, help_buf);
		rc =set_field_back(f, help_attr);
		rc =set_field_fore(f, help_attr);
		rc =set_new_page(f, TRUE);
		rc =field_opts_off(f, (OPTIONS)O_ACTIVE);
		rc =field_opts_off(f, (OPTIONS)O_EDIT);
		free(help_buf);
	}

	return(f);
}

/*
 *  Actually assemble the menu and all associated data (environment vars, etc)
 */
int
create_menu(f_num, kwp, row)
int	f_num;		/* What field number are we at? */
struct keywords *kwp;	/* tmp ptr to manage keyword list */
int	row;		/* What row are we starting at? */
{
	char	buf[80];		/* Tmp buffer to play w/strings */
	int	num_items=0;		/* How many items in Menu? */
	int	col;			/* What column to display at */
	int	num_lines;

	/*
	 *  We're starting menu at num_items + labels rows up from .bottom
	 */
	row -= (4 + count_lines(KW_FORM));
	col = 2;  /* MARGIN */

	if (keywords[KW_OPTSTRING].buffer && (keywords[KW_OPTSTRING].buffer != NULL)) {
		fields[f_num++] = make_label(&row, col, keywords[KW_OPTSTRING].buffer);
	}
	
	row++;
	col++;

	formtype = TYPE_MENU;
	if ((kwp->buffer) && (*(kwp->buffer) != NULL)) { /* nullptr */
		num_items++;
		if (!strcmp(kwp->buffer, "ENTER")) {
			num_items = 0;
			(void)sprintf(buf, " ");
		} else
			(void)sprintf(buf, "%d. %s", num_items, kwp->buffer);
		fields[f_num++] = make_label(&row, col, buf);
	}
	while(kwp->next != NULL) {
		kwp = kwp->next;
		if ((kwp->buffer) && (*(kwp->buffer) != NULL)) { /* nullptr */
			num_items++;
			(void)sprintf(buf, "%d. %s", num_items, kwp->buffer);
			fields[f_num++] = make_label(&row, col, buf);
		}
	}

	row++;
	col--;

	if (keywords[KW_SELECTION].buffer != NULL) {
		(void)sprintf(buf, keywords[KW_SELECTION].buffer, 1, num_items);
		fields[f_num++] = make_label(&row, col, buf);
	}

	/*
	 *  Determine the column for placing the input field.
	 */
	col = strlen(buf)+3;  /* MARGIN */
	row--;

	/*
	 *  Display menu if this is not an ENTER form
	 */
	if (num_items != 0) {
		fields[f_num] = new_field(1, 5, row, col, 0, 2);
		
		if (fields[f_num]) {
			set_field_back(fields[f_num], input_attr);
			set_field_fore(fields[f_num], input_attr);
			set_field_type(fields[f_num], TYPE_INTEGER, 0, 1, num_items);
			/*
			 *  Setting buffer 2 to "Y" means "Y"es, check this
			 *  field for being null in after_field().
			 */
			set_field_buffer(fields[f_num], 2, "Y");
		}

		if (field_env_vble(f_num, fields[f_num], "RETURN_VALUE") == NULL) {
			set_field_buffer(fields[f_num], 0, "1");
			set_field_buffer(fields[f_num], 1, "1");
		}
	} else {
		/*
		 *  This is a "Press ENTER to continue" menu.
		 */
		fields[f_num] = new_field(1, 2, row, col, 0, 2);
		
		if (fields[f_num]) {
			set_field_back(fields[f_num], regular_attr);
			set_field_fore(fields[f_num], regular_attr);
			field_opts_off(fields[f_num], (OPTIONS)O_EDIT);
			set_field_type(fields[f_num], TYPE_ALNUM, 0, 1, num_items);
		}

		if (field_env_vble(f_num, fields[f_num], "RETURN_VALUE") == NULL) {
			set_field_buffer(fields[f_num], 0, " ");
			set_field_buffer(fields[f_num], 1, " ");
			set_field_buffer(fields[f_num], 2, "N");
		}
	}

	f_num++;
	return(f_num);
}

/*
 *  Actually assemble the form and all associated data (environment vars, etc)
 */
int
create_form(f_num, kwp, row)
int	f_num;		/* What field number are we at? */
struct keywords *kwp;	/* tmp ptr to manage keyword list */
int	row;		/* What row are we starting at? */
{
	struct	keywords *f_kwp, f_kw;	/* Pointers for traversing lists */
	int	i;			/* Counter */
	int	min, max, precision; 	/* Args for making fields */
	int	field_length;		/* Length of a field */
	char	tp;			/* What type of field is this? */
	char	*cp1, *cp2;		/* Tmp ptrs for strstr() */
	int	maxlabel=0;		/* Widest label */

	first_field = f_num;
	f_kwp = &f_kw;

	formtype = TYPE_FORM;

	while (kwp->next != NULL) {


		/*
		 *  Get to the first non-blank line.
		 */
		if ((kwp->buffer != NULL) && (*(kwp->buffer) == NULL)) { /* nullptr */
			kwp = kwp->next;
			continue;
		}



		/*
		 *  Read form description line
		 */
		if ((cp2 = (char *)strstr(kwp->buffer, "//")) != NULL) {
			cp1 = kwp->buffer;
			*cp2 = '\0';
			if (strlen(cp1))
				f_kwp = (struct keywords *)add_a_buffer(cp1, f_kwp);
			cp1 = cp2+2;
			while ((cp2 = (char *)strstr(cp1, "//")) != NULL) {
				*cp2 = '\0';
				f_kwp = (struct keywords *)add_a_buffer(cp1, f_kwp);
				cp1 = cp2+2;
			}
		}
		/*
		 *  Read Label line
		 */
		kwp = kwp->next;
		if ((cp2 = (char *)strstr(kwp->buffer, "//")) != NULL) {
			cp1 = kwp->buffer;
			*cp2 = '\0';
			if (strlen(cp1))
				f_kwp = (struct keywords *)add_a_buffer(cp1, f_kwp);
			cp1 = cp2+2;
			while ((cp2 = (char *)strstr(cp1, "//")) != NULL) {
				*cp2 = '\0';
				f_kwp = (struct keywords *)add_a_buffer(cp1, f_kwp);
				cp1 = cp2+2;
			}
		}
		kwp = kwp->next;
		
		f_kwp = &f_kw;
		switch(*(f_kwp->buffer)) {
		case '0':
		case '1':
			/*
			 *  Get the parms off the line
			 */
			if (sscanf(f_kwp->buffer, "%c %d %d", &tp, &min, &max)<3) {
				(void)fprintf(stderr, "Couldn't parse args\n");
				continue;
			}
			/*
			 *  Create the ALPHA/ALNUM FIELD
			 */
			fields[f_num] = new_field(1, max, row, DELIM, 0, 2);
			if (fields[f_num] == NULL) {
				(void)fprintf(stderr, "couldn't new_field %d\n", f_num);
				continue;
			}

			/*
			 *  Set the ALPHA/ALNUM field up.
			 */
			if (tp == '0')
				set_field_type(fields[f_num], TYPE_ALPHA, min);
			else
				set_field_type(fields[f_num], TYPE_ALNUM, min);
			set_field_fore(fields[f_num], input_attr);
			set_field_back(fields[f_num], input_attr);

			/*
			 *  Setting buffer 2 to "Y" means "Y"es, check this
			 *  field for being null in after_field().
			 */
 			if (min)
				set_field_buffer(fields[f_num], 2, "Y");

			break;
		case '2':
			/*
			 *  Get the parms off the line
			 */
			if (sscanf(f_kwp->buffer, "%c %d", &tp, &max)<2) {
				(void)fprintf(stderr, "Couldn't parse args\n");
				continue;
			}
			/*
			 *  Malloc space to hold the pointers to choice strings
			 *  Also for tags for i18n of choice strings
			 */
			tags[f_num] = (char **)malloc((max+1)*sizeof(char **));
			if (tags[f_num] == NULL) {
				(void)fprintf(stderr, "Couldn't malloc choices\n");
				continue;
			}
			choices[f_num] = (char **)malloc((max+1)*sizeof(char **));
			if (choices[f_num] == NULL) {
				(void)fprintf(stderr, "Couldn't malloc choices\n");
				continue;
			}
			field_length = 0;
			for (i=0; i<max; i++) {
				f_kwp = f_kwp->next;
/*
 *  Read form description line TAG
 */
choices[f_num][i] = ""; /* initialize to null string just in case */
tags[f_num][i] = ""; /* initialize to null string just in case */
if ((cp2 = (char *)strstr(f_kwp->buffer, "::")) != NULL) {
	cp1 = f_kwp->buffer;
	*cp2 = '\0';
	if (strlen(cp1)) {
		tags[f_num][i] = f_kwp->buffer;
	}
	else {
		tags[f_num][i] = (char *)0;
	}



	cp1 = cp2+2;
} else
	cp1 = f_kwp->buffer;
				choices[f_num][i] = cp1;
				if (field_length < (int)strlen(choices[f_num][i]))
					field_length = strlen(choices[f_num][i]);
			}
			choices[f_num][max] = (char *)0;
			tags[f_num][max] = (char *)0;

			/*
			 *  Create the ENUM FIELD
			 */
			fields[f_num] = new_field(1, field_length, row, DELIM, 0, 2);
			if (fields[f_num] == NULL) {
				(void)fprintf(stderr, "couldn't new_field %d\n", f_num);
				continue;
			}

			set_field_type(fields[f_num], TYPE_ENUM, choices[f_num], FALSE, FALSE);
			set_field_buffer(fields[f_num], 0, *choices[f_num]);
			set_field_buffer(fields[f_num], 1, *choices[f_num]);
			set_field_buffer(fields[f_num], 2, *tags[f_num]);
			field_opts_off(fields[f_num], (OPTIONS)O_EDIT);

			set_field_fore(fields[f_num], select_attr);
			set_field_back(fields[f_num], select_attr);

			break;
		case '3':
		case '4':
			/*
			 *  Get the parms off the line
			 */
			if (sscanf(f_kwp->buffer, "%c %d %d %d", &tp, &precision, &min, &max)<4) {
				(void)fprintf(stderr, "Couldn't parse args\n");
				continue;
			}
			/*
			 *  Create the integer/numeric FIELD
			 */
			fields[f_num] = new_field(1, fld_wid(max), row, DELIM, 0, 2);
			if (fields[f_num] == NULL) {
				(void)fprintf(stderr, "couldn't new_field %d\n", f_num);
				continue;
			}
			if (tp=='3') {
				set_field_type(fields[f_num], TYPE_INTEGER, precision, (long)min, (long)max);
	}
			else
				set_field_type(fields[f_num], TYPE_NUMERIC, precision, (long)min, (long)max);
			set_field_fore(fields[f_num], input_attr);
			set_field_back(fields[f_num], input_attr);

			/*
			 *  Setting buffer 2 to "Y" means "Y"es, check this
			 *  field for being null in after_field().
			 */
			set_field_buffer(fields[f_num], 2, "Y");
			break;
		case '5':
			/*
			 *  Get the parms off the line
			 */
			if (sscanf(f_kwp->buffer, "%c %d %d", &tp, &min, &max)<3) {
				(void)fprintf(stderr, "Couldn't parse args\n");
				continue;
			}
			/*
			 *  Create the REGEXP FIELD
			 */
			fields[f_num] = new_field(1, max, row, DELIM, 0, 2);
			if (fields[f_num] == NULL) {
				(void)fprintf(stderr, "couldn't new_field %d\n", f_num);
				continue;
			}

			/*
			 *  Get the regexp and set the REGEXP field up.
			 */
			f_kwp = f_kwp->next;
			set_field_type(fields[f_num], TYPE_REGEXP, f_kwp->buffer);
			set_field_fore(fields[f_num], input_attr);
			set_field_back(fields[f_num], input_attr);

			/*
			 *  Setting buffer 2 to "Y" means "Y"es, check this
			 *  field for being null in after_field().
			 */
 			if (min)
				set_field_buffer(fields[f_num], 2, "Y");
			break;
		/*
		 *  LABEL field
		 */
		case '6':
			f_kwp = f_kwp->next;
			fields[f_num] = new_field(1, strlen(f_kwp->buffer), row, DELIM, 0, 2);
			if (fields[f_num] == NULL) {
				(void)fprintf(stderr, "couldn't new_field %d\n", f_num);
				continue;
			}
			set_field_buffer(fields[f_num], 0, f_kwp->buffer);
			set_field_buffer(fields[f_num], 1, f_kwp->buffer);
			set_field_buffer(fields[f_num], 2, "N");
			set_field_back(fields[f_num], regular_attr);
			set_field_fore(fields[f_num], regular_attr);
			field_opts_off(fields[f_num], (OPTIONS)O_ACTIVE);
			field_opts_off(fields[f_num], (OPTIONS)O_EDIT);
			break;
		default:
			(void)fprintf(stderr, "Invalid field type\n");
			/*
			 *  This makes up for puttinf f_num+=2 outside of the
	 		 *  switch() stmt
	 		 */
			f_num -= 2;
			continue;
		}
		/*
		 *  Create the Label
		 */
		f_kwp = f_kwp->next;
		fields[f_num+1] = make_label(&row, DELIM - strlen(f_kwp->buffer) - 2, f_kwp->buffer);
		if (maxlabel < (int)strlen(f_kwp->buffer))
			maxlabel = strlen(f_kwp->buffer);

		/*
		 *  See if there's a value in the env vble & use it.
		 */
		f_kwp = f_kwp->next;
		field_env_vble(f_num, fields[f_num], f_kwp->buffer);


		/*
		 *  Set up the field_label (which gets displayed on left
		 *  side of help bar)
		 */
		f_kwp = f_kwp->next;
		if ( (field_label[f_num] = (char *)malloc(strlen(f_kwp->buffer)+1)) == NULL) {
			(void)fprintf(stderr, "couldn't malloc label %s\n", f_kwp->buffer);
			curs_set(0);
			refresh();
			curs_set(1);
			refresh();
			reset_shell_mode();
			exit(ENOMEM);
		}
		strcpy(field_label[f_num], f_kwp->buffer);
		f_num += 2;
		f_kwp = &f_kw;
	}

	/*
	 *  Add the "Apply" and "Reset" buttons
	 */
	radio_buttons(&f_num, row);
	f_num++;
	last_field = f_num;

	/*
	 *  Move the form fields over to the left a bit.
	 */
	adjust_fields(maxlabel);

	return(f_num);
}

/*
 *  Create radio buttons at bottom of form.
 */
radio_buttons(f_num, row)
int	*f_num;
int	row;
{
	int	left, right;		/* where to place left, right radios */
	struct	keywords *kwp;		/* tmp ptr to kwd struct */

	if (!APPLY || !strlen(APPLY)) {	/* nullptr */
		kwp = &keywords[KW_BUTTONS];
		kwp = (struct keywords *)add_a_buffer(DFLT_APPLY, kwp);
		kwp = (struct keywords *)add_a_buffer(DFLT_RESET, kwp);
	}
		
	/*
	 *  Create the LABEL FIELD
	 */
	left = ((COLS/2) - strlen(APPLY) - 5);
	right = ((COLS/2) + 5);

	fields[*f_num] = new_field(1, strlen(APPLY), row+1, left, 0, 2);
	if (fields[*f_num]) {
		set_field_buffer(fields[*f_num], 0, APPLY);
		set_field_buffer(fields[*f_num], 1, APPLY);
		set_field_back(fields[*f_num], regular_attr);
		set_field_fore(fields[*f_num], regular_attr);
		field_opts_off(fields[*f_num], (OPTIONS)O_EDIT);
	}
	
	(*f_num)++;

	fields[*f_num] = new_field(1, strlen(RESET), row+1, right, 0, 2);
	if (fields[*f_num]) {
		set_field_buffer(fields[*f_num], 0, RESET);
		set_field_buffer(fields[*f_num], 1, RESET);
		set_field_back(fields[*f_num], regular_attr);
		set_field_fore(fields[*f_num], regular_attr);
		field_opts_off(fields[*f_num], (OPTIONS)O_EDIT);
	}
}

/*
 *  Slide the form over towards the left side of the screen now that we
 *  know how big the largest label is
 */
void
adjust_fields(maxlabel)
int	maxlabel;		/* Size of largest label */
{
	int	rows, cols, frow, fcol, nrow, nbuf;	/* field_info args */
	int	i;					/* counter */

	for (i=first_field; i<last_field -2; i++) {
		field_info(fields[i], &rows, &cols, &frow, &fcol, &nrow, &nbuf);
		fcol -= (DELIM - maxlabel - 4);  /* MARGIN */
		if (i==(last_field-1) || i==(last_field-2))
			frow++;
		move_field(fields[i], frow, fcol);
	}
}

/*
 *  Set up use of environmental variable associated with a field.  Stores
 *  variable name, and sets field buffer if that variable is set in the
 *  env.  Returns what the getenv() returned.
 */
char *
field_env_vble(index,f, buf)
int	index; 	/* What field number is this? */
FIELD *f;	/* What field are we doing this for */
char *buf;	/* What is the environment variable */
{
	char	*env;		/* to hold environment variable value */
	int	i;		/* counter for checking tags */
	int	set2tag;	/* did we set ENUM field to TAG? */

	/*
	 *  We fetch the environment variable used to store the value
	 *  for this field.
	 */
	if ( (env = (char *)getenv(buf)) != NULL) {
		
		/*
		 *  If this field is an ENUM type, the env variable might be
		 *  set to a TAG value, so we check and set to the correspond-
		 *  ing value from the choices[] array.
		 */
		if (field_type(f) == TYPE_ENUM) {
			set2tag = 0;
			for (i=0; *(tags[index][i]) != '\0'; i++) {
				if (!strncmp(tags[index][i], env, strlen(env))) {
					set2tag = 1;
					set_field_buffer(f, 0, choices[index][i]);
					set_field_buffer(f, 1, choices[index][i]);
				}
			}
			if (!set2tag) {
				set_field_buffer(f, 0, env);
				set_field_buffer(f, 1, env);
			}
		} else {
			set_field_buffer(f, 0, env);
			set_field_buffer(f, 1, env);
		}
	}

	if ( (field_vble[index] = (char *)malloc(strlen(buf)+1)) == NULL) {
			(void)fprintf(stderr, "couldn't malloc vble %s\n", buf);
			curs_set(0);
			refresh();
			curs_set(1);
			refresh();
			reset_shell_mode();
			exit(ENOMEM);
	}

	/*
	 *  This is a hack to work around a spurious control character
	 *  ETI places in the field buffer sometimes.
	 */
	memset(field_vble[index], '\0', strlen(buf)+1);

	strcpy(field_vble[index], buf);

	return(env);
}


/*
 *  For each active field, print out string VARIABLE="value" to -o (output)
 *  file. If unwriteable, use stderr (I mean, we got *this* far!)
 */
void
write_output(output_file)
char	*output_file;			/* Pathname of file for output */
{
	FILE	*output_fp;		/* file descriptor for output */
	int	field_cnt;		/* counter for fields */
	char	buf[80];		/* tmp buffer to hold output */
	char	*cp;			/* ptr to operate on buffer */
	char	*choice;		/* ptr to operate on choices */
	char	*tag;			/* ptr to operate on tags */
	int	enum_cnt;		/* Counter for which enum choice */
	int	done;

	if ( (output_fp = fopen(output_file, "w+")) == NULL) {
		(void)fprintf(stderr, "Could not fopen %s to write\n", output_file);
		output_fp = (FILE *)stderr;
	}

	/*
	 *  Run through every field we ever allocated for this form,
	 *  even though some are just labels.  See next comment
	 */
	for(field_cnt=0; field_cnt<field_count(form); field_cnt++)


		/*
		 *  If this was an ACTIVE, EDITable field, then
		 *  write the value/tag variable assignment out to disk.
		 */  
		if (field_opts(fields[field_cnt]) & (O_ACTIVE | O_EDIT)) {


			/*
			 *  TAG - added for use with TAGS for enum types.
			 *  If we're looking at an ENUM TYPE...
			 */
			if (field_type(fields[field_cnt]) == TYPE_ENUM) {
				enum_cnt = 0;
				done = 0;
				while (!done) {
					/*
					 *  ...we match which response was 
					 *  selected, and...
					 */
/* nullptr */					if (field_buffer(fields[field_cnt],0))
					if (!strncmp(field_buffer(fields[field_cnt], 0), choices[field_cnt][enum_cnt],strlen(choices[field_cnt][enum_cnt]))) {
						done = 1;
						/*
						 *  ...if there was a TAG for
						 *  that ENUM TYPE, we use it,
						 *  otherwise we use the ENUM
						 *  TYPE choice itself!
						 */
						if ((tags[field_cnt][enum_cnt]) && strlen(tags[field_cnt][enum_cnt]))  /* nullptr */
							strcpy(buf, tags[field_cnt][enum_cnt]);
						else
							if (choices[field_cnt][enum_cnt]) /* nullptr */
							strcpy(buf, choices[field_cnt][enum_cnt]);
						cp = buf + strlen(buf);
						continue;
					}
					enum_cnt++;
				}
			} else {

			/*
			 *  Don't do it for the radio buttons!
			 */
			if (!(field_opts(fields[field_cnt]) & (O_EDIT)))
				continue;
			/*
			 *  If it's not an ENUM TYPE, we just use the value
			 *  of the buffer.
			 */
			strcpy(buf, field_buffer(fields[field_cnt], 0));
			cp = buf + strlen(buf);
			while(*(cp-1) == ' ') {
				*(cp-1) = '\0';
				cp--;
			}
}
			(void)fprintf(output_fp, "%s=\"%s\"\n",
				field_vble[field_cnt], buf);
		}

	/*
	 *  If we had not been forced to write to stderr, close the
	 *  output file.
	 */
	if (output_fp != stderr)
		fclose(output_fp);
}

/*
 *  This function takes a number and lets us know how many
 *  spaces it would take to type it in (+1).
 */
int
fld_wid(number)
int	number;
{
	int	result=0;

	while ( (number/=10) >= 1 )
		result++;

	return (result+2);
}

/*
 *  For each active field, reset to initial values.
 */
void
reset_form()
{
	int	field_cnt;		/* counter for fields */
	char	buf[80];		/* tmp buffer to hold output */
	char	*cp;			/* ptr to operate on buffer */

	for(field_cnt=0; field_cnt<field_count(form); field_cnt++)
		if (field_opts(fields[field_cnt]) & (O_ACTIVE | O_EDIT)) {
			set_field_buffer(fields[field_cnt], 0, field_buffer(fields[field_cnt], 1));
		}
}

static void
secret_screen_dump()
{
	char	filename[80];
	char	outbuf[128];
	char	*cp;
	int	sv_r, sv_c;
	int	r,c;
	FILE	*scr_fd;

	getyx(w1, sv_r, sv_c);

	cp = strrchr(menu_file, '/');
	if (cp != NULL)
		cp++;
	else
		cp = menu_file;
	sprintf(filename, "/tmp/DUMP%s.%d", cp, num_scr_dump);
	if ((scr_fd=fopen(filename, "w+")) == NULL)
		return;

	for (r=0; r<LINES; r++) {
		for (c=0; c<COLS; c++) {
			outbuf[c] = mvwinch(w1, r, c)&A_CHARTEXT;
		}
		outbuf[c] = '\0';
		fprintf(scr_fd, "%s", outbuf);
	}
	num_scr_dump++;
	fclose(scr_fd);

	wmove(w1, sv_r, sv_c);
}
