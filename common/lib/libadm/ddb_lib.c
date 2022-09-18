/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/ddb_lib.c	1.1.12.4"
#ident	"$Header: ddb_lib.c 1.3 91/06/25 $"

#include <stdio.h>	/* standard I/O definitions     */
#include <string.h>	/* string handling definitions  */
#include <ctype.h>	/* character types & macros     */
#include <errno.h>	/* error codes                  */
#include <stdlib.h>	/* storage alloc functions      */
#include <stdarg.h>	/* arglist macro definitions    */
#include <pwd.h>	/* struct passwd defined        */
#include <grp.h>	/* struct group defined        */
#include <sys/time.h>	/* Time definitions             */
#include <mac.h>	/* MAC functions                */
#include <sys/types.h>	/* System data types            */
#include <sys/stat.h>	/* File status information      */
#include <devmgmt.h>	/* global devmgmt definitions   */
#include "devtab.h"	/* local devmgmt definitions    */

/*
 * Functions defined in ddb_lib.c:
 *
 *	ddb_errmsg()	formats specified error message in msgbuf
 *	ddb_errset()	saves error code in ddbmsg buffer
 *	ddb_errget()	returns saved error code from ddbmsg buffer
 *	err_report()	displays error message on stderr and exits
 *
 *	getfield()	gets the next field terminated by delimiter
 *	skpfield()	skips to the next field after delimiter
 *	getquoted()	extracts the quoted string from input string
 *	getlistcnt()	returns number of items in value-list
 *	getattrtype()	returns the type of input attribute (sec/oam/dsf)
 *	getattrval()	returns the value of specified attr from dev_record
 *
 *	valid_alias()	validates device alias (Secure or Logical)
 *	valid_path()	validates path to be an absolute pathname
 *	valid_stperm()	validates startup DAC permissions
 *	valid_perm()	validates User Authorization List permissions
 *
 *	parse_range()	parses device range string, returns hilevel & lolevel
 *	parse_state()	parses device state string, returns values
 *	parse_mode()	parses device mode string, returns values
 *	parse_uid()	parses user name/id string, returns uid_t
 *	parse_gid()	parses group name/id string, returns gid_t
 *	make_ual()	makes sorted array of ual_entry's from ual string
 *	make_users()	makes users string from ual_entry's array
 *	parse_users()	parses User Authorization List, returns sorted users
 *	_insert_uid()	inserts uid in ascending order into ual_entry array	
 *	_mac_installed()	checks if MAC is installed.
 *	mac_running()	checks if MAC is running.
 *	gettabversion()	determines if device.tab is a 4.0 or 4ES version
 */
/*
 * L O C A L  D E F I T I O N S
 */
/*
 * Static Global Variables:
 *
 *	ddbmsg		ddb error message buffer
 *	sevs[]		array of char strings-- defines severity of error msg.
 *	sec_attrs[]	array of pre-defined security attr names
 *	tab_attrs[]	array of pre-defined OA&M(tab) attr names
 *	dsf_attrs[]	array of pre-defined dsf attr names
 */
static struct msgbuf	ddbmsg = { SEV_NONE, 0, 0, '\0' };
static char	*sevs[] = {	"NOTHING",
				"ERROR",
				"WARNING"
			  };

/* security attributes of devices */
static char	*sec_attrs[]= {
			DDB_ALIAS,
			DDB_RANGE,
			DDB_STATE,
			DDB_MODE,
			DDB_STARTUP,
			DDB_ST_LEVEL,
			DDB_ST_OWNER,
			DDB_ST_GROUP,
			DDB_ST_OTHER,
			DDB_UAL_ENABLE,
			DDB_USERS,
			DDB_OTHER
		};

/* oam(tab) attributes of devices */
static char	*tab_attrs[]= {
			DDB_ALIAS,
			DDB_SECDEV,
			DDB_PATHNAME
			/* any other OA&M attrs names can be added 
			 * to DDB_TAB without being defined here.
                         */
		};

/* dsf attributes of devices */
static char	*dsf_attrs[]= {
			DDB_CDEVICE,
			DDB_BDEVICE,
			DDB_CDEVLIST,
			DDB_BDEVLIST
};


/*
 *  void ddb_errmsg(int sev, int excode, char *fmt,...)
 *
 *	This function copies the error message components -- <fmt>
 *	and the variable argument list into the static message buffer, 
 *	<ddbmsg>, and sets the exit code to <excode>, and severity to <sev>.
 *
 *  Arguments:
 *      int  excode      exit code
 *      int  severity    severity code
 *      char *fmt        format string for vsprintf()
 *      char *...        specific values to be subsitituted in format string
 *
 *  Returns: void
 *
 */

void 
ddb_errmsg(int sev, int excode, char *fmt,...)
{
	va_list	args;

	/* make args point to the 1st unnamed arg */
	va_start(args, fmt);

	ddbmsg.excode = excode;             /* set error/exit code       */
	ddbmsg.sev = sev;                   /* set severity of error     */
	/* form error message in ddbmsg buffer */
	vsprintf(ddbmsg.text, fmt, args);
	va_end(args);
}

