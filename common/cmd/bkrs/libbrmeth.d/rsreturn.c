/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/rsreturn.c	1.5.5.2"
#ident  "$Header: rsreturn.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<time.h>
#include	<backup.h>
#include	<bkmsgs.h>
#include	<bkrs.h>

extern int brtype;
extern pid_t bkdaemonpid;

extern unsigned int strlen();
extern char *strcpy();
extern int bkm_send();

int
rsresult( jobid, retcode, explanation )
int retcode;
char *jobid, *explanation;
{
	bkdata_t msg;

	switch( brtype ) {

	case RESTORE_T:

		if( explanation ) {
			if( strlen( explanation ) > BKTEXT_SZ )
				return( BRTOOBIG );
			(void) strcpy( msg.rsret.errmsg, explanation );
		} else *(msg.rsret.errmsg) = '\0';

		if( jobid ) {
			if( strlen( jobid ) > BKJOBID_SZ )
				return( BRTOOBIG );
			(void) strcpy( msg.rsret.jobid, jobid );
		} else *(msg.rsret.jobid) = '\0';

		msg.rsret.retcode = retcode;

		if( bkm_send( bkdaemonpid, RSRESULT, &msg ) == -1 )
			return( BRFATAL );
		break;

	case BACKUP_T:
		break;

	default:
		return( BRNOTINITIALIZED );
	}
	return( BRSUCCESS );
}
