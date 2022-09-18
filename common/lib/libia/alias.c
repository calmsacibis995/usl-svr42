/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libia:common/lib/libia/alias.c	1.1.4.2"
#ident  "$Header: alias.c 1.2 91/06/21 $"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mac.h>
#include <ia.h>
#include <audit.h>
#include <pfmt.h>
#include <locale.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>


#define	ALIAS		"alias"
#define	SAME		0

/*
 * Be sure that the message strings always specify the catalog
 * name explicitly, since no setcat() is done in the library.
 * And you don't want to necessarily use the catalog that the
 * calling command uses.
 */
#define MSG_MALLOC "uxcore:710:unable to allocate space\n"
#define MSG_CLASS "uxcore:711:unable to obtain event class information\n"
#define MSG_SYNTAX "uxcore:712:syntax error in <%s>, line <%d>\n"
#define MSG_TOOLONG "uxcore:713:event type or class \"%s\" exceeds <%d> characters\n"
#define MSG_BADCHAR "uxcore:714:event type or class \"%s\" contains non-printable characters\n"

static int chk_atbl(), verify(), isalias(), in_evtbl();
static void process_atbl(), search_atbl(), add_evt();
extern int isprint();

char *atbl_buf;		/* pounter to alias table buffer(classes) */	
char **evtlist = NULL;	/* pounter to the list of classes/types */	

extern	int	errno;

int aliases 	= 0;
int pos 	= 0;
int cflag 	= 0;

/********************************************************
 *	Allocate a buffer for the alias table and read
 *	in the contents of /etc/security/audit/classes 
 *	(alias file).
 ********************************************************/