/*
 *  void ddb_errset(errnum)
 *	int    errnum;
 *
 *	This function saves the current errnum in
 *	the static ddbmsg.errnum.
 *
 *  Arguments:
 *      int  errnum      error number(errno)
 *
 *  Returns: void
 *
 */

void
ddb_errset(errnum)
	int    errnum;
{
	    ddbmsg.errnum = errnum;
}

/*
 *  int ddb_errget()
 *
 *	This function gets the saved errnum
 *	from the static ddbmsg.errnum.
 *
 *  Arguments:
 *
 *  Returns: int
 *	returns saved errnum from ddbmsg.errnum.
 *
 */

int
ddb_errget()
{
	return(ddbmsg.errnum);
}
/*
 *  void err_report(cmdname, action)
 *	char   *cmdname; 
 *	int    action;
 *
 *	This function displays the error message in <msgbuf> to stderr.
 *	If <action>=
 *	    ACT_QUIT      it exits with exit code(msgbuf.ecode)
 *	    ACT_CONT      it resets <msgbuf> and continues.
 *
 *  Arguments:
 *      char  *cmdname    name of the command that failed
 *      int   action      ACT_QUIT or ACT_CONT
 *
 *  Returns: void
 *
 */
void
err_report(cmdname, action)
	char   *cmdname; 
	int	action;
{
	/* display error message on <stderr> */
	fprintf(stderr, "UX:%s:%s:%s", cmdname, 
	         sevs[ddbmsg.sev], ddbmsg.text);
	if (action == ACT_QUIT) {
	    /* exit with exit-code in msgbuf */
	    exit(ddbmsg.excode);
	} else {
	    /* reset msgbuf to NULL values */
	    ddbmsg.sev = SEV_NONE;
	    ddbmsg.excode = 0;
	    ddbmsg.errnum = 0;
	    ddbmsg.text[0] = '\0';
	}
}

/*
 *  char *getfield(ptr, delims, next)
 *	char   *ptr;
 *	char   *delims;
 *	char   **next;
 *
 *  Function returns field starting from <ptr>, and ending with one of
 *  the delimiters, <delims>. It also returns the pointer to next field, <next>,
 *  after delimiter. It converts delimiter found into '\0'.
 *
 *  Arguments:
 *	ptr	- pointer to char string to parse.
 *	delims	- pointer to delimiters to search for, in <ptr>
 *		  '\0' cannot be specified as a delimiter.
 *	next	- Address of pointer to next field.
 *		  Returns pointer to next field in <next>
 *
 *  Returns: char *
 *	field ptr   - if successful in finding delimiter
 *	(char *)NULL- if delimiter NOT found
 *
 *  Notes:
 *    -	Can't use "strtok()" because of its use of static data.  The caller
 *	may be using strtok() and we'll really mess them up.
 */

char *
getfield(ptr, delims, next)
	char   *ptr;
	char   *delims;
	char   **next;
{
	int	done;		/* TRUE if we're finished     */
	char   *p, *st;		/* Temp pointer               */

	p = ptr;
	while (*p && isspace(*p)) p++;              /* eat white space     */

	st = *next = p;                             /* ptr to start of str */
	if (p && (*p)) {			    /* Anything to do ?? */
	    done = FALSE;			    /* We're not done yet */
	    while (*p && !done) {		    /* Any more chars */
		if (*p == '\\') {		    /* Escaped ? */
		  if (*(++p)) p++;		    /* Skip escaped char */
		}
		else if (!strchr(delims, *p)) p++;  /* Skip non-delims */
	 	else done = TRUE;		    /* Otherwise, done */
	    }
	    if (*p) {				    /* Terminator found? */
		*p++ = '\0';			    /* Null-terminate token */
		*next = p;			    /* return next field    */
	    }
	    else {
		return((char *)NULL);               /* delim not found */
	    }
	} else  return((char *)NULL);               /* delim not found */

	/*  Finished  */
	return(st);                                /* SUCCESS, delim found   */
}

/*
 *  char *skpfield(str, delims)
 *	char   *str;
 *	char   *delims;
 *
 *  Function returns pointer to the next field after any one of the specified
 *  delimiters(<delims>) occurs in the input string <str>.
 *
 *  Arguments:
 *	str	- pointer to input char string to parse.
 *	delims	- pointer to delimiters to search for, in <ptr>
 *		  '\0' cannot be specified as a delimiter.
 *
 *  Returns: char *
 *	If delimter found, it returns ptr to the next char string 
 *		after any one of <delims>
 *	If delimiter not found, it returns a NULL pointer.
 *
 *  Notes:
 *    -	Can't use "strtok()" because of its use of static data.  The caller
 *	may be using strtok() and we'll really mess them up.
 */

