/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmutil.c	1.10.14.2"
#ident  "$Header: tmutil.c 1.2 91/06/24 $"

# include <unistd.h>
# include <stdlib.h>
# include <stdio.h>
# include <errno.h>
# include <sys/types.h>
# include <ctype.h>
# include <string.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <sys/stropts.h>
# include <sys/termios.h>
# include <sys/sad.h>
# include <mac.h>
# include <priv.h>
# include <pfmt.h>
# include "ttymon.h"
# include <sys/termios.h>
# include <sys/stream.h>
# include <sys/tp.h>
# include <sys/secsys.h>
# include "tmstruct.h"

#define	NSTRPUSH	9	/* should agree with the tunable in	*/
				/* 		/etc/master.d/kernel	*/

extern	int	EnhancedSecurityInstalled;
extern	int	MACRunning;
extern	int	B2Running;
extern	char	Scratch[];
extern	void	log();
static	const	char
	badopen[] = ":4:Cannot open %s: %s\n",
	badstat[] = ":682:Cannot access %s: %s",
	badsak[] = ":872:\"%s\" is an invalid SAK definition for SAK type '%c'",
	needfullpath[] = ":684:Must specify full path name for \"%s\".";

extern	int	devstat();

/*
 * Procedure:	  check_device
 *
 * Restrictions:
                 access(2): none
                 stat(2): none
                 strerror: none
*/

/*
 *	check_device - check to see if the device exists,
 *		     - and if it is a character device
 *		     - return 0 if everything is ok. Otherwise, return -1
 */

check_device(device)
char	*device;
{
	struct stat statbuf;

	if ((device == NULL) || (*device == '\0')) {
		log(MM_ERROR, ":683:Device field is missing");
		return(-1);
	}
	if (*device != '/') {
		log(MM_ERROR, needfullpath, device);
		return(-1);
	}
	if (access(device, 0) == 0) {
		if (stat(device,&statbuf) < 0) {
			log(MM_ERROR, badstat, device, strerror(errno));
			return(-1);
		}
		if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
			log(MM_ERROR, ":685:\"%s\" not character special device",device);
			return(-1);
		}
	}
	else {
		log(MM_ERROR, ":686:Device \"%s\" does not exist",device);
		return(-1);
	}
	return(0);
}

/*
 * Procedure:	  check_cmd
 *
 * Restrictions:
                 access(2):P_MACREAD
                 stat(2):P_MACREAD
                 strerror: none
*/

/*
 *	check_cmd - check to see if the cmd file exists,
 *		  - and if it is executable
 *		  - return 0 if everything is ok. Otherwise, return -1
 */

check_cmd(cmd)
char	*cmd;
{
	struct stat statbuf;
	char	tbuf[BUFSIZ];
	char	*tp = tbuf;

	if ((cmd == NULL) || (*cmd == '\0')) {
		log(MM_ERROR, ":687:Server command is missing");
		return(-1);
	}
	(void)strcpy(tp,cmd);
	(void)strtok(tp, " \t");
	if (*tp != '/') {
		log(MM_ERROR, needfullpath, tp);
		return(-1);
	}
	(void) procprivl(CLRPRV, MACREAD_W, 0);
	if (access(tp, 0) == 0) {
		if (stat(tp,&statbuf) < 0) {
			(void) procprivl(SETPRV, MACREAD_W, 0);
			log(MM_ERROR, badstat, tp, strerror(errno));
			return(-1);
		}
		(void) procprivl(SETPRV, MACREAD_W, 0);
		if (!(statbuf.st_mode & 0111)) {
			log(MM_ERROR, ":688:\"%s\" not executable\n",tp);
			return(-1);
		}
		if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
			log(MM_ERROR, ":689:\"%s\" not a regular file",tp);
			return(-1);
		}
	}
	else {
		(void) procprivl(SETPRV, MACREAD_W, 0);
		log(MM_ERROR, ":690:\"%s\" does not exist",tp);
		return(-1);
	}
	return(0);
}

