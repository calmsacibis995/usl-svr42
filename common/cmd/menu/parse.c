/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)menu.cmd:parse.c	1.1"
#ident	"$Header: $"

#undef	MAIN
#include "menu.h"

struct keywords *add_a_buffer();
struct keywords *io_redir();
static struct keywords *cmd_sub();

extern	char *menu_file;

/*
 *  open and parse the menu description file.
 */
void
parse()
{
	FILE*	menu_fp;	/* File ptr for menu desc file */
	char	buf[PARSE_BUFSZ];	/* tmp buffer to hold menu file lines */
	struct	keywords *kwp;	/* pointer to keywords structure */
	int	len,i;		/* counter for which keyword we're matching */

	if ( (menu_fp = fopen(menu_file, "r")) == NULL ) {
		(void)fprintf(stderr, "Could not fopen() %s\n", menu_file);
		exit(EACCES);
	}

	for (i=0; i<KW_END; i++) {
		keywords[i].buffer = NULL;
		keywords[i].next = NULL;
	}

	kwp = &keywords[KW_END];

	while( fgets(buf, PARSE_BUFSZ, menu_fp) != NULL ) {
		len = strlen(buf);
		if (i==KW_FORM && buf[0]=='\n') {
			continue;
		} else
		if (len>1) 
			buf[len-1] = '\0';

		/*
		 *  Allow shell-style comments
	 	 */
		if (buf[0]=='#')
			continue;

		/*
		 *  Found a keyword
		 */
		if (buf[0] == '.') {
			/*
			 *  Which keyword is it?
			 */
			for (i=0; i<KW_END; i++) {
				/*
				 *  Make new buffer current
				 */
				if (!strcmp(keywords[i].delimiter, buf)) {
					kwp = &keywords[i];
					kwp->next = NULL;
					kwp->buffer = NULL;
					break;
				}
			}
			continue;
		}
		/*
		 *  If not a keyword, check to see if we need to do any I/O
		 *  redirection or command substitution.
	 	 */
		if (strchr(buf, 0140) != NULL) {
			/*
			 *  Command substitution
			 */
			kwp = (struct keywords *)cmd_sub(buf, kwp);
			continue;
		}
		if (buf[0]=='<') {
			/*
			 *  I/O Redirection
			 */
			kwp = (struct keywords *)io_redir(buf+1, kwp);
			continue;
		}

		kwp = add_a_buffer(buf, kwp);
	}

	(void)fclose(menu_fp);

	/*
	 *  Now suck in the help within help text
	 */
	locale_ify(HHELP_FMT, buf);
	kwp = &keywords[KW_HELPHELP];
	kwp = (struct keywords *)io_redir(buf, kwp);
	
}

/*
 *  Make sure we get the correct suppport files from the $LANG
 *  environment, default to 'C' locale if LANG not set, and
 *  go away nicely if nothing was usable.
 */
locale_ify(format, filename)
char	*format;		/* sprintf() format for pathname */
char	*filename;		/* buffer to hold filename */
{
	char	*lang;		/* contents of LANG env variable */

	/*
	 *  If we get LANG from env, use it.  Otherwise, use C-locale.
	 */
	if ( (lang=(char *)getenv("LANG")) != NULL )
		sprintf(filename, format, lang);
	else
		sprintf(filename, format, "C");

	/*
	 *  If we can't read this one, then if its not the C-locale,
	 *  try the C-locale.  If we can't read the C-locale, then
	 *  complain and exit.
	 */
	if ( access(filename, 04) < 0) {
		sprintf(filename, format, "C");
			if ( access(filename, 04) < 0) {
				fprintf(stderr, "Cannot access %s\n", filename);
				exit(EACCES);
			}
	}

}

/*
 *  open and parse file for I/O redirection.
 */
struct keywords *
io_redir(path, kwp)
char	*path;			/* Path name to redirect from */
struct keywords *kwp;		/* Placeholder in keyword list */
{
	FILE*	tmp_fp;		/* File ptr for file */
	char	buf[PARSE_BUFSZ];	/* tmp buffer to hold menu file lines */

	while(*path == ' ' || *path == 0x09 )
		path++;

	if ( (tmp_fp = fopen(path, "r")) == NULL ) {
		(void)fprintf(stderr, "Could not fopen() %s\n", path);
		exit(EACCES);
	}

