#ident	"@(#)siserver:os/io.c	1.5"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/***********************************************************
Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: io.c,v 1.65 89/09/14 16:19:50 rws Exp $ */

/*****************************************************************
 * i/o functions
 *
 *   WriteToClient, ReadRequestFromClient
 *   InsertFakeRequest, ResetCurrentRequest
 *
 *****************************************************************/

#include <stdio.h>
#include "Xos.h"
#include "Xmd.h"
#include <errno.h>
#include <sys/param.h>
#ifdef SVR4
#include <sys/uio.h>
#endif
#include "X.h"
#include "Xproto.h"
#include "os.h"
#include "osdep.h"
#include "opaque.h"
#include "dixstruct.h"
#include "misc.h"

#include "Xstreams.h"

extern Xstream	xstream[];
extern char	TypeOfStream[];

extern void MarkClientException();
extern long ClientsWithInput[];
extern long YieldedClientsWithInput[];
extern long ClientsBlocked[];
extern long OutputPending[];
extern long OutputBufferSize;
extern int ConnectionTranslation[];
static int timesThisConnection = 0;
static ConnectionInputPtr FreeInputs = (ConnectionInputPtr)NULL;
static ConnectionOutputPtr FreeOutputs = (ConnectionOutputPtr)NULL;
static OsCommPtr AvailableInput = (OsCommPtr)NULL;

static ConnectionInputPtr AllocateInputBuffer();
static int TransmitData();

#ifndef i386	/* funNotUsedByATT, AllocateOutputBuffer */

static ConnectionOutputPtr AllocateOutputBuffer();

#endif /* i386, funNotUsedByATT */

extern int errno;

#define request_length(req, cli) ((cli->swapped ? \
	lswaps((unsigned)(req)->length) : (unsigned)(req)->length) << 2)
#define MAX_TIMES_PER         10

/*****************************************************************
 * ReadRequestFromClient
 *    Returns one request in client->requestBuffer.  Return status is:
 *
 *    > 0  if  successful, specifies length in bytes of the request
 *    = 0  if  entire request is not yet available
 *    < 0  if  client should be terminated
 *
 *    The request returned must be contiguous so that it can be
 *    cast in the dispatcher to the correct request type.  Because requests
 *    are variable length, ReadRequestFromClient() must look at the first 4
 *    bytes of a request to determine the length (the request length is
 *    always the 3rd and 4th bytes of the request).  
 *
 *    Note: in order to make the server scheduler (WaitForSomething())
 *    "fair", the ClientsWithInput mask is used.  This mask tells which
 *    clients have FULL requests left in their buffers.  Clients with
 *    partial requests require a read.  Basically, client buffers
 *    are drained before select() is called again.  But, we can't keep
 *    reading from a client that is sending buckets of data (or has
 *    a partial request) because others clients need to be scheduled.
 *****************************************************************/

#define YieldControl()				\
        { isItTimeToYield = TRUE;		\
	  timesThisConnection = 0; }
#define YieldControlNoInput()			\
        { YieldControl();			\
	  BITCLEAR(ClientsWithInput, fd); }
#define YieldControlDeath()			\
        { timesThisConnection = 0; }

