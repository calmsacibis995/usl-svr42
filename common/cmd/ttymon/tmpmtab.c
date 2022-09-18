/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmpmtab.c	1.14.16.2"
#ident  "$Header: tmpmtab.c 1.2 91/06/24 $"

#include	<unistd.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<ctype.h>
#include	<string.h>
#include 	<pwd.h>
#include 	<grp.h>
#include	<signal.h>
#include	<pfmt.h>
#include	<priv.h>
#include	<deflt.h>
#include	"ttymon.h"
#include	<sys/termios.h>
#include	<sys/stream.h>
#include	<sys/tp.h>
#include	"tmstruct.h"
#include	"tmextern.h"

extern	char	*strsave();
extern	int	vml();
extern	int	set_saktype();
extern	int	set_sakdef();
extern	int	set_saksec();
extern	int	strcheck();

extern	const char badopen[], nomem[], entrytoolong[];
extern	int	B2Running;


	void	read_pmtab();
static	int	get_flags();
static	int	get_ttyflags();
	char	*pflags();
	char	*pttyflags();
	void	dump_pmtab();
static	int	same_entry();
static	void	insert_pmtab();
	void	purge();
static	void	free_pmtab();
static	int	check_pmtab();
	int	check_identity();
static	char	*expand();



/*
 * Procedure:	  read_pmtab
 *
 * Restrictions:
                 fopen: none
           	 strerror: none
                 check_version: none
                 sprintf: none
                 fclose: none
*/

/*
 * read_pmtab() 
 *	- read and parse pmtab 
 *	- store table in linked list pointed by global variable "PMtab"
 *	- exit if file does not exist or error detected.
 */