char *
skpfield(str, delims)
	char   *str;
	char   *delims;
{
	int	done;		/* TRUE if we're finished     */
	char   *p;		/* Temp pointer               */

	p = str;                            /* ptr to start of str */

	if (p && (*p)) {			    /* Anything to do ?? */
	    done = FALSE;			    /* We're not done yet */
	    while (*p && !done) {		    /* Any more chars */
		if (*p == '\\') {		    /* Escaped ? */
		  if (*(++p)) p++;		    /* Skip escaped char */
		}
		else if (!strchr(delims, *p)) p++;  /* Skip non-delims */
	 	else done = TRUE;		    /* Otherwise, done */
	    }
	    if (*p) {				    /* Terminator found? */
		/* skip delimiter and return next field */
		return(++p);
	    }
	    else {
               /* delim not found, return original ptr */
		return((char *)NULL);
	    }
	}
}

/*
 *  char *getquoted(ptr, next)
 *	char   *ptr;
 *	char   **next;
 *
 *  This function extracts a quoted string from the string pointed
 *  to by <ptr>, and returns pointer to the first character in string.
 *  The character string following the end quote, is returned in 
 *  <next>.
 *
 *  Arguments:
 *	ptr	- Pointer to the character-string to parse
 *	next	- Address of pointer to string following end quote
 *		  Returns pointer to string following end of quote.
 *
 *  Returns:  char *
 *	Pointer to string within quotes.
 *
 */

char
*getquoted(ptr, next)
	char   *ptr;
	char   **next;
{
	char   *rtn;		/* Value to return */
	char   *p;		/* Temps           */

	*next = p = ptr;		/* ptr to start of string */

	/* If there's anything to parse and it's a quoted string ... */
	if ((p) && (*p == '"')) {
	    if (rtn=getfield(p+1, "\"", next)) {
		return(rtn);
	    } else return((char *)NULL);
	} else     return((char *)NULL);
}

/*
 *  int getlistcnt(str)
 *	char   *str;
 *
 *  This function returns the number of list elements present in the
 *  input <str>. Each list item is assumed to be separated by commas (","),
 *  and the last item is '\0' terminated.
 *
 *  Arguments:
 *	str	- Pointer to the character-string to parse
 *
 *  Returns:  int
 *	Number of items in the input string.
 *
 */

int
getlistcnt(str)
char	*str;
{
	char		*p, *q;		/* Temps           */
	register int    cnt;		/* counter	   */

	p = str;		/* ptr to start of string */
	cnt = 0;		/* reset counter          */

	/* If there's anything to parse ...       *
	 * skip to next field after "," separator */
	while ((*p) && (q=skpfield(p,","))) {
	    p = q;	/* bump ptr to next item */
	    cnt++;	/* bump count */
	}
	/* increment count for the only item or last item (if defined) */
	if (*p)
	    cnt++;
	return(cnt);
}

/*
 *  int  getattrtype(attr, item)
 *	char   *attr;
 *      int    *item;
 *
 *	This function returns the type of attribute name defined in <attr>.
 *	The attribute types available are the following:
 *		TYPE_SEC	attr name in sec_attrs[] array
 *		TYPE_DSF	attr name in dsf_attrs[] array
 *		TYPE_TAB	attr name in tab_attrs[] array
 *	It also returns the element number, <item>, in the corresponding 
 *      array in which the attribute was found.
 *
 *  Arguments:
 *	char *attr	The input attribute=value string
 *	item *item	Returned item number in corresponding array.
 *
 *  Returns:  int
 *	TYPE_SEC	if attr name is one of sec_attrs[]
 *	TYPE_DSF	if attr name is one of dsf_attrs[]
 *	TYPE_TAB	if attr name is one of tab_attrs[] or any other
 *			attr name not pre-defined.
 */

int
getattrtype(attr, item)
char	*attr;
int	*item;
{
	int		i;

	if (*attr) {
	    /* attr = TYPE_SEC? */
	    for (i=1 ; i<MAXSECATTRS ; i++) {
		if (strcmp( attr, sec_attrs[i])==0) {
		    *item = i;			/* get item number matched */
		    return(TYPE_SEC);		/* return  TYPE_SEC        */
		}
	    }

	    /* attr = TYPE_DSF? */
	    for (i=0 ; i<MAXDSFATTRS ; i++) {
		if (strcmp( attr, dsf_attrs[i])==0) {
		    *item = i;			/* get item number matched */
		    return(TYPE_DSF);		/* return TYPE_DSF         */
		}
	    }

	    /* attr = TYPE_TAB? */
	    for (i=0 ; i<MAXTABATTRS ; i++) {
		if (strcmp( attr, tab_attrs[i])==0) {
		    *item = i;			/* get item number matched */
		    return(TYPE_TAB);		/* return TYPE_DSF         */
		}
	    }
	    /* if attribute type is still unknown */
	    /* should be a new(undefined) OAM attr */
	    *item = -1;			/* undefined item no.      */
	    return(TYPE_TAB);

	} else return(TYPE_UNK);
}