int
ReadRequestFromClient(client)
    ClientPtr client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    register ConnectionInputPtr oci = oc->input;
    int fd = oc->fd;
    int result, gotnow, needed;
    register xReq *request;

    if (AvailableInput)
    {
	if (AvailableInput != oc)
	{
	    AvailableInput->input->next = FreeInputs;
	    FreeInputs = AvailableInput->input;
	    AvailableInput->input = (ConnectionInputPtr)NULL;
	}
	AvailableInput = (OsCommPtr)NULL;
    }
    if (!oci)
    {
	if (oci = FreeInputs)
	{
	    FreeInputs = oci->next;
	}
	else if (!(oci = AllocateInputBuffer()))
	{
	    YieldControlDeath();
	    return -1;
	}
	oc->input = oci;
    }
    oci->bufptr += oci->lenLastReq;

    request = (xReq *)oci->bufptr;
    gotnow = oci->bufcnt + oci->buffer - oci->bufptr;
    if ((gotnow < sizeof(xReq)) ||
	(gotnow < (needed = request_length(request, client))))
    {
	oci->lenLastReq = 0;
	if ((gotnow < sizeof(xReq)) || (needed == 0))
	   needed = sizeof(xReq);
	else if (needed > MAXBUFSIZE)
	{
	    YieldControlDeath();
	    return -1;
	}
	if ((gotnow == 0) ||
	    ((oci->bufptr - oci->buffer + needed) > oci->size))
	{
	    if ((gotnow > 0) && (oci->bufptr != oci->buffer))
		bcopy(oci->bufptr, oci->buffer, gotnow);
	    if (needed > oci->size)
	    {
		char *ibuf;

		ibuf = (char *)xrealloc(oci->buffer, needed);
		if (!ibuf)
		{
		    YieldControlDeath();
		    return -1;
		}
		oci->size = needed;
		oci->buffer = ibuf;
	    }
	    oci->bufptr = oci->buffer;
	    oci->bufcnt = gotnow;
	}

	result = (*(xstream[TypeOfStream[fd]].ReadFromStream))
                        (fd, oci->buffer + oci->bufcnt,
                             oci->size - oci->bufcnt, NO_BUFFERING);

	if (result <= 0)
	{
	    if ((result < 0) && (errno == EWOULDBLOCK))
	    {
		YieldControlNoInput();
		return 0;
	    }
	    YieldControlDeath();
	    return -1;
	}
	oci->bufcnt += result;
	gotnow += result;
	/* free up some space after huge requests */
	if ((oci->size > BUFWATERMARK) &&
	    (oci->bufcnt < BUFSIZE) && (needed < BUFSIZE))
	{
	    char *ibuf;

	    ibuf = (char *)xrealloc(oci->buffer, BUFSIZE);
	    if (ibuf)
	    {
		oci->size = BUFSIZE;
		oci->buffer = ibuf;
		oci->bufptr = ibuf + oci->bufcnt - gotnow;
	    }
	}
	request = (xReq *)oci->bufptr;
	if ((gotnow < sizeof(xReq)) ||
	    (gotnow < (needed = request_length(request, client))))
	{
	    YieldControlNoInput();
	    return 0;
	}
    }

    if(request_length(request, client) == 0)
    {
/*
	    *status = -1;
	    YieldControlAndReturnNull();
*/
	    YieldControlNoInput();
	    return -1;
    }
	
    if (needed == 0)
	needed = sizeof(xReq);
    oci->lenLastReq = needed;

    /*
     *  Check to see if client has at least one whole request in the
     *  buffer.  If there is only a partial request, treat like buffer
     *  is empty so that select() will be called again and other clients
     *  can get into the queue.   
     */

    if (gotnow >= needed + sizeof(xReq)) 
    {
	request = (xReq *)(oci->bufptr + needed);
        if (gotnow >= needed + request_length(request, client))
	    BITSET(ClientsWithInput, fd);
        else
	    YieldControlNoInput();
    }
    else
    {
	if (gotnow == needed)
	    AvailableInput = oc;
	YieldControlNoInput();
    }
    if (++timesThisConnection >= MAX_TIMES_PER) {
	/* Indicate this client has given up its time for reading */
	BITSET(YieldedClientsWithInput, fd);
	BITCLEAR(ClientsWithInput, fd);
	YieldControl();
    }

    client->requestBuffer = (pointer)oci->bufptr;
    return needed;
}

/*****************************************************************
 * InsertFakeRequest
 *    Splice a consed up (possibly partial) request in as the next request.
 *
 **********************/