void
read_pmtab()
{
	register struct pmtab *gptr;
	register char *ptr, *wptr;
	FILE 	 *fp;
	int 	 input,state,size,rawc,field;
	char 	 oldc;
	char 	 line[BUFSIZ];
	char 	 wbuf[BUFSIZ];
	static 	 char *states[] = {
	      "","tag","flags","identity","reserved1","reserved2","iascheme",
	      "device","ttyflags","count","service", "timeout","ttylabel",
	      "modules","prompt","disable msg","saktype","sak","saksec"
	};

# ifdef DEBUG
	debug("in read_pmtab");
# endif

	if ((fp = fopen(PMTABFILE,"r")) == NULL) {
		logexit(1, badopen, PMTABFILE, strerror(errno));
	}

	Nentries = 0;
	if (check_version(PMTAB_VERS, PMTABFILE) != 0)
		logexit(1,":641:Check pmtab version failed");

	for (gptr = PMtab; gptr; gptr = gptr->p_next) {
		if (gptr->p_status == SESSION) {
			if (gptr->p_fd > 0) {
				(void)close(gptr->p_fd);
				gptr->p_fd = 0;
			}
			gptr->p_inservice = gptr->p_status;
		}
		gptr->p_status = NOTVALID;
	}

	wptr = wbuf;
	input = ACTIVE;
	do {
		line[0] = '\0';
		for (ptr= line,oldc = '\0'; ptr < &line[sizeof(line)-1] &&
		 (rawc=getc(fp))!= '\n' && rawc != EOF; ptr++,oldc=(char)rawc){
			if ((rawc == '#') && (oldc != '\\'))
				break;
			*ptr = (char)rawc;
		}
		*ptr = '\0';

		/* skip rest of the line */
		if (rawc != EOF && rawc != '\n') {
			if (rawc != '#') 
				log(MM_ERROR, entrytoolong);
			while ((rawc = getc(fp)) != EOF && rawc != '\n') 
				;
		}

		if (rawc == EOF) {
			if (ptr == line) break;
			else input = FINISHED;
		}

		/* if empty line, skip */
		for (ptr=line; *ptr != '\0' && isspace(*ptr); ptr++)
			;
		if (*ptr == '\0') continue;

#ifdef DEBUG
		(void)sprintf(Scratch,"**** Next Entry ****\n%s",line);
		debug(Scratch);
#endif

		/* Now we have the complete line */

		if ((gptr = ALLOC_PMTAB) == PNULL)
			logexit(1, nomem, strerror(errno));

		/* set hangup flag, this is the default */
		gptr->p_ttyflags |= H_FLAG;

		for (state=P_TAG,ptr=line;state !=FAILURE && state !=SUCCESS;) {
			switch(state) {
			case P_TAG:
				gptr->p_tag = strsave(getword(ptr,&size,0));
				break;
			case P_FLAGS:
				(void)strcpy(wptr, getword(ptr,&size,0));
				if ((get_flags(wptr, &gptr->p_flags)) != 0) {
					field = state;
					state = FAILURE;
				}
				break;
			case P_IDENTITY:
				gptr->p_identity=strsave(getword(ptr,&size,0));
				break;
			case P_RES1:
				gptr->p_res1=strsave(getword(ptr,&size,0));
				break;
			case P_RES2:
				gptr->p_res2=strsave(getword(ptr,&size,0));
				break;
			case P_IASCHEME:
				gptr->p_iascheme=strsave(getword(ptr,&size,1));
				break;
			case P_DEVICE:
				gptr->p_realdevice =
				strsave(getword(ptr,&size,0));
				break;
			case P_TTYFLAGS:
				(void)strcpy(wptr, getword(ptr,&size,0));
				if ((get_ttyflags(wptr,&gptr->p_ttyflags))!=0) {
					field = state;
					state = FAILURE;
				}
				break;
			case P_COUNT:
				(void)strcpy(wptr, getword(ptr,&size,0));
				if (strcheck(wptr, NUM) != 0) {
					log(MM_ERROR, ":644:wait_read() count must be a positive number"); 
					field = state;
					state = FAILURE;
				}
				else
				    gptr->p_count = atoi(wptr);
				break;
			case P_SERVER:
				gptr->p_server = 
				strsave(expand(getword(ptr,&size,1), 
					gptr->p_device));
				break;
			case P_TIMEOUT:
				(void)strcpy(wptr, getword(ptr,&size,0));
				if (strcheck(wptr, NUM) != 0) {
					log(MM_ERROR, ":645:Timeout value must be a positive number"); 
					field = state;
					state = FAILURE;
				}
				else
				    gptr->p_timeout = atoi(wptr);

				break;
			case P_TTYLABEL:
				gptr->p_ttylabel=strsave(getword(ptr,&size,0));
				break;
			case P_MODULES:
				gptr->p_modules = strsave(getword(ptr,&size,0));
				if (vml(gptr->p_modules) != 0) {
					field = state;
					state = FAILURE;
				}
				break;
			case P_PROMPT:
				gptr->p_prompt = strsave(getword(ptr,&size,TRUE));
				break;
			case P_DMSG:
				gptr->p_dmsg = strsave(getword(ptr,&size,TRUE));
				break;
			/* -NOTE:
			**
			** IF enhanced security is not running
			**	-set_saktype()
			**		-will always succeed
			**		-the p_sak.sak_type will be set to
			**		 saktypeNONE
			**	-set_sakdef() AND set_saksec()
			**		-will always succeed
			*/
			case P_SAKTYPE:
				(void)strcpy(wptr, getword(ptr,&size,0));
				if (set_saktype(wptr, &gptr->p_sak.sak_type) ==
				-1){
					field = state;
					state = FAILURE;
				}
				break;
			case P_SAKDEF:
				(void)strcpy(wptr, getword(ptr, &size,0));
				if (set_sakdef(wptr, &gptr->p_sak) == -1){
					field = state;
					state = FAILURE;
				}
				break;
			case P_SAKSEC:
				(void)strcpy(wptr, getword(ptr, &size,0));
				if (set_saksec(wptr, &gptr->p_sak) == -1){
					field = state;
					state = FAILURE;
				}
				break;
			} /* end switch */
			ptr += size;
			if (state == FAILURE) 
				break;
			if (state == P_SAKSEC){
				if ((*ptr == '\0') || (*ptr == ':')) {
					state = SUCCESS;
				}
				else {
					field = state;
					state = FAILURE;
				}
			}
			else {
				if (*ptr != ':') {
					field = state;
					state = FAILURE;
				} else {
					ptr++;	/* Skip the ':' */
					state++ ;
				}
			}
		} /* end for loop */

		if (state == SUCCESS) {
			/*
			 * If B2 enhanced security is running, login timeout
			 * value is zero, and SAK is not NONE, get the login
			 * timeout value, TIMEOUT from the login /etc/default
			 * file.  If TIMEOUT is not defined use a hard coded
			 * default login timeout, DEFLT_LOGIN_TIMEOUT.
			 */
			if ((B2Running == TRUE) &&
			 (gptr->p_timeout == 0) &&
			 (gptr->p_sak.sak_type != saktypeNONE)){
				FILE	*loginfp;
				char	*linep;
				if ((loginfp = defopen(DEFLT_LOGIN_FILE)) ==
				 (FILE *)NULL){
					log(MM_ERROR,
					 ":902:Cannot open %s/%s\n",DEFLT,
					 DEFLT_LOGIN_FILE);
					gptr->p_timeout = DEFLT_LOGIN_TIMEOUT;
				}else if ((linep = defread(loginfp, "TIMEOUT")) ==
				 (char *)NULL){
					log(MM_ERROR,
					 ":903:%s not found in %s/%s\n",
					 "TIMEOUT", DEFLT, DEFLT_LOGIN_FILE);
					gptr->p_timeout = DEFLT_LOGIN_TIMEOUT;
					(void)defclose(loginfp);
				}else{
					gptr->p_timeout = atoi(linep);
					(void)defclose(loginfp);
				}
			}

			if (check_pmtab(gptr) == 0) {
				if (Nentries < Maxfds) 
					insert_pmtab(gptr);
				else {
					log(MM_ERROR,
			":646:Cannot add more entries to pmtab, Maxfds = %d", Maxfds);
					free_pmtab(gptr);
					(void)fclose(fp);
					return;
				}
			}
			else {
				log(MM_ERROR, ":647:Parsing failure for entry: \n%s",line);
			log(MM_NOGET|MM_NOSTD, "-------------------------------------------");
				free_pmtab(gptr);
			}
		} else {
			*++ptr = '\0';
			log(MM_ERROR, ":648:Parsing failure in the \"%s\" field,\n%s<--error detected here", states[field],line);
			log(MM_NOGET|MM_NOSTD, "-------------------------------------------");
			free_pmtab(gptr);
		}
	} while (input == ACTIVE);

	(void)fclose(fp);
	return;
}