/*
 * strcheck(sp, flag)	- check string
 *				- if flag == ALNUM, all char. are expected to
 *				  be alphanumeric
 *				- if flag == NUM, all char. are expected to
 *				  be digits and the number must be >= 0
 *				- return 0 if successful, -1 if failed.
 */
int
strcheck(sp, flag)
char	*sp;		/* string ptr		*/
int	flag;		/* either NUM or ALNUM	*/
{
	register	char	*cp;
	if (flag == NUM) {
		for (cp = sp; *cp; cp++) {
			if (!isdigit(*cp)) {
				return(-1);
			}
		}
	}
	else {	/* (flag == ALNUM) */ 
		for (cp = sp; *cp; cp++) {
			if (!isalnum(*cp)) {
				return(-1);
			}
		}
	}
	return(0);
}

/*
 * Procedure:	  vml
 *
 * Restrictions:
                 open(2): none
             	 strerror: none
                 ioctl(2): none
*/

/*
 * vml(modules)	- validate a list of modules
 *		- return 0 if successful, -1 if failed
 */
int
vml(modules)
char	*modules;
{
	char	buf[BUFSIZ];
	char	*modp = buf;
	int	i, fd;
	struct str_mlist newmods[NSTRPUSH];	/* modlist for newlist	*/
	struct str_list	newlist;		/* modules to be pushed	*/

	if ((modules == NULL) || (*modules == '\0'))
		return(0);

	newlist.sl_modlist = newmods;
	newlist.sl_nmods = NSTRPUSH;
	(void)strcpy(modp, modules);
	/*
	 * pull mod names out of comma-separated list
	 */
	for ( i = 0, modp = strtok(modp, ",");
	modp != NULL; i++, modp = strtok(NULL, ",") ) {
		if ( i >= NSTRPUSH) {
			log(MM_ERROR, ":691:Too many modules in <%s>", modules);
			return(-1);
		}
		(void)strncpy(newlist.sl_modlist[i].l_name,
					modp, FMNAMESZ);
	}
	newlist.sl_nmods = i;

	/*
	 * Is it a valid list of modules?
	 */
	if ((fd = open(USERDEV, O_RDWR)) == -1) {
		if (errno == EBUSY) {
			log(MM_WARNING, ":692:Cannot validate module list, /dev/sad/user busy");
			return(0);
		}
		log(MM_ERROR, badopen, USERDEV, strerror(errno));
		return(-1);
	}
	if ( (i = ioctl(fd, SAD_VML, &newlist)) < 0 ) {
		log(MM_ERROR, ":693:Validate modules ioctl failed, modules = <%s>: %s", 
			modules, strerror(errno));
		(void)close(fd);
		return(-1);
	}
	if ( i != 0 ) {
		log(MM_ERROR, ":694:Invalid STREAMS module list <%s>.", modules);
		(void)close(fd);
		return(-1);
	}
	(void)close(fd);
	return(0);
}

/*
 * copystr(s1, s2) - copy string s2 to string s1
 *		   - also put '\' in front of ':'
 */
void
copystr(s1,s2)
char	*s1, *s2;
{
	while (*s2) {
		if (*s2 == ':') {
			*s1++ = '\\';
		}
		*s1++ = *s2++;
	}
	*s1 = '\0';
}

/*
 *	lastname	- If the path name starts with "/dev/",
 *			  return the rest of the string.
 *			- Otherwise, return the last token of the path name
 */
char	*
lastname(name)
char	*name;
{
	char	*sp, *p;
	sp = name;
	if (strncmp(sp, "/dev/", 5) == 0)
		sp += 5;
	else
		while ((p = (char *)strchr(sp,'/')) != (char *)NULL) {
			sp = ++p;
		}
	return(sp);
}


/*
 * Procedure:	  check_session
 *
 * Restrictions:
		 ioctl(2): none
*/

/*
 * check_session(fd) - check if a session established on fd
 *		       return 1 if session exists, otherwise, return 0.
 *
 */