	while( fgets(buf, PARSE_BUFSZ, tmp_fp) != NULL ) {
		buf[strlen(buf)-1]='\0'; 
		kwp = add_a_buffer(buf, kwp);
	}

	(void)fclose(tmp_fp);
	return(kwp);
}

struct keywords *
cmd_sub(cmdline, kwp)
char	*cmdline;		/* buffer containing command to be executed */
struct keywords *kwp;		/* keyword pointer */
{
	char	*cp1, *cp2;		/* For delimiting command from buffer */
	char	tmp1[PARSE_BUFSZ];	/* For reading output from popen() */
	char	tmp2[PARSE_BUFSZ];	/* For reading output from popen() */
	char	buf[PARSE_BUFSZ];	/* For assembling line */
	int	lines_read=0;		/* number of lines read from cmd */
	FILE	*pipe_fp;		/* For reading output from popen() */

	cp1 = (char *)strchr(cmdline, 0140);
	if ((cp2 = (char *)strchr(cp1+1, 0140)) == NULL) {
		(void)fprintf(stderr, "Unmatched backticks\n");
		exit(2);
	}

	*cp1='\0';
	*cp2='\0';

	memset(buf, '\0', PARSE_BUFSZ);
	memset(tmp1, '\0', PARSE_BUFSZ);
	memset(tmp2, '\0', PARSE_BUFSZ);

	if ( (pipe_fp = popen(cp1+1, "r")) == NULL ) {
		(void)fprintf(stderr, "Could not popen(%s)\n", cp1+1);
		return(kwp);
	}

	/*
	 *  Place beginning of line in buffer for adding to list.
	 */
	if ( strlen(cmdline) )
		strcpy(buf, cmdline);

	/*
	 *  As we read lines from the command...
	 */
	while( fgets(tmp1, PARSE_BUFSZ, pipe_fp) != NULL) {

		/*
		 *  Strip off any carriage return at end of line
		 */
		if (*(tmp1 + strlen(tmp1) -1) == '\n')
			*(tmp1 + strlen(tmp1) -1) = '\0';

		/*
		 *  and increment our idea of how many lines we read.
		 */
		lines_read++;

		/*
		 *  If this is the first line, then tack what we read onto
		 *  the end of the part before the backtick
		 */
		if (lines_read == 1) {
			(void)strcat(buf, tmp1);

			/*
			 *  and copy to tmp2 in case there's another line
			 *  to be read.
			 */
			strcpy(tmp2, buf);
		} else {

			/*
			 *  If this isn't the first line, add the previous
			 *  line to the list.  This will leave the last line
			 *  for the outside of the while loop so we can tack
			 *  on any text AFTER the backtick and add that to
			 *  the list.
			 */
			kwp = add_a_buffer(tmp2, kwp);
			strcpy(tmp2, tmp1);
		}

	}

	/*
	 *  Tack on any text from after the backtick, and add the buffer.
	 *  Which buffer depends on if we read more than one line from the
	 *  command.
	 */
	if (lines_read == 1) {
		if (strlen(cp2+1))
			(void)strcat(buf, cp2+1);
		kwp = add_a_buffer(buf, kwp);
	} else {
		if (strlen(cp2+1))
			(void)strcat(tmp2, cp2+1);
		kwp = add_a_buffer(tmp2, kwp);
	}
		
	(void)pclose(pipe_fp);

	return(kwp);
}


/*
 *  Routine to add a buffer to the current keyword structure,
 */
struct	keywords *
add_a_buffer(buffer, kwp)
char	*buffer;
struct	keywords *kwp;
{

	/*
	 *  Allocate the buffer
	 */
	kwp->buffer = (char *)malloc(strlen(buffer)+1);
	if (kwp->buffer == NULL) {
		(void)fprintf(stderr, "Could not malloc buffer\n");
		exit(ENOMEM);
	}
	/*
	 *  Copy the string in
	 */
	if ( strcpy(kwp->buffer, buffer) == NULL ) {
		(void)fprintf(stderr, "Could not strcpy to buffer\n");
		exit(ENOMEM);
	}

	/*
	 *  Allocate the next struct keywords
	 */
	kwp->next = (struct keywords *)malloc(sizeof(struct keywords));
	if (kwp->next == NULL) {
		(void)fprintf(stderr, "Could not malloc struct\n");
		exit(ENOMEM);
	}
	kwp = kwp->next;
	kwp->next = NULL;
	kwp->buffer = NULL;

	return(kwp);
		
}