/*
 * get_flags	- scan flags field to set U_FLAG and X_FLAG
 */
static	int
get_flags(wptr, flags)
char	*wptr;		/* pointer to the input string	*/
long *flags;		/* pointer to the flag to set	*/
{
	register char	*p;
	for (p = wptr; *p; p++) {
		switch (*p) {
		case 'x':
			*flags |= X_FLAG;
			break;
		case 'u':
			*flags |= U_FLAG;
			break;
		default:
			log(MM_ERROR, ":649:Invalid flag -- %c", *p);
			return(-1);
		} 
	}
	return(0);
}

/*
 * get_ttyflags	- scan ttyflags field to set corresponding flags
 */
static	int
get_ttyflags(wptr, ttyflags)
char	*wptr;		/* pointer to the input string	*/
long 	*ttyflags;	/* pointer to the flag to be set*/
{
	register char	*p;
	for (p = wptr; *p; p++) {
		switch (*p) {
		case 'c':
			*ttyflags |= C_FLAG;
			break;
		case 'h': /* h means don't hangup */
			*ttyflags &= ~H_FLAG;
			break;
		case 'b':
			*ttyflags |= B_FLAG;
			*ttyflags |= R_FLAG;
			break;
		case 'r':
			*ttyflags |= R_FLAG;
			break;
		default:
			log(MM_ERROR, ":650:Invalid ttyflag -- %c", *p);
			return(-1);
		} 
	}
	return(0);
}