Bool
InsertFakeRequest(client, data, count)
    ClientPtr client;
    char *data;
    int count;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    register ConnectionInputPtr oci = oc->input;
    int fd = oc->fd;
    register xReq *request;
    int gotnow, moveup;

    if (AvailableInput)
    {
	if (AvailableInput != oc)
	{
	    AvailableInput->input->next = FreeInputs;
	    FreeInputs = AvailableInput->input;
	    AvailableInput->input = (ConnectionInputPtr)NULL;
	}
	AvailableInput = (OsCommPtr)NULL;
    }
    if (!oci)
    {
	if (oci = FreeInputs)
	    FreeInputs = oci->next;
	else if (!(oci = AllocateInputBuffer()))
	    return FALSE;
	oc->input = oci;
    }
    oci->bufptr += oci->lenLastReq;
    oci->lenLastReq = 0;
    gotnow = oci->bufcnt + oci->buffer - oci->bufptr;
    if ((gotnow + count) > oci->size)
    {
	char *ibuf;

	ibuf = (char *)xrealloc(oci->buffer, gotnow + count);
	if (!ibuf)
	    return(FALSE);
	oci->size = gotnow + count;
	oci->buffer = ibuf;
	oci->bufptr = ibuf + oci->bufcnt - gotnow;
    }
    moveup = count - (oci->bufptr - oci->buffer);
    if (moveup > 0)
    {
	if (gotnow > 0)
	    bcopy(oci->bufptr, oci->bufptr + moveup, gotnow);
	oci->bufptr += moveup;
	oci->bufcnt += moveup;
    }
    bcopy(data, oci->bufptr - count, count);
    oci->bufptr -= count;
    request = (xReq *)oci->bufptr;
    gotnow += count;
    if ((gotnow >= sizeof(xReq)) &&
	(gotnow >= request_length(request, client)))
	BITSET(ClientsWithInput, fd);
    else
	YieldControlNoInput();
    return(TRUE);
}

/*****************************************************************
 * ResetRequestFromClient
 *    Reset to reexecute the current request, and yield.
 *
 **********************/

ResetCurrentRequest(client)
    ClientPtr client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    register ConnectionInputPtr oci = oc->input;
    int fd = oc->fd;
    register xReq *request;
    int gotnow;

    if (AvailableInput == oc)
	AvailableInput = (OsCommPtr)NULL;
    oci->lenLastReq = 0;
    request = (xReq *)oci->bufptr;
    gotnow = oci->bufcnt + oci->buffer - oci->bufptr;
    if ((gotnow >= sizeof(xReq)) &&
	(gotnow >= request_length(request, client)))
    {
	BITSET(ClientsWithInput, fd);
	YieldControl();
    }
    else
	YieldControlNoInput();
}

    /* lookup table for adding padding bytes to data that is read from
    	or written to the X socket.  */
static int padlength[4] = {0, 3, 2, 1};


extern long ClientsBlocked[];

struct bufblock {
	struct	bufblock *nextblock;
	int	nleft;
	char	*dataptr;
	char	databuf[1];
	};

struct buflist {
	struct  bufblock *firstsavedbuffer;
	struct  bufblock *lastsavedbuffer;
	int	listlength;
	};

static struct buflist  buflist[MAXSOCKS];

InitClientBuffers(fd)
int	fd;
{
	buflist[fd].lastsavedbuffer = buflist[fd].firstsavedbuffer = NULL;
    	buflist[fd].listlength = 0; 
    	BITCLEAR(ClientsBlocked, fd);
}

FreeClientBuffers(fd)
int	fd;
{
	register struct bufblock *b1 = buflist[fd].firstsavedbuffer, *b2;

	while(b1 != NULL){
		b2 = b1;
		b1 = b2->nextblock;
		Xfree(b2);
		}
	InitClientBuffers(fd);
}


static
AppendBufferToList(listptr, buf, size)
struct buflist *listptr;
char	*buf;
int	size;
{
	if(listptr->firstsavedbuffer == NULL)
		listptr->lastsavedbuffer = listptr->firstsavedbuffer =
		      (struct bufblock *) Xalloc(sizeof(struct bufblock)+size);
	else {
		listptr->lastsavedbuffer->nextblock = 
		      (struct bufblock *) Xalloc(sizeof(struct bufblock)+size);
		listptr->lastsavedbuffer = listptr->lastsavedbuffer->nextblock;
	}

	listptr->lastsavedbuffer->dataptr = 
			listptr->lastsavedbuffer->databuf;
	listptr->lastsavedbuffer->nleft = size;
	listptr->lastsavedbuffer->nextblock = NULL;
	listptr->listlength += size;
	memcpy(listptr->lastsavedbuffer->databuf, buf, size);
#ifdef DEBUG
	fprintf(stderr, "Append %d total bufferd data %d\n", size, listptr->listlength);
#endif
}