/*
 *  char  *getattrval(attr, devrec)
 *	char       *attr;
 *      dev_record *devrec;
 *
 *	This function returns the value for the specified <attr>, by
 *	first, identifying its attribute type, and then locating it
 *	in the corresponding entry field of <devrec>.
 *
 *	The action taken for the following available attr types are:
 *		TYPE_SEC	find attr-value in sec_entry of <devrec>.
 *		TYPE_DSF	find attr-value in dsf_entry of <devrec>.
 *		TYPE_TAB	find attr-value in tab_entry of <devrec>.
 *
 *  Arguments:
 *	char *attr		attribute name string.
 *	dev_record *devrec	dev_record structure.
 *
 *  Returns:  char *
 *	It returns the value of specified <attr>. If attribute is not defined
 *	it returns (char *)NULL.
 */

char *
getattrval(attr, devrec)
char		*attr;
dev_record	*devrec;
{
	int	etype;		/* attr type */
	int	fieldno;	/* field number */
	char	*value,		/* attr value extracted from devrec */
		*get_secattr(),
		*get_dsfattr(),
		*get_tabattr();

	/* determine type of attribute       */
	etype = getattrtype(attr, &fieldno);
	switch (etype) {
	case (TYPE_SEC):
	    /* - security attribute -  */
	    value = get_secattr(attr,fieldno,devrec->sec);
	    break;
	case (TYPE_DSF):
	    /* - dsf      attribute -  */
	    value = get_dsfattr(attr,fieldno,devrec);
	    break;
	case (TYPE_TAB):
	    /* - oam      attribute -  */
	    value = get_tabattr(attr,fieldno,devrec->tab);
	    break;
	default:
	    /* invalid attr=value pair */
	    return((char *)NULL);
	}
	return(value);
}

/*
 *  int valid_alias()
 *	char   *alias;
 *
 *	This function determines if the alias is valid.
 *	A valid alias is limited to DDB_MAXALIAS(64) chars
 *	and must contain only alphanumeric chars or 
 *	one of the following chars "_" "$" "-" "."
 *
 *	Note: Because of lack of time and to fix a boundary problem
 *	DDB_MAXALIAS is set to be 65, so that any declarations that use
 *	DDB_MAXALIAS have enough space for the '\0' that ends the string.
 *	Thus, valid_alias() checks that strlen(alias) <= DDB_MAXALIAS - 1. 
 *
 *  Arguments:
 *	char *alias	string (value) to validate
 *
 *  Returns:
 *	TRUE    - if alias is valid
 *	FALSE   - if alias is invalid
 *
 */

int
valid_alias(alias)
char	*alias;
{
	char	*p;		/* temp pointer        */
	int	len;		/* length of <alias>   */
	int	rtn;		/* return value        */

	/* assume the worst */
	rtn = FALSE;

	/* check length */
	if (((alias != NULL) && (len = strlen(alias)) > 0) && 
		(len <= DDB_MAXALIAS - 1 )) {
	    p = alias;
	    /* check each character for validity */
	    while ((*p) && (isalnum(*p) || strchr("$_-.",*p)))
		p++;
	    if (*p == '\0')
		rtn = TRUE;
	}

	/* return TRUE/FALSE */
	return(rtn);
}

/*
 *  int valid_path()
 *	char   *path;
 *
 *	This function determines if the <path> specified,
 *	is an absolute pathname.
 *
 *  Arguments:
 *	char *path	string (value) to validate
 *
 *  Returns:
 *	TRUE    - if path is valid
 *	FALSE   - if path is invalid
 *
 */

int
valid_path(path)
char	*path;
{
	if (*path == '/')
	    return(TRUE);
	else
	    return(FALSE);
}

/*
 *  int valid_stperm()
 *	char   *perm;
 *
 *	This function determines if the startup DAC permission
 *	string specified in <perm> is valid.
 *	This field is specified as part of value string for  
 *	the startup attrs - <startup_owner>, <startup_group>, <startup_other>.
 *
 *  Arguments:
 *	char *perm	string (value) to validate
 *			Valid values are:
 *			rwx, rw-, r-x, -wx, r--, -w-, --x, ---
 *
 *  Returns:
 *	TRUE    - if path is valid
 *	FALSE   - if path is invalid
 *
 */

int
valid_stperm(perm)
char	*perm;
{
	char		*ptr;
	int		i;
	static char	p[] = "rwx";

	ptr = perm;
	if (strlen(ptr) == 3) {
	    /* check if chars in <perm> match *
	     * those in <p> or match "-"      */
	    for (i=0 ; i<3 ; i++, ptr++) {
		if ((*ptr != p[i]) && (*ptr != '-')) {
		    return(FALSE);
		}
	    }
	} else {
	    return(FALSE);
	}
	return(TRUE);
}