# ifdef DEBUG
/*
 * pflags - put service flags into intelligible form for output
 */

char *
pflags(flags)
long flags;	/* binary representation of the flags */
{
	register int i;			/* scratch counter */
	static char buf[BUFSIZ];	/* formatted flags */

	if (flags == 0)
		return("-");
	i = 0;
	if (flags & U_FLAG) {
		buf[i++] = 'u';
		flags &= ~U_FLAG;
	}
	if (flags & X_FLAG) {
		buf[i++] = 'x';
		flags &= ~X_FLAG;
	}
	if (flags)
		dlog("Internal error in pflags");
	buf[i] = '\0';
	return(buf);
}

/*
 * pttyflags - put ttyflags into intelligible form for output
 */

char *
pttyflags(flags)
long flags;	/* binary representation of ttyflags */
{
	register int i;			/* scratch counter */
	static char buf[BUFSIZ];	/* formatted flags */

	if (flags == 0)
		return("h");
	i = 0;
	if (flags & C_FLAG) {
		buf[i++] = 'c';
		flags &= ~C_FLAG;
	}
	if (flags & H_FLAG) 
		flags &= ~H_FLAG;
	else
		buf[i++] = 'h';
	if (flags & B_FLAG) {
		buf[i++] = 'b';
		flags &= ~B_FLAG;
	}
	if (flags & R_FLAG) {
		buf[i++] = 'r';
		flags &= ~B_FLAG;
	}
	if (flags)
		dlog("Internal error in p_ttyflags");
	buf[i] = '\0';
	return(buf);
}

void
dump_pmtab()
{
	struct	pmtab *gptr;

	debug("in dump_pmtab");
	dlog("********** dumping pmtab **********");
	dlog(" ");
	for (gptr=PMtab; gptr; gptr = gptr->p_next) {
		dlog("-------------------------------------------");
		dlog("tag:\t\t%s",gptr->p_tag);
		dlog("flags:\t\t%s",pflags(gptr->p_flags));
		dlog("identity:\t%s",gptr->p_identity);
		dlog("reserved1:\t%s",gptr->p_res1);
		dlog("reserved2:\t%s",gptr->p_res2);
		dlog("iascheme:\t%s",gptr->p_iascheme);
		if (gptr->p_device != (char *)NULL)
			dlog("device:\t%s",gptr->p_device);
		dlog("realdevice:\t%s",gptr->p_realdevice);
		dlog("ttyflags:\t%s",pttyflags(gptr->p_ttyflags));
		dlog("count:\t\t%d",gptr->p_count);
		dlog("server:\t%s",gptr->p_server);
		dlog("timeout:\t%d",gptr->p_timeout);
		dlog("ttylabel:\t%s",gptr->p_ttylabel);
		dlog("modules:\t%s",gptr->p_modules);
		dlog("prompt:\t%s",gptr->p_prompt);
		dlog("disable msg:\t%s",gptr->p_dmsg);
		dlog("status:\t\t%d",gptr->p_status);
		dlog("tpstatus:\t\t%d",gptr->p_tpstatus);
		dlog("inservice:\t%d",gptr->p_inservice);
		dlog("tpctrlfd:\t\t%d",gptr->p_tpctrlfd);
		dlog("fd:\t\t%d",gptr->p_fd);
		dlog("tpdataconnid:\t\t%d",gptr->p_tpdataconnid);
		dlog("reason\t\t%ld",gptr->p_reason);
		dlog("pid:\t\t%ld",gptr->p_pid);
		dlog("uid:\t\t%ld",gptr->p_uid);
		dlog("gid:\t\t%ld",gptr->p_gid);
		dlog("dir:\t%s",gptr->p_dir);
		dlog(" ");
	}
	dlog("********** end dumping pmtab **********");
}
# endif

