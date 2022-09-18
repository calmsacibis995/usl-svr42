/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)menu.cmd:menu.h	1.1"
#ident	"$Header: $"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <curses.h>
#include <form.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#define	HHELP_FMT "/etc/inst/locale/%s/menus/helphelp"
#define	ERROR_FMT "/etc/inst/locale/%s/menus/menu.errs"

#define	KW_UL	0
#define	KW_UR	1
#define	KW_LL	2
#define	KW_LR	3
#define	KW_TOP	4
#define	KW_FORM	5
#define	KW_BOTTOM	6
#define	KW_HELP	7
#define	KW_HELPINST	8
#define	KW_HHELPINST	9
#define	KW_WORKING	10
#define	KW_HELPBANNER	11
#define	KW_HHELP_BAN	12
#define	KW_PAGENO	13
#define	KW_OPTSTRING	14
#define	KW_SELECTION	15
#define	KW_HELPHELP	16
#define	KW_BUTTONS	17
#define	KW_END	18

#define TYPE_FORM	0
#define TYPE_MENU	1
#define TYPE_ENTER	2

#define	PARSE_BUFSZ	512

struct	keywords {
	char	*delimiter;
	char	*buffer;
	struct	keywords *next;
};

/* 
 *  define's for Error strings
 */
#define	ERR_BADKEY	1	/* Invalid keystroke */
#define	ERR_BADDATA	2	/* Invalid data in field  */
#define	ERR_INCFORM	3	/* Cannot exit from incomplete form */
#define	ERR_NOHELP	4	/* No Help for this step */
#define	ERR_LASTPG	5	/* On last page */
#define	ERR_FIRSTPG	6	/* On first page */
#define	ERR_WINSZ	7	/* Window too small */
#define	ERR_NULLFLD	8	/* Can't have a null field here */
#define	ERR_NOCANCEL	9	/* Can't Cancel from here */

void	start_curses();
void	end_curses();
void	draw_bg();
void	put_err();
int	do_form();
void	main();
void	parse();
struct	keywords *io_redir();
struct	keywords *add_a_buffer();
int	get_spaces();
int	strlines();

#ifdef MAIN
struct keywords keywords[]={
	{".ul", ""},
	{".ur", ""},
	{".ll", ""},
	{".lr", ""},
	{".top", ""},
	{".form", ""},
	{".bottom", ""},
	{".help", ""},
	{".helpinst", ""},
	{".hhelpinst", ""},
	{".working", ""},
	{".helpbanner", ""},
	{".hhelp_ban", ""},
	{".pageno", ""},
	{".optstring", ""},
	{".selection", ""},
	{".helphelp", ""},
	{".button", ""},
	{".end", ""},
	{"0"}
};

/*
 *  Different attributes for setting video
 */
int	regular_attr, help_attr, select_attr, input_attr, error_attr;

/*
 *  Page counts for multi-page text
 */
int	cur_pg, tot_pg;

/*
 *  This is the global form for processing
 */
FORM	*form;
FIELD	*fields[50];
char	*field_label[50];
/* char	*field_vble[50]; */
WINDOW	*w1;

int	error_displayed;
int	rflag;
int	first_field;
int	last_field;
int	formtype;
#endif

#ifndef MAIN
extern struct	keywords keywords[];
extern int	regular_attr, help_attr, select_attr, input_attr, error_attr;
extern FORM	*form;
extern FIELD	*fields[];
extern char	*field_label[];
/* extern char	*field_vble[]; */
extern int	error_displayed;
extern WINDOW	*w1;
extern int	cur_pg, tot_pg;
extern int	rflag;
extern	int	first_field;
extern	int	last_field;
extern	int	formtype;
#endif