/*
 *  int valid_perm()
 *	char   *perm;
 *
 *	This function determines if the UAL permissions
 *	string specified in <perm> is valid.
 *	This field is specified as part of value string for  
 *	the attrs - <users>, <other>
 *
 *  Arguments:
 *	char *perm	string (value) to validate
 *			Valid values are:
 *			"y", "n"
 *
 *  Returns:
 *	TRUE    - if path is valid
 *	FALSE   - if path is invalid
 *
 */

int
valid_perm(perm)
char	*perm;
{
	if ((strcmp(perm,"y")==0)||
		(strcmp(perm,"yes")==0)|| 
		(strcmp(perm,"no")==0)|| 
		(strcmp(perm,"n")==0)) {
	    return(TRUE);
	} else {
	    return(FALSE);
	}
}

/*
 *  int parse_range(str, hilevel, lolevel)
 *	char   *str;
 *	level_t	*hilevel;
 *	level_t	*lolevel;
 *
 *	This function validates the input string which should represent
 *	valid <hilevel,lolevel>, and converts the levels to MAC level
 *	ids (level_t), and returns them in <hilevel> & <lolevel>. 
 *
 *  Arguments:
 *	char *str		char string representing device range
 *	level_t *hilevel	High level in device level range
 *	level_t *lolevel	Low level in device level range
 *
 *  Returns:
 *	SUCCESS   - if valid range
 *	FAILURE   - if invalid range, and <hilevel,lolevel> contains LIDs= NULL.
 *
 */

int
parse_range(str, hilevel, lolevel)
	char	*str;
	level_t	*hilevel;
	level_t	*lolevel;
{
	char	*strhi, *strlo;		/* strings represent hi/lo levels */
	char	*tmp;			/* temp pointer                   */

	/* parse input string and extract range (hilevel, lolevel) */
	if((strhi=getfield(str, "-", &strlo))==(char *)NULL) {
	    /* error, delimiter "-" no found */
	    ddb_errmsg(SEV_ERROR, EX_ERROR , E_INDLM, DDB_RANGE, str);
	    return(FAILURE);
	}

	/* extract hilevel, and validate */
	if (lvlin(strhi, hilevel)==FAILURE) {
	    switch(errno) {
		case(EINVAL):
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INLVL, DDB_RANGE, strhi);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_LTDB);
		    break;
	    }
	    return(FAILURE);
	}

	/* replace "-" in range string */
	tmp = strlo - 1;
	*tmp = '-';

	/* validate lolevel */
	if (lvlin(strlo, lolevel)==FAILURE) {
	    switch(errno) {
		case(EINVAL):
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_INLVL, DDB_RANGE, strlo);
		    break;
		default:
		    ddb_errmsg(SEV_ERROR, EX_ERROR, E_LTDB);
		    break;
	    }
	    return(FAILURE);
	}
	/* got here, then range is valid */
	return(SUCCESS);
}

/*
 *  int parse_state(str)
 *	char   *str;
 *
 *	This function validates the input string which should represent
 *	valid device state, and returns the equivalent bunary value of
 *	state (DEV_PRIVATE, DEV_PUBLIC, or both).
 *
 *  Arguments:
 *	char 	*str	char string representing device state
 *
 *  Returns: int value of state
 *	     0	if <str> is invalid state
 *
 */
int
parse_state(str)
	char	*str;
{
	if (strcmp(str, DDB_PUBLIC)==0)
	    return(DEV_PUBLIC);
	else if (strcmp(str, DDB_PRIVATE)==0)
	    return(DEV_PRIVATE);
	else if (strcmp(str, DDB_PUB_PRIV)==0)
	    return(DEV_PUBLIC|DEV_PRIVATE);
	else
	    return(0);
}

/*
 *  int parse_mode(str)
 *	char   *str;
 *
 *	This function validates the input string which should represent
 *	valid device mode, and returns the equivalent bunary value of
 *	state (DEV_STATIC or DEV_DYNAMIC).
 *
 *  Arguments:
 *	char 	*str	char string representing device mode
 *
 *  Returns: int value of mode
 *	     0	if <str> is invalid mode
 *
 */
int
parse_mode (str)
	char	*str;
{
	if (strcmp(str, DDB_STATIC)==0)
	    return(DEV_STATIC);
	else if (strcmp(str, DDB_DYNAMIC)==0)
	    return(DEV_DYNAMIC);
	else
	    return(0);
}

/*
 *  int parse_uid(str, uid)
 *	char     *str;
 *	uid_t  *uid;
 *
 *	This function validates the input string <str>, to be either
 *	a valid user name or uid defined on the system. It returns
 *	the <uid> corresponding to the input string.
 *
 *  Arguments:
 *	char *str	char string representing MAC level
 *	uid_t *uid	uid value of the input ASCII string <str>
 *	
 *
 *  Returned Value:
 *	SUCCESS   - if valid MAC level, and <*level> is valid MAC LID.
 *	FAILURE  - if invalid MAC level, and <*level> is NULL.
 *
 */