/*****************
 * WriteToClient
 *    The following is not a valid comment so ignore it. One day I will
 *    replace it with the right comments.
 ****************************************************************************
 *    We might have to wait, if the client isn't keeping up with us.  We 
 *    wait for a short time, then close the connection.  This isn't a 
 *    wonderful solution,
 *    but it rarely seems to be a problem right now, and buffering output for
 *    asynchronous delivery sounds complicated and expensive.
 *    Long word aligns all data.
 *****************/

int
WriteToClient (who, count, buf)
    ClientPtr who;
    char *buf;
    int count;
{
    int connection = ((OsCommPtr)who->osPrivate)->fd;

    register int n = 0, m = 0, ActualCount = 0; 
    register struct buflist *listptr = &buflist[connection];

    /*
    ErrorF("Calling WriteToClient for client %d of type %d\n",
			 connection, TypeOfStream[connection]);
    */

    if (connection < 0) 
    {
	    if (connection == -2) 
    	{
#ifdef notdef
		ErrorF( "CONNECTION %d ON ITS WAY OUT\n", connection);
#endif
		return(-1);
    	}
	ErrorF( "OH NO, %d translates to -1\n", connection);
	return(-1);
    }

    errno = 0;

    count += padlength[count & 3];
    
#ifdef DEBUG
fprintf(stderr, "Calling WriteToClient for client %d with %d char\n",
                         connection, count);
#endif
    if(listptr->listlength == 0)
    {
	while ((n = TransmitData (connection, buf, count - ActualCount)) > 0)
	{
		if((ActualCount += n) >= count)
                          return(count);
		buf   += n;
	}
#ifdef DEBUG
	if(n < 0)
		fprintf(stderr, "Error in WriteToClient %d\n", errno);
#endif
	if( n < 0 && ioFatalError(connection))
			return(-1);
	n = count - ActualCount;
	AppendBufferToList(listptr, buf, n);
    }
    else 
    {
    	AppendBufferToList(listptr, buf, count);
	if(TryToWriteAgain(connection) < 0  && ioFatalError(connection))
                        return(-1);
    }

    if(listptr->listlength > 0)
		BITSET(ClientsBlocked, connection);
    else	InitClientBuffers(connection);



    return(count);
}


static ConnectionInputPtr
AllocateInputBuffer()
{
    register ConnectionInputPtr oci;

    oci = (ConnectionInputPtr)xalloc(sizeof(ConnectionInput));
    if (!oci)
	return (ConnectionInputPtr)NULL;
    oci->buffer = (char *)xalloc(BUFSIZE);
    if (!oci->buffer)
    {
	xfree(oci);
	return (ConnectionInputPtr)NULL;
    }
    oci->size = BUFSIZE;
    oci->bufptr = oci->buffer;
    oci->bufcnt = 0;
    oci->lenLastReq = 0;
    return oci;
}

#ifndef i386	/* funNotUsedByATT, AllocateOutputBuffer */

static ConnectionOutputPtr
AllocateOutputBuffer()
{
    register ConnectionOutputPtr oco;

    oco = (ConnectionOutputPtr)xalloc(sizeof(ConnectionOutput));
    if (!oco)
	return (ConnectionOutputPtr)NULL;
    oco->buf = (unsigned char *) xalloc(OutputBufferSize);
    if (!oco->buf)
    {
	xfree(oco);
	return (ConnectionOutputPtr)NULL;
    }
    oco->size = OutputBufferSize;
    oco->count = 0;
    return oco;
}

#endif /* i386, funNotUsedByATT */

void
FreeOsBuffers(oc)
    OsCommPtr oc;
{
    register ConnectionInputPtr oci;
    register ConnectionOutputPtr oco;

    if (AvailableInput == oc)
	AvailableInput = (OsCommPtr)NULL;
    if (oci = oc->input)
    {
	if (FreeInputs)
	{
	    xfree(oci->buffer);
	    xfree(oci);
	}
	else
	{
	    FreeInputs = oci;
	    oci->next = (ConnectionInputPtr)NULL;
	    oci->bufptr = oci->buffer;
	    oci->bufcnt = 0;
	    oci->lenLastReq = 0;
	}
    }
    if (oco = oc->output)
    {
	if (FreeOutputs)
	{
	    xfree(oco->buf);
	    xfree(oco);
	}
	else
	{
	    FreeOutputs = oco;
	    oco->next = (ConnectionOutputPtr)NULL;
	    oco->count = 0;
	}
    }
}