int
check_session(fd)
int	fd;
{
	pid_t	sid;
	int     ret;

	ret=ioctl(fd, TIOCGSID, &sid);
	if (ret == -1)
		return(0);
	else if (sid == 0)
		return(0);
	else
		return(1);
}



/* is_macrunning()
**
** -returns TRUE if MAC is running on the system.
** -returns FALSE if MAC is not running.
*/
int
is_macrunning()
{
 	lvlproc(MAC_GET, (level_t *)NULL);
	if (!( (errno == ENOSYS) || (errno == ENOPKG))){
		return (TRUE);
	}else{
		return (FALSE);
	}
}


/* is_b2running()
**
** -returns TRUE if B2 enhanced security is running
** -returns FALSE if B2 enhanced security in not running
**
** -A B2 system is considered running if MAC and LPM is running
*/
int
is_b2running()
{
	if ((is_macrunning() == TRUE)  && (secsys(ES_PRVID, 0) < (uid_t)0)){
		return (TRUE);
	}else{
		return (FALSE);
	}
}


/*
 * Procedure:	  is_enhancedsecurityinstalled()
 *
 * Restrictions:
		 lvlin: none
*/

/* is_enhancedsecurityinstalled()
** 
** -returns TRUE if enhanced security is installed
** -returns FALSE if enhanced security in not installed
*/
int
is_enhancedsecurityinstalled()
{
	level_t	level;

 	if (lvlin("SYS_PRIVATE", &level) == 0){
		return (TRUE);
	}else{
		return (FALSE);
	}
}




/* set_saktype(externalsaktype, internalsaktypep)
**
** -sets the internal representation of a sak type given the external
**  (administrative/_pmtab database) representation.  The internal 
**  representation is set in the return parameter internalsaktypep.
**
** IF B2 enhanced security is not running
**	-internal representation of a sak is set to saktypeNONE
**
** -NOTES:
**	-returns -1 if external is not recognized and sets internalsaktypep
**	 to saktypeUNDEF
**	-returns 0 if external value is NULL and sets interanlsaktypep to
**	 saktypeUNDEF
**	-otherwise returns 0 and sets internalsaktypep to the appropriate
**	 type
*/

int
set_saktype(externalsaktype, internalsaktypep)
	char		*externalsaktype;
	enum saktype	*internalsaktypep;
{
	size_t length;
	int ret = 0;

	*internalsaktypep = saktypeUNDEF;

	if (B2Running == FALSE)
		*internalsaktypep = saktypeNONE;
	else if ((externalsaktype == (char *)NULL)||(*externalsaktype == '\0'))
		ret = 0;
	else if ((length = strlen(externalsaktype)) == 0)
		ret = 0;
	else if (length != 1)
		ret = -1;
	else{
		switch (*externalsaktype){

		case 'l':	/* line condition */
			*internalsaktypep = saktypeLINECOND;
			break;
		case 'c':	/* character */
			*internalsaktypep = saktypeCHAR;
			break;
		case 'n':	/* none */
			*internalsaktypep = saktypeNONE;
			break;
		case 'x':	/* undefined */
			break;
		default:
			ret = -1;
			break;
		}
	}
	return (ret);
}


/* set_sakdef(externalsak, sakp)
**
** IF B2 enhanced security is running
**	-sets internal representation of sak in sak structure given external
**	 (administrative/_pmtab database) sak representation and the internal
**	 sak type representation
**
** -NOTES:
**	-returns -1 if sak type is not valid or external sak representation is
**	 not valid
**	-otherwise returns 0 and sets sak structure (the return argument
**	 parameter) with the internal sak representation 
*/

/* -this maps external sak representations of the from ^char (where char is a
**  printable ascii character) to internal sak representation.
** -there is a mapping entry for each ascii character. an external 
**  representation value of 0 indicates no valid mapping exists for the given
**  ascii character.
** -the map is defined such that it can be indexed by ascii character to
**  obtain mapping information
*/