/*
 * same_entry(e1,e2,reasonp)	-compare 2 entries of pmtab
 *				 if the fields are different, copy e2 to e1
 *				 return 1 if same, return 0 if different
 *				-if fields are different reasponp will indicate
 *				 the reason(s) for the difference
 */
static	int
same_entry(e1,e2,reasonp)
	struct	pmtab	*e1,*e2;
	ulong *reasonp;
{
 	int reason = 0;
	int ret = 1;


	if (strcmp(e1->p_identity, e2->p_identity) != 0){
		ret = 0;
		reason |= REASidentity;
	}
        if (strcmp(e1->p_res1, e2->p_res1) != 0){
		ret = 0;
		reason |= REASres1;
	}
	if (strcmp(e1->p_res2, e2->p_res2) != 0){
		ret = 0;
		reason |= REASres2;
	}
	if (strcmp(e1->p_iascheme, e2->p_iascheme) != 0){
		ret = 0;
		reason |= REASiascheme;
	}
	if (strcmp(e1->p_realdevice, e2->p_realdevice) != 0){
		ret = 0;
		reason |= REASrealdevice;
	}
	if (strcmp(e1->p_server, e2->p_server) != 0){
		ret = 0;
		reason |= REASserver;
	}
	if (strcmp(e1->p_ttylabel, e2->p_ttylabel) != 0){
		ret = 0;
		reason |= REASttylabel;
	}
	if (strcmp(e1->p_modules, e2->p_modules) != 0){
		ret = 0;
		reason |= REASmodules;
	}
	if (strcmp(e1->p_prompt, e2->p_prompt) != 0){
		ret = 0;
		reason |= REASprompt;
	}
	if (strcmp(e1->p_dmsg, e2->p_dmsg) != 0){
		ret = 0;
		reason |= REASdmsg;
	}
	if (e1->p_sak.sak_type != e2->p_sak.sak_type){
		if (e1->p_sak.sak_type == saktypeUNDEF)
			/* indicates a default sak has been defined when there
			** was not one defined before
			*/
			reason |= REASsakdef;
		else if (e2->p_sak.sak_type == saktypeUNDEF)
			/* indicates a default sak has not been defined when
			** there was one defined before
			*/
			reason |= REASsakundef;
		else
			/* since both the current and new pmtab entry are both
			** not == saktypeUNDEF, the reason for the difference
			** is that the default sak definition has changed
			*/
			reason |= REASsakchg;
		ret = 0;
	}else{	/* -sak_type are the same
		** -need to compare the SAK Representative if sak_type is
		**  saktypeCHAR or saktypeLINECOND
		** -need to compare if secondary SAK representative has changed
		*/
		/* -as long as sak_type for e2 is saktypeUNDEF
		**  mark reason as REASsakundef (even though e1 sak_type
		**  may be the same.  Want to save this state every time
		**  because invalidly marked PMtab entries are not purged until
		**  an end of session is indicated (i.e. Hangup, SAK,
		**  death of child)
		*/
		if (e2->p_sak.sak_type == saktypeUNDEF){
			reason |= REASsakundef;
			ret = 0;
		}else if ((e1->p_sak.sak_type == saktypeCHAR) &&
		 (e2->p_sak.sak_type == saktypeCHAR)){
			if (e1->p_sak.sak_char != e2->p_sak.sak_char){
				reason |= REASsakchg;
				ret = 0;
			}
		}else if ((e1->p_sak.sak_type == saktypeLINECOND) &&
		 (e2->p_sak.sak_type == saktypeLINECOND)){
			if (e1->p_sak.sak_linecond != e2->p_sak.sak_linecond){
				reason |= REASsakchg;
				ret = 0;
			}
		}
		if (e1->p_sak.sak_secondary != e2->p_sak.sak_secondary){
			reason |= REASsakchg;
			ret = 0;
		}
	}
	if (e1->p_flags != e2->p_flags){
		ret = 0;
		reason |= REASflags;
		if ((e1->p_flags & U_FLAG) != (e2->p_flags & U_FLAG)){
			reason |= (e2->p_flags & U_FLAG ? REASadduflag:REASdeluflag);
		if (e2->p_flags & X_FLAG)
			reason |= REASxflag;

	/* -as long as p_flags for e2 has X_FLAG
	**  mark reason as REASxflag even though e1 sak_type
	**  may be the same.  Want to save this state every time because
	**  invalidly marked  PMtab entries are not purged until an end of
	**  session is indicated (i.e. Hangup, SAK, death of child)
	*/
	}else if (e2->p_flags & X_FLAG)
		ret = 0;
		reason |= REASxflag;
	}
	/*
	* compare lowest 4 bits only,
	* because A_FLAG is not part of original ttyflags
	*/
	if ((e1->p_ttyflags & 017) != e2->p_ttyflags){ /*cmp lowest 4 bit only*/
		ret = 0;
		reason |= REASttyflags;
	}
	if (e1->p_count != e2->p_count){
		ret = 0;
		reason |= REAScount;
	}
	if (e1->p_timeout != e2->p_timeout){
		ret = 0;
		reason |= REAStimeout;
	}
	if (e1->p_uid != e2->p_uid){
		ret = 0;
		reason |= REASuid;
	}
	if (e1->p_gid != e2->p_gid){
		ret = 0;
		reason |= REASgid;
	}
	if (strcmp(e1->p_dir, e2->p_dir) != 0){
		ret = 0;
		reason |= REASdir;
	}
	*reasonp = reason;
	return(ret);
}




