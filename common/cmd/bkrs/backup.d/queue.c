/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/backup.d/queue.c	1.5.5.3"
#ident  "$Header: queue.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<time.h>
#include	<backup.h>
#include	<bkmsgs.h>

extern void *malloc();
extern void brlog();
extern void free();

static queued_msg_t	*inhead = 0, *intail = 0;

/*
	This file contains routines that manage the various queues.
*/

/* Place a new message at the head of the new message queue */
void
in_enqueue( type, data )
int type;
bkdata_t *data;
{
	queued_msg_t *newmsg;
	if( !( newmsg = (queued_msg_t *)malloc( sizeof( queued_msg_t ) ) ) ) {
		brlog( "Unable to malloc for a new incoming message.\n" );
		return;
	}
	newmsg->type = type;
	newmsg->data = *data;
	newmsg->next = 0;
	if( !intail ) {
		intail = inhead = newmsg;
	} else {
		inhead->next = newmsg;
		inhead = newmsg;
	}
}

/* Remove a new message from the tail of the new message queue */
int
in_dequeue( type, data )
int *type;
bkdata_t *data;
{
	register queued_msg_t *msg;
	if( !intail ) return( 0 );

	msg = intail;
	intail = intail->next;
	msg->next = NULL;
	if( !intail ) inhead = NULL;

	*type = msg->type;
	*data = msg->data;

	free( msg );
	return( 1 );
}