struct sak_extrnal_intrnal_map{
	ulong sakeimap_extrnal;
	ulong sakeimap_intrnal;
};
static struct sak_extrnal_intrnal_map sakeimap[]= {

0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,				/* 000 - 007 */
0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,				/* 010 - 017 */
0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,				/* 020 - 027 */
0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,				/* 030 - 037 */
0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,				/* 040 - 047 */
0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, '/',037,			/* 050 - 057 */
0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,				/* 060 - 067 */
0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, '?',037,			/* 070 - 077 */
'@',000, 'A',001, 'B',002, 'C',003, 'D',004, 'E',005, 'F',006, 'G',007, /* 0100 - 0107*/
'H',010, 'I',011, 'J',012, 'K',013, 'L',014, 'M',015, 'N',0, 'O',0, /* 0110 - 0117*/
'P',020, 'Q',021, 'R',022, 'S',023, 'T',024, 'U',025, 'V',026, 'W',027, /* 0120 - 0127*/
'X',030, 'Y',031, 'Z',032, '[',033, '\\',034, ']',035, '^',036, '_',037, /* 0130 - 0137*/
'`',000, 'a',001, 'b',002, 'c',003, 'd',004, 'e',005, 'f',006, 'g',007, /* 0140 - 0147*/
'h',010, 'i',011, 'j',012, 'k',013, 'l',014, 'm',015, 'n',0, 'o',0, /* 0150 - 0157*/
'p',020, 'q',021, 'r',022, 's',023, 't',024, 'u',025, 'w',026, 'x',027, /* 0160 - 0167*/
'x',030, 'y',031, 'z',032, '{',033, '|',034, '}',035, '~',036, 0,0	/* 0170 - 0177*/
};

int
set_sakdef(externalsak, sakp)
	char		*externalsak;
	struct sak	*sakp;
{
	ulong charsak;
	int ret = 0;

	if (B2Running == TRUE){

		switch(sakp->sak_type){

		case saktypeUNDEF:
		case saktypeNONE:
			break;
		case saktypeLINECOND:

			if ((externalsak == (char *)NULL) ||
			 (*externalsak == '\0')){
				ret = -1;
			}
			else if (strcmp(externalsak, "drop") == 0)
				sakp->sak_linecond = saklinecondLINEDROP;
			else if (strcmp(externalsak, "break") == 0)
				sakp->sak_linecond = saklinecondBREAK;
			else
				ret = -1;
			break;

		case saktypeCHAR:

			if ((externalsak == (char *)NULL) ||
			 (*externalsak == '\0')){
				ret = -1;
			}
			else if (*externalsak == '0'){
				char *strpp = (char *)NULL;

				charsak = strtoul(externalsak, &strpp, 0);
				if ((*strpp != '\0') || (charsak & ~037) ||
				 (charsak == 016) || (charsak == 017))
					ret = -1;
				else
					sakp->sak_char = charsak;
			}else if (*externalsak == '^'){
				if (strlen(externalsak) != 2)
					ret = -1;
				else if (*externalsak & (char)0200)
					ret = -1; /* not mapping extended ascii */
				else if (sakeimap[externalsak[1]].sakeimap_intrnal == 0)
					ret = -1;
				else
					sakp->sak_char =
					 sakeimap[externalsak[1]].sakeimap_intrnal;
			}else
				ret = -1;
			break;

		default:
			ret = -1;
			break;
		}
	}

	return (ret);
}


/* set_saksec(externalsec, sakp)
**
** IF B2 enhanced security is running
**	-sets internal representation of whether a secondary sak is defined or
**	 not, given the external (administrative/_pmtab database) represenation
** -NOTE:
**	-returns -1 if external representation is invalid
**	-returns 0 if external representation is valid and
**		--sets internal representation to saksecNO if external
**		  representation
**		  is  NULL
**			OR
**		--sets internal representation to saksecYES if external
**		  representation
**		  is "drop"
*/

int
set_saksec(externalsec, sakp)
	char		*externalsec;
	struct sak	*sakp;
{
	int ret = 0;
	sakp->sak_secondary = saksecNO;
	