/*
 * Procedure:	  insert_pmtab
 *
 * Restrictions:
                 sprintf: none
*/

/*
 * insert_pmtab - insert a pmtab entry into the linked list
 */

static	void
insert_pmtab(sp)
register struct pmtab *sp;	/* ptr to entry to be inserted */
{
	register struct pmtab *tsp, *savtsp;	/* scratch pointers */
	int ret;				/* strcmp return value */
	ulong reason;				/* indicates reason for
						** difference when same_entry()
						** indicates a difference in
						** the fields
						*/


# ifdef DEBUG
	debug("in insert_pmtab");
# endif
	savtsp = tsp = PMtab;

/*
 * find the correct place to insert this element
 */

	while (tsp) {
		ret = strcmp(sp->p_tag, tsp->p_tag);
		if (ret > 0) {
			/* keep on looking */
			savtsp = tsp;
			tsp = tsp->p_next;
			continue;
		}
		else if (ret == 0) {
			if (tsp->p_status) {
				/* this is a duplicate entry, ignore it */
				log(MM_ERROR, ":651:Ignoring duplicate entry for <%s>", tsp->p_tag);
			}
			else {
				if (same_entry(tsp,sp,&reason)) {  /* same entry TP!!! */
					tsp->p_status = VALID;
				}
				else {	/* entry changed */
					if ((sp->p_flags & X_FLAG) && 
						((sp->p_dmsg == NULL) ||
						(*(sp->p_dmsg) == '\0'))) {
						/* disabled entry */
						tsp->p_status = NOTVALID;
						tsp->p_reason |= REASdisabled; /*TP!!!*/
					}
					else if (sp->p_sak.sak_type == saktypeUNDEF){
						tsp->p_status = NOTVALID;
						tsp->p_reason |= REASdisabled; /*TP!!!*/
					}
					else {
# ifdef DEBUG
					(void)sprintf(Scratch, "replacing <%s>", sp->p_tag);
					debug(Scratch);
# endif
						/* replace old entry */
						sp->p_next = tsp->p_next;
						if (tsp == PMtab) {
						   PMtab = sp;
						}
						else {
						   savtsp->p_next = sp;
						}
						sp->p_status = CHANGED;
						sp->p_fd = tsp->p_fd;
						sp->p_pid = tsp->p_pid;
					        sp->p_inservice =
							tsp->p_inservice;
						sp->p_tpctrlfd = tsp->p_tpctrlfd;
						sp->p_muxid = tsp->p_muxid;
						sp->p_tpdataconnid = tsp->p_tpdataconnid;
						sp->p_device = tsp->p_device;
						tsp->p_device = (char *)NULL;
						sp->p_reason = reason;
						sp = tsp;
					}
				}
				Nentries++;
			}
			free_pmtab(sp);
			return;
		}
		else {
			if ((sp->p_flags & X_FLAG) && 
				((sp->p_dmsg == NULL) ||
				(*(sp->p_dmsg) == '\0'))) { /* disabled entry */
				free_pmtab(sp);
				return;
			}
			else if (sp->p_sak.sak_type == saktypeUNDEF){
				free_pmtab(sp);
				return;
			}
			/* insert it here */
			if (tsp == PMtab) {
				sp->p_next = PMtab;
				PMtab = sp;
			}
			else {
				sp->p_next = savtsp->p_next;
				savtsp->p_next = sp;
			}
# ifdef DEBUG
			(void) sprintf(Scratch, "adding <%s>", sp->p_tag);
			debug(Scratch);
# endif
			Nentries++;
			/* this entry is "current" */
			sp->p_status = VALID;
			return;
		}
	}

/*
 * either an empty list or should put element at end of list
 */

	if ((sp->p_flags & X_FLAG) && 
		((sp->p_dmsg == NULL) ||
		(*(sp->p_dmsg) == '\0'))) { /* disabled entry */
		free_pmtab(sp);		 /* do not poll this entry */
		return;
	}
	else if (sp->p_sak.sak_type == saktypeUNDEF){
		free_pmtab(sp);
		return;
	}
	sp->p_next = NULL;
	if (PMtab == NULL)
		PMtab = sp;
	else
		savtsp->p_next = sp;
# ifdef DEBUG
	(void) sprintf(Scratch, "adding <%s>", sp->p_tag);
	debug(Scratch);
# endif
	++Nentries;
	/* this entry is "current" */
	sp->p_status = VALID;
}