int
cr_atbl(atbl)
register struct	stat	atbl;
{
	int	fd;
	char    *end_of_atbl;


	if ((atbl_buf = (char *)malloc((unsigned)(atbl.st_size +1))) == NULL)
	{
		(void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
                exit(ADT_MALLOC);
	}

	if ((fd = open(ADT_CLASSFILE, O_RDONLY)) == -1) 
	{
		(void)pfmt(stderr,MM_ERROR,MSG_CLASS);
		return(1);
	}

	if (read(fd,atbl_buf,(unsigned)atbl.st_size) != atbl.st_size) 
	{
		(void)pfmt(stderr,MM_ERROR,MSG_CLASS);
		return(1);
	}
	end_of_atbl = atbl_buf + atbl.st_size;
	*end_of_atbl = '\0';
	close(fd);
	/***********************************************************
	 *	Check the syntax of each alias entry in the 
	 *	/etc/security/audit/classes file 
	 *	and determine the total number of alias entries.
	 ***********************************************************/
	if ((aliases = chk_atbl(atbl_buf)) == -1) {
		errno=EINVAL;
		return(1);
	}
	/***********************************************************
	 *	Allocate a structure for each alias entry. Each 
	 *	structure consists of two pointers, one for the 
	 *	alias name and the other for its corresponding list.
	 ***********************************************************/
	if((evtabptr=(struct evtab *)malloc(sizeof(struct evtab)*aliases))==0) 
	{
         	(void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		exit(ADT_MALLOC);
	}

	process_atbl(evtabptr,atbl_buf);
	cflag++;
	return(0);
}

/**********************************************************
 *	Check the alias table for syntactic errors,
 * 	and determine the number of alias entries.
 *	If the new line character ('\n') of an alias entry
 *	is immediately preceded by a backslash then the 
 *	next line is a continuation, or else it represents
 *	the next alias entry.
 **********************************************************/

int
chk_atbl(atbl_ptr)
char	*atbl_ptr;
{
	int	line = 0;
	int	errflg = 0;
	int	num_aliases = 0;

	while (*atbl_ptr != '\0') {
		atbl_ptr += strspn(atbl_ptr," \t");
		while (*atbl_ptr == '\n') {
			atbl_ptr++;
			atbl_ptr += strspn(atbl_ptr," \t");
			line++;
		}
		if (*atbl_ptr == '\0')
			break;

		if (strncmp(atbl_ptr,ALIAS,strlen(ALIAS)) == SAME) {
			atbl_ptr = strpbrk(atbl_ptr,"\n");
			line++;
			while (*(atbl_ptr - 1) == '\\') {
				if (*(atbl_ptr + 1) == '\0') {
					(void)pfmt(stderr,MM_ERROR,MSG_SYNTAX,ADT_CLASSFILE,line);
					errflg++;
					break;
				}
				*(atbl_ptr - 1) = ' ';
				*atbl_ptr = ' ';
				atbl_ptr = strpbrk(atbl_ptr,"\n");
				line++;
			}
			num_aliases++;
		} else {
			atbl_ptr = strpbrk(atbl_ptr,"\n");
			line++;
			errflg++;
			(void)pfmt(stderr,MM_ERROR,MSG_SYNTAX,ADT_CLASSFILE,line);
		}
		atbl_ptr++;
	}
	if (errflg)
		return(-1);
	else
		return(num_aliases);
}

/***************************************************************
 *	Set the pointers for each alias entry in the alias table
 *	so that the first pointer points to the alias name 
 * 	while the second points to the alias list.
 *	NOTE: alias entries without an alias list are ignored.
 ***************************************************************/

void
process_atbl(atbl,atbl_ptr)
struct	evtab	*atbl;
char	*atbl_ptr;
{
	register int i;
	int	 inval_cnt = 0;
	register char *buf;

	for (i=0; i<aliases; i++) {
		atbl_ptr += strspn(atbl_ptr," \t\n");
		atbl_ptr += strlen(ALIAS);
		atbl_ptr += strspn(atbl_ptr," \t");
		buf = strpbrk(atbl_ptr," \t\n");
		if(*buf == '\n' || *(buf + strspn(buf," \t")) == '\n'){
			atbl_ptr = buf + strspn(buf," \t\n");
			inval_cnt++;
			continue;
		}
		*buf++ = '\0';
		atbl->aclassp = atbl_ptr;
		buf += strspn(buf," \t");
		atbl_ptr = strpbrk(buf,"\n");
		*atbl_ptr++ = '\0';
		atbl->atyplistp = buf;
		atbl++;
	}
	aliases -= inval_cnt;
}


void
cr_evtbl(evtbl)
char	*evtbl[];
{
	register int index = 0;
	int	 old_pos;
	char	 *event;


	if ((evtlist = (char **)malloc((ADT_NUMOFEVTS) * sizeof(char *))) == NULL) 
	{
         	(void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		exit(ADT_MALLOC);
	}


	/********************************************************
	 *	If the alias table exists, replace events defined 
	 *	as classes, or else assume the events to be
	 *	valid event types and form an event list.
	 ********************************************************/
	pos=0; /*start populating array at position zero*/
	while ((event = evtbl[index]) != NULL) {
		if (*event == '\0' || verify(event)) {
			evtbl[index++][0] = '\0';
			continue;
		}

		if (cflag && isalias(event,evtabptr)) {
			old_pos = pos;
			search_atbl(event,aliases);
			if (old_pos == pos)
				evtbl[index][0] = '\0';
		} else {
			if (!in_evtbl(event))
				add_evt(event);
			else
				evtbl[index][0] = '\0';
		}
		index++;
	}
	evtlist[pos] = NULL;
	return;
}

/**************************************************
	Is given event name a class alias?
**************************************************/

int
isalias(evt,atbl)
char	*evt;
struct	evtab	*atbl;
{
	register int i;

	for (i = 0; i < aliases; i++) {
		if (strcmp(evt,atbl->aclassp) == SAME) 
			return(1);
		atbl++;
	}
	return(0);
}

/*************************************************************
 *	Search the alias table to determine if "event" is an
 *	alias. If it is an alias obtain its corresponding
 *	list of events, and determine if they are aliases by
 *	searching preceding entries in the alias table. This
 *	is done to eliminate loops. When "event" is no longer
 * 	an alias copy it into the client list.		
 *************************************************************/

void
search_atbl(evt,index)
char	*evt;
int	index;
{
	register int i, level;
	register char *list, *curr_pos;
	char	buffer[ADT_EVTNAMESZ + 1];
	struct	evtab	*atbl;

	atbl = evtabptr;
	level = 0;
	for (i = 0; i < index; i++) {
		if (strcmp(evt,atbl->aclassp) == SAME) {
			list = atbl->atyplistp;
			curr_pos = strpbrk(list," \t");
			while (curr_pos != NULL) {
				if (curr_pos-list <= ADT_EVTNAMESZ) {
					strncpy(buffer,list,curr_pos-list);
					buffer[curr_pos-list] = '\0';
					search_atbl(buffer,level);
				} else {
					*curr_pos = '\0';
					(void)pfmt(stderr,MM_WARNING,MSG_TOOLONG,list,ADT_EVTNAMESZ);

					*curr_pos = ' ';
				}
				list = curr_pos + strspn(curr_pos," \t");
				curr_pos = strpbrk(list," \t");
			}
			if (*list == '\0')
				return;

			if ((int)strlen(list) <= ADT_EVTNAMESZ) {
				strcpy(buffer,list);
				search_atbl(buffer,level);
			} else
				(void)pfmt(stderr,MM_WARNING,MSG_TOOLONG,list,ADT_EVTNAMESZ);
			return;
		} else {
			level++;
			atbl++;
		}
	}
	if (!in_evtbl(evt))
		add_evt(evt);
	return;
}

/********************************************************
	Is given event name already in event list?
********************************************************/

int
in_evtbl(evt)
char	*evt;
{
 	register int i;

	for (i = 0; i < pos; i++) {
		if (strcmp(evt,evtlist[i]) == SAME){
			return(1);
		}
	}
	return(0);
}

/********************************************************
	Add given event name to event list!
********************************************************/

void
add_evt(evt)
char	*evt;
{
	if ((evtlist[pos] = malloc((unsigned)strlen(evt)+1)) == NULL ) 
	{
         	(void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		exit(ADT_MALLOC);
	}
	strcpy(evtlist[pos++],evt);
}

/**************************************
	Verify length of event name	
***************************************/

int
verify(evt)
char	*evt;
{
	register char *evtmp;

	if ((int)strlen(evt) > ADT_EVTNAMESZ) {
		(void)pfmt(stderr,MM_WARNING,MSG_TOOLONG,evt,ADT_EVTNAMESZ);
		return(1);
	}

	evtmp = evt;
	while (*evtmp != '\0') {
		if (!isprint(*evtmp)) {
			(void)pfmt(stderr,MM_WARNING,MSG_BADCHAR,evt);
			return(1);
		}
		evtmp++;
	}
	return(0);
}