void
ResetOsBuffers()
{
    register ConnectionInputPtr oci;
    register ConnectionOutputPtr oco;

    while (oci = FreeInputs)
    {
	FreeInputs = oci->next;
	xfree(oci->buffer);
	xfree(oci);
    }
    while (oco = FreeOutputs)
    {
	FreeOutputs = oco->next;
	xfree(oco->buf);
	xfree(oco);
    }
}


#define BLOCKSIZE	512

int
TryToWriteAgain(fd)
int	fd;
{
    register int	n, packed;
    register struct bufblock *blockptr;
    register struct buflist *listptr = &buflist[fd];
    char	tmpbuf[BLOCKSIZE], *ptr;
    char	StreamIsNotBlocked = 1;

    while(listptr->listlength > 0 && StreamIsNotBlocked)
    {
	packed = 0;
	blockptr = listptr->firstsavedbuffer;
	while((n = BLOCKSIZE - packed) > 0)
	{
		if(n < blockptr->nleft)
			break;
		memcpy(&tmpbuf[packed], blockptr->dataptr, blockptr->nleft);
		packed += blockptr->nleft;
		if((blockptr = blockptr->nextblock) == NULL)
			break;
	}
	if(packed == 0){
		packed = blockptr->nleft;
		ptr = blockptr->dataptr;
                blockptr = blockptr->nextblock;
		}
	else	ptr = tmpbuf;
				
	n = TransmitData (fd,ptr,packed);
	if(n <= 0)
		return(n);

#ifdef DEBUG
	fprintf(stderr, "wrote %d out of %d\n", n, listptr->listlength);
#endif
	listptr->listlength -= n;
	StreamIsNotBlocked = packed == n;

	blockptr = listptr->firstsavedbuffer;

	while(n > 0)
		if(n >= blockptr->nleft){
			n -= blockptr->nleft;
			ptr = (char *) blockptr;
			blockptr = blockptr->nextblock;
			Xfree(ptr);
			}
		else {
			listptr->firstsavedbuffer = blockptr;
			blockptr->dataptr += n;
			blockptr->nleft -= n;
		 	return(0);
		}
	listptr->firstsavedbuffer = blockptr;
    }
    if(StreamIsNotBlocked){
#ifdef DEBUG
	fprintf(stderr, "No io buffered for client %d\n", fd);
#endif
	InitClientBuffers(fd);
    	return(1);
	}
    return(0);
}

ioFatalError(fd)
int	fd;
{
	if( errno != EINTR && errno != EWOULDBLOCK)
	{
                MarkClientException(clients[ConnectionTranslation[fd]]);
#ifdef DEBUG
		fprintf(stderr, "ioFatalError(%d) with errno %d\n", fd, errno);
#endif
		return(1);
	}
	return(0);
}

static int
TransmitData (connection, buf, count)
int	connection, count;
char	*buf;
{
	int	n;
	int	total, left;
	int	packet = -1;

	errno = 0;

     	n = (*xstream[TypeOfStream[connection]].WriteToStream)
                        (connection, buf, count);

	if(errno != ERANGE)
	{
#ifdef DEBUG
		fprintf(stderr, "TransmitData sent %d out of %d;errno %d\n",
					n, count, errno);
#endif
		return(n);
	}

	left = count;
	total = count;
	if(packet < 0)
		packet = count;

	do
	{
		if(errno == ERANGE) 
		{
			packet >>= 1;	
			if(packet <= 0)
			{
#ifdef DEBUG
			fprintf(stderr, "TransmitData packet is %d\n", packet);
#endif
				return(-1);
			}
#ifdef DEBUG
			fprintf(stderr, "TransmitData packet is %d\n", packet);
#endif
		}
		if(packet > left)
			count = left;
		else    count = packet;

		errno = 0;
     		n = (*xstream[TypeOfStream[connection]].WriteToStream)
                        (connection, buf, count);

		if(n > 0)
		{
			buf += n;
			left -= n;
		}
		else if(errno != ERANGE)
			break;
	} while(left > 0);

	n = total - left;
	if(n >= 0)
		return(n);
	return(-1);
}