/*
 * Procedure:	  purge
 *
 * Restrictions:
                 sprintf: none
*/

/*
 * purge - purge linked list of "old" entries
 */


void
purge()
{
	register struct pmtab *sp;		/* working pointer */
	register struct pmtab *savesp, *tsp;	/* scratch pointers */

# ifdef DEBUG
	debug("in purge");
# endif
	sp = savesp = PMtab;
	while (sp) {
		/* 
		** -do not purge NOTVALID entries if children exist
		*/
		if ((sp->p_status) || (sp->p_pid > 0)) {
			savesp = sp;
			sp = sp->p_next;
		}
		else {
			tsp = sp;
			if (tsp == PMtab) {
				PMtab = sp->p_next;
				savesp = PMtab;
			}
			else
				savesp->p_next = sp->p_next;
# ifdef DEBUG
			(void) sprintf(Scratch, "purging <%s>", sp->p_tag);
			debug(Scratch);
# endif
			sp = sp->p_next;
			free_pmtab(tsp);
		}
	}
}

/*
 *	free_pmtab	- free one pmtab entry
 */
static	void
free_pmtab(p)
struct	pmtab	*p;
{
#ifdef	DEBUG
	debug("in free_pmtab");
#endif
	if (p->p_tag != (char *)NULL)
		free(p->p_tag);
	if (p->p_identity != (char *)NULL)
		free(p->p_identity);
	if (p->p_res1 != (char *)NULL)
		free(p->p_res1);
	if (p->p_res2 != (char *)NULL)
		free(p->p_res2);
	if (p->p_iascheme != (char *)NULL)
		free(p->p_iascheme);
	if (p->p_device != (char *)NULL)
		free(p->p_device);
	if (p->p_realdevice != (char *)NULL)
		free(p->p_realdevice);
	if (p->p_server != (char *)NULL)
		free(p->p_server);
	if (p->p_ttylabel != (char *)NULL)
		free(p->p_ttylabel);
	if (p->p_modules != (char *)NULL)
		free(p->p_modules);
	if (p->p_prompt != (char *)NULL)
		free(p->p_prompt);
	if (p->p_dmsg != (char *)NULL)
		free(p->p_dmsg);
	if (p->p_dir)
		free(p->p_dir);
	free(p);
}