int
parse_uid (str, uid)
	char	*str;
	uid_t	*uid;
{
	char *tmp;
	struct passwd	*pwent,
			*getpwnam(), *getpwuid();
	uid_t		tmp_uid;

	tmp =  (char *)malloc(strlen(str) + 1);
	/* check if valid user name */
	if (isalpha(*str) && (pwent=getpwnam(str))) {
		/* user name found in /etc/passwd */
	    	*uid = pwent->pw_uid;
	} else {
		tmp_uid=strtol(str, &tmp, 10);
		/* Make sure return value is for uid 0 and not an error */
		if ((strcmp(tmp, str) == 0) && (tmp_uid == 0))
			return(FAILURE);
	    	/* could be a uid */
	    	if (getpwuid(tmp_uid) == (struct passwd *)NULL) {
			return(FAILURE);
	    	}
	    	*uid = tmp_uid;
	}
	return(SUCCESS);
}

/*
 *  int parse_gid(str, gid)
 *	char     *str;
 *	gid_t  *gid;
 *
 *	This function validates the input string <str>, to be either
 *	a valid user name or gid defined on the system. It returns
 *	the <gid> corresponding to the input string.
 *
 *  Arguments:
 *	char *str	char string representing MAC level
 *	gid_t *gid	gid value of the input ASCII string <str>
 *	
 *
 *  Returned Value:
 *	SUCCESS   - if valid MAC level, and <*level> is valid MAC LID.
 *	FAILURE  - if invalid MAC level, and <*level> is NULL.
 *
 */

int
parse_gid (str, gid)
	char	*str;
	gid_t	*gid;
{
	struct group	*grent,
			*getgrnam(), *getgrgid();
	gid_t		tmp_gid;

	/* check if valid user name */
	if (isalpha(*str) && (grent=getgrnam(str))) {
	    /* user name found in /etc/passwd */
	    *gid = grent->gr_gid;
	} else if(tmp_gid=strtol(str, (char **)NULL, 10)) {
	    /* could be a gid */
	    if (getgrgid(tmp_gid) == (struct group *)NULL) {
		return(FAILURE);
	    }
	    *gid = tmp_gid;
	} else
	    return(FAILURE);

	return(SUCCESS);
}

/*
 *  int make_ual(str, cnt, uals, ualent)
 *	char       *str;
 *	int        cnt;
 *	ual_entry  **uals;
 *	ual_entry  *ualent;
 *
 *	This function validates the string <str>, representing the
 *	User Authorization List (ual). It verifies the following -
 *	1. Format = "uid1>y,uid2>n,uid3>n...."
 *	2. All uids are unique in the input string <str>. No uid is
 *	   repeated.
 *	3. The delimiter between uid and permission is ">".
 *	4. The permission field is either "y" or "n".
 *
 *	It returns an array of pointers, <uals>, to ual_entry's,
 *	<ualent>, with ual_entry's initialized to each uid and
 *	corresponding permission. The <uals> pointers specify
 *	the sorted order of ual_entry's.
 *
 *  Arguments:
 *	str	ptr to input caharacter string
 *	cnt	number of items in UAL list
 *	uals	array of pointers to ual_entry's
 *	ualent	ual_entry's
 *
 *  Returns:
 *	SUCCESS   - if successful, a sorted ptr to ual_entry's returned.
 *	FAILURE   - if one of the ual is not valid.
 *
 */

