/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/sendreq.c	1.12.2.3"
#ident  "$Header: sendreq.c 1.2 91/06/26 $"

#include <stdio.h>
#include <netconfig.h>
#include <stropts.h>
#include <sys/types.h>
#include <dial.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "extern.h"

static void	debug_write(int, struct  dial_request *);

static char	netpathenv[STRSIZE] = "NETPATH=";
static char	*usrnetpath;

int
write_dialrequest(fd, Call)
int	fd;
CALL	Call;
{
	struct 	dial_request	dialrequest;
	struct 	dial_request	*dialrequestp=&dialrequest;
	char	*where = (char *)&dialrequest;
	int	dialsize = sizeof(dialrequest);
	int	i;

	if (Call.attr != NULL){
		dialrequestp->termioptr=NOTNULLPTR;
		dialrequestp->c_iflag=Call.attr->c_iflag;	
		dialrequestp->c_oflag=Call.attr->c_oflag;	
		dialrequestp->c_cflag=Call.attr->c_cflag;	
		dialrequestp->c_lflag=Call.attr->c_lflag;	
		dialrequestp->c_line=Call.attr->c_line;	
		strcpy(dialrequestp->c_cc,(char *)(Call.attr->c_cc)); 
	} else
		dialrequestp->termioptr=NULLPTR;

	if (Call.line != NULL) {
		dialrequestp->lineptr=NOTNULLPTR;
		strcpy(dialrequestp->line,Call.line); 
	} else
		dialrequestp->lineptr=NULLPTR;


	if (Call.telno != NULL) {
		dialrequestp->telnoptr=NOTNULLPTR;
		strcpy(dialrequestp->telno,Call.telno); 
	} else
		dialrequestp->telnoptr=NULLPTR;

	if (Call.device != NULL) {
		dialrequestp->deviceptr=NOTNULLPTR;

		if (((CALL_EXT *)Call.device)->service != NULL) { 
			strcpy(dialrequestp->service,
			       ((CALL_EXT *)Call.device)->service); 
			dialrequestp->serviceptr=NOTNULLPTR;
		} else
			dialrequestp->serviceptr=NULLPTR;

		if (((CALL_EXT *)Call.device)->class != NULL) { 
			dialrequestp->classptr=NOTNULLPTR;
			strcpy(dialrequestp->class,
			       ((CALL_EXT *)Call.device)->class); 
		} else
			dialrequestp->classptr=NULLPTR;

		if (((CALL_EXT *)Call.device)->protocol != NULL) { 
			dialrequestp->protocolptr=NOTNULLPTR;
			strcpy(dialrequestp->protocol,
			       ((CALL_EXT *)Call.device)->protocol); 
		} else
			dialrequestp->protocolptr=NULLPTR;

	} else
		dialrequestp->deviceptr=NULLPTR;

	dialrequestp->version=1;	
	dialrequestp->baud=Call.baud;	
	dialrequestp->speed=Call.speed;	
	dialrequestp->modem=Call.modem;	
	dialrequestp->dev_len=Call.dev_len;	
		
	if (Debugging) {
		debug_write(fd, dialrequestp);
	}

	while ((i=write(fd, where, dialsize )) != dialsize) {
		if ( i == -1 )
			return(NULL);
		where = where + i;
		dialsize = dialsize -i;
	}


	return(0);
}