/*
 *	check_pmtab - check the fields to make sure things are correct
 *		    - return 0 if everything is ok
 *		    - return -1 if something is wrong
 */

static	int
check_pmtab(p)
struct	pmtab	*p;
{
	if (p == NULL) {
		log(MM_ERROR, ":652:pmtab ptr is NULL");
		return(-1);
	}

	/* check service tag */
	if ((p->p_tag == NULL) || (*(p->p_tag) == '\0')) {
		log(MM_ERROR, ":653:Port/service tag is missing");
		return(-1);
	}
	if (strlen(p->p_tag) > (size_t)(MAXID - 1)) {
		log(MM_ERROR, ":654:Port/service tag <%s> is longer than %d", p->p_tag, MAXID-1);
		return(-1);
	}
	if (strcheck(p->p_tag, ALNUM) != 0) {
		log(MM_ERROR, ":655:Port/service tag <%s> is not alphanumeric", p->p_tag);
		return(-1);
	}

	if (check_identity(p) != 0) {
		return(-1);
	}

	if (check_device(p->p_realdevice) != 0)
		return(-1);

	if (check_cmd(p->p_server) != 0)
		return(-1);
	return(0);
}

extern  struct 	passwd *getpwnam();
extern  void 	endpwent();
extern  struct 	group *getgrgid();
extern  void 	endgrent();

/*
 * Procedure:	  check_identity
 *
 * Restrictions:
		 getpwnam: none
		 endpwent: none
		 getgrgid: P_MACREAD
		 endgrent: none
*/

/*
 *	check_identity - check to see if the identity is a valid user
 *		       - log name in the passwd file,
 *		       - and if its group id is a valid one
 *		  	- return 0 if everything is ok. Otherwise, return -1
 */

int
check_identity(p)
struct	pmtab	*p;
{
	register struct passwd *pwdp;

	if ((p->p_identity == NULL) || (*(p->p_identity) == '\0')) {
		if ((p->p_iascheme == NULL) || (*(p->p_iascheme) == '\0')) {
			log(MM_ERROR, ":867:identity and scheme fields are missing");
			return(-1);
		}
		else {
			p->p_uid = 0;
			p->p_gid = 0;
			p->p_dir = strsave(NULL);
			return(0);
		}
	}
	if ((pwdp = getpwnam(p->p_identity)) == NULL) {
		log(MM_ERROR, ":82:Unknown user id: %s", p->p_identity);
		endpwent();
		return(-1);
	}
	(void) procprivl(CLRPRV, MACREAD_W, 0);
	if (getgrgid(pwdp->pw_gid) == NULL) {
		(void) procprivl(SETPRV, MACREAD_W, 0);
		log(MM_ERROR, ":657:no group entry for %ld", pwdp->pw_gid);
		endgrent();
		endpwent();
		return(-1);
	}
	(void) procprivl(SETPRV, MACREAD_W, 0);
	p->p_uid = pwdp->pw_uid;
	p->p_gid = pwdp->pw_gid;
	p->p_dir = strsave(pwdp->pw_dir);
	endgrent();
	endpwent();
	return(0);
}

/*
 * expand(cmdp, devp)	- expand %d to device name and %% to %,
 *				- any other characters are untouched.
 *				- return the expanded string
 */
static char	*
expand(cmdp,devp)
char	*cmdp;		/* ptr to cmd string	*/
char	*devp;		/* ptr to device name	*/
{
	register char	*cp, *dp, *np;
	static char	buf[BUFSIZ];
	cp = cmdp;
	np = buf;
	dp = devp;
	while (*cp) {
		if (*cp != '%') {
			*np++ = *cp++;
			continue;
		}
		switch (*++cp) {
		case 'd':
			while (*dp) {
				*np++ = *dp++;
			}
			cp++;
			break;
		case '%':
			*np++ = *cp++;
			break;
		default:
			*np++ = *cp++;
			break;
		}
	}
	*np = '\0';
	return(buf);
}