int
make_ual(str, cnt, uals, ualent)
	char       *str;
	int        cnt;
	ual_entry  **uals;
	ual_entry  *ualent;
{
	char		*user, *next_user;	/* ptr to uid in str    */
	char		*perm;			/* ptr to permission    */
	char		*pstr;			/* next field in str    */
	ual_entry	*pual;			/* ptr to ual_entry's   */
	uid_t		uid;
	int		i;

	/* set ptrs to start of <str> and <ualent> array */
	pstr = next_user = str;
	pual = ualent;

	/* set ual_entry[0] to NULL values */
	uals[0] = pual;
	pual->user[0] = '\0';
	pual->perm[0] = '\0';
	pual++;

	/* For each item(uid>perm pair) in input UAL string */
	for (i=1 ; i<cnt ; i++,pual++) {
	     /* extract next uid and perm */
	     if (pstr=getfield(pstr,",", &next_user)) {
		if (user=getfield(pstr,">", &perm)) {
		    /* validate uid and perm */
		    if (parse_uid(user,&uid)<0) {
			/* error, invalid user/uid */
			ddb_errmsg (SEV_ERROR, EX_ERROR, E_INUID, DDB_USERS, user);
			return(FAILURE);
		    }
		    if (!valid_perm(perm)) {
			/* error, delimiter not found */
			ddb_errmsg (SEV_ERROR, EX_ERROR, E_INPRM, DDB_USERS, perm);
			return(FAILURE);
		    }
		    /* copy uid and perm into next ual_entry */
		    sprintf(pual->user,"%d", uid);
		    sprintf(pual->perm, ">%c", *perm);
		    /* insert ual_entry into uals pointer array */
		    if (_insert_uid(i, pual, uals) < 0) {
			return(FAILURE);
		    }
		    /* process next uid>perm pair */
		    pstr = next_user;
		} else {
		    /* error, delimiter not found */
		    ddb_errmsg (SEV_ERROR, EX_ERROR, E_INDLM, DDB_USERS, pstr);
		    return(FAILURE);
		}
	    } else {
		/* error, delimiter not found */
		ddb_errmsg (SEV_ERROR, EX_ERROR, E_INDLM, DDB_USERS, pstr);
		return(FAILURE);
	    }
	}	/* end for loop */
	/* extract last user & perm */
	if (*next_user) {
	    if (user=getfield(next_user,">", &perm)) {
		/* validate uid and perm */
		if (parse_uid(user,&uid)<0) {
		    /* error, invalid user/uid */
		    ddb_errmsg (SEV_ERROR, EX_ERROR, E_INUID, DDB_USERS, user);
		    return(FAILURE);
		}
		if (!valid_perm(perm)) {
		    /* error, delimiter not found */
		    ddb_errmsg (SEV_ERROR, EX_ERROR, E_INPRM, DDB_USERS, perm);
		    return(FAILURE);
		}
		/* copy uid and perm into next ual_entry */
		sprintf(pual->user,"%d", uid);
		sprintf(pual->perm, ">%c", *perm);
		/* insert ual_entry into uals pointer array */
		if (_insert_uid(i, pual, uals) < 0) {
			return(FAILURE);
		}
	    } else {
		/* error, delimiter not found */
		ddb_errmsg (SEV_ERROR, EX_ERROR, E_INDLM, DDB_USERS, pstr);
		return(FAILURE);
	    }
	} else {
	    /* error, delimiter not found */
	    ddb_errmsg (SEV_ERROR, EX_ERROR, E_INDLM, DDB_USERS, pstr);
	    return(FAILURE);
	}
	return(SUCCESS);
}

/*
 *  void make_users(uals, cnt, strbuf)
 *	ual_entry  **uals;
 *	int        cnt;
 *	char       *strbuf;
 *
 *	This function generates an UAL string in <strbuf>, from the 
 *	ual_entry's that <uals> points to. The size of the character string
 *	buffer <strbuf>, should be the same that was passed to make_ual().
 *
 *  Arguments:
 *	uals	array of pointers to ual_entry's
 *	cnt	number of ual_entry's (or ual_entry pointers)
 *	strbuf	ptr to character string buffer returned
 *
 *  Returns: void
 *
 */

void
make_users(uals, cnt, strbuf)
	ual_entry  **uals;
	int        cnt;
	char       *strbuf;
{
	int		i;
	char		*ptr, *p;

	/* terminate start of buffer with NULL */
	*strbuf = '\0';
	ptr = strbuf;
	/* while there are ual_entry's */
	for (i=1 ; i<cnt ; i++) {
	    /* copy uid into <strbuf> */
	    p = uals[i]->user;
	    while (isspace(*p)) p++;	/* skip leading spaces */
	    strcat(ptr, p);

	    /* copy perm into <strbuf> */
	    strncat(ptr, uals[i]->perm, 2);
	    /* append ',' betn user>perm  */
	    strcat(ptr, ",");
	}
	/* copy last uid into <strbuf> */
	p = uals[i]->user;
	while (isspace(*p)) p++;	/* skip leading spaces */
	strcat(ptr, p);

	/* copy last perm into <strbuf> */
	strncat(ptr, uals[i]->perm, 2);
}

/*
 *  int parse_users(str, users)
 *	char   *str;
 *	char   **users;
 *
 *	This function validates the string <str>, representing the
 *	User Authorization List (ual). 
 *	It returns the ual string(in malloc'ed memory), with uids 
 *	sorted in ascending order by uid value, in <users>.
 *
 *  Arguments:
 *	char *str	ptr to input character string
 *	char **users	address of sorted ual list returned.
 *
 *  Returns:
 *	SUCCESS   - if successful, and <str> points to sorted ual string.
 *	FAILURE   - if one of the ual is not valid, or internal error.
 *
 */

int
parse_users(str, users)
	char   *str;
	char   **users;
{
	char		*ualbuf;	/* ualptr's array + ual entries */
	ual_entry	*ualent,	/* buff contains ual_entry's    */
			**ualptr;	/* array of ptrs to ual_entry's */
	int		cnt, strsz, buffsz, err;

	/* get lenght of <str>, and no. of  *
	 * comma separated entries in <str> */
	strsz = strlen(str) + 1;
	cnt = getlistcnt(str);

	/* allocate memory for ualptrs array + ual_entry's       *
	 * Number of entries equal (cnt+1) -- extra entry        *
	 * is required to store NULL values for sorting purposes */
	if(ualbuf=(char *)
		malloc((cnt+1)*(sizeof(ual_entry *)+sizeof(ual_entry))))  {
	    /* set ualptr's array to beginning of buffer */
	    ualptr = (ual_entry **)ualbuf;
	    /* set 1st ual_entry at end of ualptr's array */
	    ualent = (ual_entry *)(ualbuf + (cnt+1)*(sizeof(ual_entry *)));

	    /* make ordered list of ual_entry's */
	    if (make_ual(str, cnt, ualptr, ualent) < 0) {
		/* error, invalid value-items encountered */
		free(ualbuf);
		return(FAILURE);
	    }
	    /* allocate memory for sorted ual string */
	    if(*users=(char *)malloc(strsz)) {
		/* convert ordered ual_entry's into char string */
		make_users(ualptr, cnt, *users);
		free(ualbuf);
	    } else {
		/* error, ran out of memory */
		ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
		return(FAILURE);
	    }
	} else {
	    /* error, ran out of memory */
	    ddb_errmsg(SEV_ERROR, EX_INTPRB, E_NOMEM);
	    return(FAILURE);
	}
	return(SUCCESS);
}