	if ((B2Running == TRUE) &&
	 ((externalsec != (char *)NULL) && (*externalsec != '\0'))){
		if (strcmp(externalsec, "drop") == 0)
			sakp->sak_secondary = saksecYES;
		else
			ret = -1;
	}
	return (ret);
}




/* check_sak(saktypep, sakdefp, saksecp)
**
** -verifies the command line (from ttymon in getty mode or ttyadm)
**  specification of the SAK
**
** -NOTE: an argument value that is NULL is interpreted to mean that its
**  associated command line option was not specified
**
** -returns 0 if check passes
** IF check fails
**	-returns -1 if the argument is not valid
**	-returns  1 if there is a usage error
**
** -verification is done in the following manner
**
**	SWITCH (type of sak, saktypep)
**
**	CASE 'n':	 sak type defined as NONE 
**		IF a SAK is defined OR a secondary SAK indicate
**			-check fails  usage error
**
**	CASE 'l':	 sak type defined as a line condition
**		IF SAK != "break" OR "drop"
**			-check fails  argument not valid
**
**	CASE 'c':	 sak type defined as ascii control character
**		IF SAK != 000 - 037 OR != ^CHAR (where CHAR is a printable ascii
**		 character that can form ascii control characters when
**		 simultaneously entered with control key) 
**			-check fails  argument not valid
**
**	CASE 'x':	 sak type undefined
**		IF a SAK is defined OR a secondary SAK indicate
**			-check fails  usage error
*/
int
check_sak(saktypep, sakdefp, saksecp)
	char	*saktypep;
	char	*sakdefp;
	char	*saksecp;
{

	int	ret = 0;


	/* IF saktypep is NULL
	**	-set it to "x" to make switch statement work
	*/
	if ((saktypep == (char *)NULL) || (*saktypep == '\0'))
		saktypep = "x";

	if (strlen(saktypep) != 1)
		ret = 1;	/* usage error */
	else{
		switch (*saktypep){

		case 'n':	/* sak type defined as NONE */
			if (!((sakdefp == (char *)NULL) || (*sakdefp == '\0'))
			 || !((saksecp == (char *)NULL) || (*saksecp == '\0')))
				ret = 1;	/* usage error */
			break;

		case 'l':	/* sak type defined as line condition */
			if ((sakdefp == (char *)NULL) || (*sakdefp == '\0'))
				ret = 1;	/* usage error */
			else if ((strcmp(sakdefp, "break") != 0) &&
			 (strcmp(sakdefp, "drop") != 0)){
				ret = -1;
				log(MM_ERROR, badsak, sakdefp, 'l');
			}
			break;

		case 'c':	/* sak type defined as ascii control character*/
			if ((sakdefp == (char *)NULL) || (*sakdefp == '\0'))
				ret = 1;	/* usage error */
			else{
				if (*sakdefp == '0'){
					ulong charsak;
					char *strpp = (char *)NULL;

					charsak = strtoul(sakdefp, &strpp, 0);
					if ((*strpp != '\0') ||
					 (charsak & ~037) || (charsak == 016) ||
					 (charsak == 017))
						ret = -1;
				}
				else if (*sakdefp == '^'){
					if (strlen(sakdefp) != 2)
						ret = -1;
					else if (*sakdefp & (char)0200)
						/* not mapping extended ascii */
						ret = -1;
					else if (sakeimap[sakdefp[1]].sakeimap_intrnal == 0)
						ret = -1;
				}
				else
					ret = -1;
			}

			if (ret == -1){
				log(MM_ERROR, badsak, sakdefp, 'c');
			}
			break;

		case 'x':	/* sak type undefined */
			if (!((sakdefp == (char *)NULL) || (*sakdefp == '\0'))
			 || !((saksecp == (char *)NULL) || (*saksecp == '\0')))
				ret = 1;	/* usage error */
			break;

		default:
			ret = -1;
			log(MM_ERROR,
			 ":873:\"%s\" is an invalid SAK type", saktypep);
			break;
		}
	}
	return (ret);
}