CALL *	
read_dialrequest(fd)
int	fd;
{
	CALL	*Callp;
	char	buff[LRGBUF];
	char	*where=buff;
	int 	dialsize;
	struct 	dial_request	*dialrequestp;
	int	i;
	char 	*pathp;
	dialsize = sizeof( struct dial_request );

	while ((i=read(fd, where, dialsize )) != dialsize) {
		if ( i == -1)
			return(NULL);
		where = where + i;
		dialsize = dialsize -i;
	}

	dialrequestp = (struct dial_request *)buff;
	
	if ((Callp =(CALL *)malloc(sizeof(CALL))) == NULL)
		return(NULL);

	if ((Call.attr =(struct termio *)malloc(sizeof(struct termio))) == NULL)
		return(NULL);
	
	if ((((CALL_EXT *)Call.device) =
	      (CALL_EXT *)malloc(sizeof(CALL_EXT))) == NULL)
		return(NULL);
	

	usrnetpath=dialrequestp->netpath;
        if (strcmp(usrnetpath,"") > 0) {
        	strcat(netpathenv,usrnetpath);
                putenv (netpathenv);
        }

	DUMP((msg, "sr: netpath<%s>", (pathp=getenv(NETPATH))!=NULL?
	      pathp: "NULL"));
	
	Call.baud=dialrequestp->baud;
	Call.speed=dialrequestp->speed;
	Call.modem=dialrequestp->modem;
	Call.dev_len=dialrequestp->dev_len;


	if (dialrequestp->termioptr != NULLPTR) {	
		Call.attr->c_iflag=dialrequestp->c_iflag;
		Call.attr->c_oflag=dialrequestp->c_oflag;
		Call.attr->c_cflag=dialrequestp->c_cflag;
		Call.attr->c_lflag=dialrequestp->c_lflag;
		Call.attr->c_line=(dialrequestp->c_line);
		strcpy((char *)(Call.attr->c_cc),dialrequestp->c_cc); 
	} else
		Call.attr=NULL;

	if (dialrequestp->lineptr != NULLPTR) {	
		Call.line=strdup(dialrequestp->line); 
	} else
		Call.line=NULL;

	Call.telno = (dialrequestp->telnoptr != NULLPTR) ?
	    strdup(dialrequestp->telno): NULL;

	if (dialrequestp->deviceptr != NULLPTR) {	

		((CALL_EXT *)Call.device)->service = 
		    (dialrequestp->serviceptr != NULLPTR) ?
		    strdup(dialrequestp->service): NULL;

		((CALL_EXT *)Call.device)->class =
		    (dialrequestp->classptr != NULLPTR) ?
		    strdup(dialrequestp->class): NULL;

		((CALL_EXT *)Call.device)->protocol =
		    (dialrequestp->protocolptr != NULLPTR) ?
		    strdup(dialrequestp->protocol): NULL;

	} else {
		Call.device=NULL;
	}
		
	Pid = dialrequestp->pid;
	return(Callp);
}

/* 
 * debug_write
 * 
 * print out the information written in write_dialrequest()
 */

static
void
debug_write(fd, dp)
	int	fd;
	struct 	dial_request	*dp;
{

/* a special define for debug_write */
#define D_WRITE(x) {sprintf x; debug(msg);}

	D_WRITE((msg, "sr: writing request to fd:<%d>",fd));
	D_WRITE((msg, "sr: baud<%d>",dp->baud));
	D_WRITE((msg, "sr: speed<%d>",dp->speed));
	D_WRITE((msg, "sr: modem<%d>",dp->modem));
	D_WRITE((msg, "sr: dev_len<%d>",dp->dev_len));

	if (dp->termioptr != NULLPTR) {
		D_WRITE((msg, "sr: c_iflag<%d>",dp->c_iflag));
		D_WRITE((msg, "sr: c_oflag<%d>",dp->c_oflag));
		D_WRITE((msg, "sr: c_cflag<%d>",dp->c_cflag));
		D_WRITE((msg, "sr: c_lflag<%d>",dp->c_lflag));
		D_WRITE((msg, "sr: c_line<%c>",dp->c_line));
	} else {
		D_WRITE((msg, "sr: termio = NULL"));
	}

	D_WRITE((msg, "sr: line<%s>", dp->lineptr != NULLPTR? 
	      dp->line: "NULL"));
	D_WRITE((msg, "sr: telno<%s>", dp->telnoptr != NULLPTR? 
	      dp->telno: "NULL"));

	if (dp->deviceptr != NULLPTR) {	
		D_WRITE((msg,"sr: service<%s>", 
		    dp->serviceptr != NULLPTR? dp->service: "NULL"));
		D_WRITE((msg,"sr: class<%s>", 
		    dp->classptr != NULLPTR? dp->class: "NULL"));
		D_WRITE((msg,"sr: protocol<%s>",
		    dp->protocolptr != NULLPTR? dp->protocol: "NULL"));
	} else {
		D_WRITE((msg, "sr: deviceptr = NULL"));
	}
}