/*
 *  int _insert_uid(n, ualent, uals)
 *	int        n;
 *	ual_entry  *ualent;
 *	ual_entry  **uals;
 *
 *	This function inserts the item number, <n>, into an array of
 *	ual_entry pointers, <uals>, based on comparing the uid field
 *	of the specified <ualent>, with the rest of the sorted entries.
 *	It uses the standard Insertion Sort algorithm.
 *
 *  Arguments:
 *	n	item number being inserted
 *	ualent	ual_entry to be inserted
 *	uals	array of pointers to ual_entry's
 *
 *  Returns: int 
 *	SUCCESS	if successful
 *	FAILURE	if there are duplicate uids
 *
 */

int
_insert_uid(n, ualent, uals)
	int        n;
	ual_entry  *ualent;
	ual_entry  **uals;
{
	ual_entry	*key;		/* entry to be inserted */
	int		i, cmp;

	key = ualent;
	i = n-1;

	while (i > 0) {
	    /* compare key->user with each user(uid) */
	    if ((cmp=strcmp(uals[i]->user, key->user)) > 0) {
		/* if array element > key, switch pointers to items */
		uals[i+1] = uals[i];
		i = i-1;
	    } else if (cmp == 0) {
		/* if array element = key, error, duplicate user(uid) */
		ddb_errmsg (SEV_ERROR, EX_USAGE, E_MULTDEF, key->user, DDB_USERS);
		return(FAILURE);
	    } else {
		/* correct position of key found, break out */
		break;
	    }
	}
	/* insert key */
	uals[i+1] = key;
	return(SUCCESS);
}
/*
 * mac_running()
 *
 *	This function resturns 
 *		TRUE	if MAC is running,
 *		FALSE	if MAC is not running
 *		FAILURE	some other MAC related error.
 */
int 
mac_running()
{
	level_t		level;

	if (lvlproc(MAC_GET, &level) < 0) {
	    if (errno==ENOPKG) {
		return(FALSE);
	    } else {
		return(FAILURE);
	    }
	} else
	    return(TRUE);
}
/*
 * _mac_installed()
 *
 *	This function returns 
 *		TRUE	if MAC is installed,
 *		FALSE	if MAC is not installed
 *		FAILURE	some other MAC related error.
 */
int 
_mac_installed()
{
	level_t		level;

	/* Using the __tabversion__ flag because in the case of release 4.0E
	 * lvlin(2) returns successfully eventhough ES is not installed.
	 */
	if (__tabversion__ == __4dot0__)
		return(FALSE);

	/* assume that SYS_PRIVATE must be defined in LTDB */
	if (lvlin(SYS_PRIVATE, &level) < 0) {
	    if (errno==EACCES) {
		return(FALSE);
	    } else {
		return(FAILURE);
	    }
	} 
	return(TRUE);
}
/* 
 * gettabversion()
 * 
 *	This function determines by looking at the 
 *	device.tab first line if the file is on 4.0 or 4ES format
 *	
 *	The function returns 
 *		__4ES__   for device.tab with 4ES's format
 *		__4dot0__ for device.tab with 4.0's format
 *		FAILURE   if a problem is encountered
 */
int
gettabversion()
{
	FILE *fp;
	char  magic[128];
	unsigned long timeval = (unsigned long) 0;
	char  *tmp, *next;

	/* open DDB_TAB file for read only */
	if ((fp = fopen(DDB_TAB, "r")) == (FILE *)NULL) {
            /* error, cannot open DDB_TAB file for read */
	    ddb_errset(errno);
	    return(FAILURE);
	}

	/* Read the first lines and check if it contains the magicno */
	if (getmagicno(fp,magic,&timeval) == FAILURE ) {
		/* error, problem encountered when reading from DDB_TAB file */
		ddb_errset(errno);
		fclose(fp);
		return(FAILURE);
	}

	fclose(fp);

	/* If first line doesn't contain the magic number,
	 * assume it is 4.0 format; otherwise, assume 4ES format.
	 */
	if (strcmp(magic,MAGICTOK) != 0)
		return(__4dot0__);
	return(__4ES__);
}
