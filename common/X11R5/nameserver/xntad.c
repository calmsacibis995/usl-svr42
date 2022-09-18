/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*copyright     "%c%"*/
#ident	"@(#)nameserver:xntad.c	1.6"

#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/param.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tiuser.h>
#include <sys/netconfig.h>
#include <netdir.h>
#include <fcntl.h>
#include <stdio.h>

#include "Xstreams.h"

#include <sys/utsname.h>
#include "Xproto.h"
#include "osdep.h"

#define	NSECONDS	2	

extern char *GetXWINHome ();
extern	char	*calloc(), *realloc();
extern	char	*program;
int	network;
int	nextentry;

char *makePacket();

int	flags = 0;
extern	char	*calloc(), *realloc();
char	*xalloc(), *xrealloc(), *alialloc();
extern  char    *program;
int	network;
int	nextentry;
char    *TheEnd;
char    *inbuf;
int     inlen;
int     dispno;
char    display[64];
int     nhosts, nHosts;

struct netconfig   *netconfigp = NULL; 


static IOBUFFER InputBuffer[MAXSOCKS];

extern int t_errno;


int ListenFd;
int lastfdesc;			/* maximum file descriptor */

long WellKnownConnections;    /* Listener mask */
long AllSockets[mskcnt];	/* select on this */
long AllClients[mskcnt];	      /* available clients */
long LastSelectMask[mskcnt];	      /* mask returned from last select call */
long MaxClients = MAXSOCKS ;

#define SUCCESS		"1"
char	*ptmx = "/dev/ptmx";

/*
int	sig = -1;
catchit()
{
}
*/


WaitForInput()
{
    int	Quit();
    int i;
    struct timeval waittime, *wt;
    long timeout;
    long readyClients[mskcnt];
    long curclient;
    int selecterr;

    CLEARBITS(readyClients);

    COPYBITS(AllSockets, LastSelectMask);

    wt = NULL;

/* 
    if (!ANYSET(AllClients))
    {
    	wt = &waittime;
	waittime.tv_sec  = 5*60;
	waittime.tv_usec = 0;
    }
    else {
	sleep(1);
        alarm(NSECONDS);
        signal(SIGALRM, catchit);
        while(wait((int *)0) > 0);
    	alarm(0);
    	wt = NULL;
    }

*/
    i = select (MAXSOCKS, LastSelectMask, (int *) NULL, (int *) NULL, wt);

    selecterr = errno;
    
    if (i <= 0) /* An error or timeout occurred */
    {
	if(i == 0)
		Quit();
	if (i < 0) 
		if (selecterr == EBADF)    /* Some client disconnected */
			CheckConnections ();
		else if (selecterr != EINTR)
			fprintf(stderr, "WaitForInput(): select: errno=%d\n",
								selecterr);
    }
    else
    {
	MASKANDSETBITS(readyClients, LastSelectMask, AllClients); 
	if (LastSelectMask[0] & WellKnownConnections) 
		   EstablishNewConnections();
    }
    if (ANYSET(readyClients))
    {
	for (i=0; i<mskcnt; i++)
	{
	    while (readyClients[i])
	    {
		curclient = NextSetBit (readyClients[i]) - 1;
		ServiceClient(curclient);	
		readyClients[i] &= ~(1 << curclient);
	    }
	}	
    }
}

/* Routines for handling local streams (streams-pipes) */

Quit(sig)
int	sig;
{
    fprintf(stderr, "xntad: received signal %d\n", sig);

    if(unlink(NAME_SERVER_NODE) < 0 && errno != ENOENT){
		fprintf(stderr, "Cannot unlink %s", NAME_SERVER_NODE);
		perror(" ");
		}
    exit(0);
}

OpenTheListener()
{
	int 	munix, sunix;
	char *	slave;
	char	buf[64];

/*
	signal(SIGHUP, Quit);
*/
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, (void (*)())Quit);
	signal(SIGQUIT,(void (*)())Quit);
	signal(SIGTERM, (void (*)())Quit);

	sprintf(buf, "%s", NAME_SERVER_NODE);
	if(open(buf, O_RDWR) >= 0){
		fprintf(stderr, "xntad is already running\n");
		return(-1);
		}
	if( (munix = open(ptmx, O_RDWR)) < 0 ){
		fprintf(stderr,"Cannot open %s", ptmx);
		perror(" ");
		return(-1);
	}
	grantpt(munix);
	unlockpt(munix);

	if(unlink(buf) < 0 && errno != ENOENT){
		fprintf(stderr, "Cannot unlink %s", buf);
		perror(" ");
		return(-1);
		}

	slave = (char *) ptsname(munix);
	if( link(slave, buf) <0 ){
		fprintf(stderr, "Cannot link %s to %s", slave, buf);
		perror(" ");
		return(-1);
		}
	if( chmod(buf, 0666) < 0){
		fprintf(stderr, "Cannot chmod %s", buf);
		perror(" ");
		return(-1);
		}

	sunix = open(buf, O_RDWR);
	if(sunix < 0){
		fprintf(stderr, "Cannot open %s", buf);
		perror(" ");
		close(munix);
		return(-1);
		}
	setpgrp();
/*
	if(fcntl(munix, F_SETFL, FNDELAY) < 0)
	{
		fprintf(stderr, "Cannot set nodelay on the nameserver\n");
		exit(1);

	}
*/

	return(munix);
}

ConnectNewClients(lfd, MoreConnections)
int	lfd;
char	* MoreConnections;
{
	
	int fd;
	int read_in;
	char length;
	char buf[64];

	*MoreConnections = 0;

	if( (read_in = read(lfd, &length, 1)) <= 0 ){
		if( !read_in )  /* client closed fd */
			perror("0 bytes read");
		else	perror("Error in reading the local connection msg length");
		return(-1);
		}


	if( (read_in = read(lfd, buf, length)) <= 0 ){
		if( !read_in )  /* client closed fd */
			perror("0 bytes read");
		else	perror("Error in reading the local connection slave name");
		return(-1);
		}

	buf[ length ] = '\0';

	if( (fd = open(buf,O_RDWR|O_SYNC)) < 0 ){
		strcat(buf," open fail, clientfd");
		perror(buf);
		return(-1);
		}

	write(fd,SUCCESS,1);
	InitClientBuffers(fd);	

	return(fd);
}

CreateWellKnownSockets()
{
    CLEARBITS(AllSockets);
    CLEARBITS(AllClients);
    CLEARBITS(LastSelectMask);

    lastfdesc = ulimit(4, (long)0) - 1;  /* Returns total # of FDs available */
    if (lastfdesc > MAXSOCKS)
    {
	lastfdesc = MAXSOCKS;
    }

    WellKnownConnections = 0;
    if ((ListenFd = OpenTheListener()) < 0)
    {
       	if(ListenFd == -1)
        	return(-1);
    }
    else	WellKnownConnections |= (1<<ListenFd);

    if (WellKnownConnections == 0) {
        fprintf(stderr, "No Listeners, nothing to do\n");
        return(-1);
    }
    
    AllSockets[0] = WellKnownConnections;
    return(1);
}

EstablishNewConnections()
{
    long readyconnections;     /* mask of listeners that are ready */
    long newconn;                  /* fd of new client */
    char *reason;
    char MoreConnections;

    if (readyconnections = (LastSelectMask[0] & WellKnownConnections)) 
    {
      MoreConnections = 0;
      do
      {
	newconn = ConnectNewClients(ListenFd,&MoreConnections);
	if (newconn >= 0)
	{
		fcntl (newconn, F_SETFL, O_NDELAY);
		BITSET(AllClients, newconn);
		BITSET(AllSockets, newconn);
	}
      }
      while(MoreConnections);
    }
}

void
CloseDownFileDescriptor(connection)
    long connection;
{
    close(connection);
    InitClientBuffers(connection);
    BITCLEAR(AllSockets, connection);
    BITCLEAR(AllClients, connection);
}


CheckConnections()
{
    long		rmask[mskcnt]; 
    long		emask[mskcnt]; 
    register int	curclient;
    int			i;
#ifdef SVR4
    int			ret;
    struct timeval	waittime;
#endif

#ifdef DEBUG
fprintf(stderr, "Calling CheckConnections\n");
#endif

    COPYBITS(AllClients, rmask);
#ifndef SVR4
    COPYBITS(AllClients, emask);
    i = select (MAXSOCKS, rmask, (int *) NULL, emask, NULL);
    if (i <= 0)
        return;

    for (i=0; i<mskcnt; i++)
    {
        while (emask[i])
        {
            curclient = NextSetBit (emask[i]) - 1 + (i << 5);
#ifdef DEBUG
            printf("CheckConnection closing %d\n", curclient);
#endif
            InitClientBuffers(curclient);
            CloseDownFileDescriptor(curclient);
            BITCLEAR(emask, curclient);
        }
    }
#else	/* SVR4: */

    for (i=0; i<mskcnt; i++)
    {
        while (rmask[i])
    	{
	    curclient = NextSetBit (rmask[i]) - 1 + (i << 5);
            CLEARBITS(emask);
            BITSET(emask, curclient);
            waittime.tv_sec  = 0;
            waittime.tv_usec = 0;
            ret = select (curclient+1,emask,(int *)NULL,(int *)NULL,&waittime);
            if(ret < 0)
		{
#ifdef DEBUG
		    fprintf(stderr, "CheckConnection closing %d\n", curclient);
#endif
		    InitClientBuffers(curclient);
		    CloseDownFileDescriptor(curclient);
		}
	    BITCLEAR(rmask, curclient);
	}
    }	
#endif	/* SVR4 */
}
/* Find the first set bit
 * i.e. least signifigant 1 bit:
 * 0 => 0
 * 1 => 1
 * 2 => 2
 * 3 => 1
 * 4 => 3
 */
int NextSetBit(mask)
unsigned int	mask;
{
	register i;

	if ( ! mask ) return 0;
	i = 1;
	while (! (mask & 1)) {
		i++;
		mask >>= 1;
	}
	return i;
}

initBuffers()
{
	int	i;

	for(i=0; i< MAXSOCKS; i++)
	{
		InputBuffer[i].flags	= 0;
		InputBuffer[i].buflen	= 0;
		InputBuffer[i].bufptr	= 0;
		InputBuffer[i].inputbuf	= NULL;
	}
}

char	path[128];

SendNull(fd)
int	fd;
{

	char	*ptr, buf[32];

	ptr = buf;
	*(int *) ptr = 0;
	ptr += sizeof(int);
	*(int *) ptr = 0;
	write(fd, inbuf, 2*sizeof(int));
}

ServiceClient(fd)
int	fd;
{
	register IOBUFFER *iop = &InputBuffer[fd];
	int	n, m;
	char	*ptr, *net;

	if(iop->inputbuf ==  NULL)
	{
		iop->inputbuf = (char *) xalloc(BUFSIZE);
		iop->buflen	= BUFSIZE;
	}
	if(iop->bufptr < HEADERSIZE)
	{
	   errno = 0;
	   n = read(fd, &iop->inputbuf[iop->bufptr], iop->buflen - iop->bufptr);
	   if(n <= 0)
	   {
		if(errno == EAGAIN)
			return(0);
		else if(errno == EINTR){
			InitClientBuffers(fd);
			SendNull(fd);
		}
		else {
#ifdef DEBUG
                      	fprintf(stderr, "XNTAD: read error; errno %d\n",
                                                 errno);
#endif
			CloseDownFileDescriptor(fd);
		}
		return(-1);
	   }
	   iop->bufptr += n;
	   if(iop->bufptr < HEADERSIZE)
	   		return(0);
	   iop->msglen = *(int *) iop->inputbuf;
	}

	if(iop->buflen < iop->msglen)
	{
	   iop->inputbuf = (char *) xrealloc(iop->inputbuf, iop->msglen);
	   iop->buflen	= iop->msglen;
	}

	if(iop->bufptr < iop->msglen)
	{
           errno = 0;
	   n = read(fd, &iop->inputbuf[iop->bufptr], iop->msglen - iop->bufptr);
	   if(n <= 0)
	   {
                if(errno == EAGAIN)
                        return(0);
		else if(errno == EINTR){
                        InitClientBuffers(fd);
                        SendNull(fd);
		}
                else {
#ifdef DEBUG
                      	fprintf(stderr, "XNTAD: read error; errno %d\n",
                                                 errno);
#endif
                        CloseDownFileDescriptor(fd);
		}
		return(-1);
           }
           iop->bufptr += n;
	   if(iop->bufptr < iop->msglen)
                        return(0);
	}

	ptr = &iop->inputbuf[4*sizeof(int)];
	n = *(int *) ptr;
	ptr += sizeof(int);
	net  = ptr;
	if(ptr[n] != '\0')
	{
		fprintf(stderr, "XNTAD: net name error\n");
                SendNull(fd);
	}	
	ProcessRequest(fd, iop->inputbuf);
		
	InitClientBuffers(fd);
	return(1);
}


InitClientBuffers(fd)
int	fd;
{
       	InputBuffer[fd].flags	= 0;
	InputBuffer[fd].bufptr	= 0;
	InputBuffer[fd].msglen	= 0;
}

char *
xalloc(n)
int	n;
{
	char	*ptr;

	if((ptr = (char *) malloc(n)) == NULL)
	{
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}
	return(ptr);
}


char *
xrealloc(buf, n)
char	*buf;
int	n;
{
 	char	*ptr;

        if((ptr = (char *) realloc(buf, n)) == NULL)
	{
         	fprintf(stderr, "realloc failed\n");
		exit(1);
	}
        return(ptr);
}

char	*program;
main(argc, argv)
int	argc;
char	*argv[];
{
	int	i;
	program = argv[0];
	
	i = CreateWellKnownSockets();
/*
	if(argc == 2 && strcmp(argv[1], "-") == 0){
		sig = SIGALRM;
		}
	if(sig > 0)
		kill(getppid(), sig);
*/

	if(i < 0)
		exit(1);

	signal(SIGCLD, SIG_IGN);

	for(;;)
		WaitForInput();
}


static void   *handlep = NULL;
/* Routines for handling TLI streams */

static int
InitializeNetPath(path, n)
char	*path;
{
	static	char	*netpath = NULL;
	static	int	netpathlen = 0;
	char	*ptr;


	if(netpath == NULL)
	{
		netpathlen = 512;
		netpath	   = (char *) malloc(netpathlen);
		netpath[0] = '\0';
	}
	else if(n > netpathlen)
	{
		netpathlen = n;
		netpath    = (char *) realloc(netpath, netpathlen);
	}
	if(netpath == NULL)
		return(-1);

	if(n > 0)
	{
		sprintf(netpath, "NETPATH=%s", path);
		if(putenv(netpath))
			return(-1);
	}
	else	netpath[0] = '\0';
	if((ptr = (char *) getenv("NETPATH")) == NULL)
		ptr = "(Null)";
#ifdef DEBUG
fprintf(stderr, "Callin setnetpath with netpath (%s)  and getenv%s\n", 
			netpath, ptr);
#endif
	if(handlep != NULL)
	{
		endnetpath(handlep);
	}
	if((handlep = setnetpath()) == NULL)
	{
		nc_perror("Cannot set network selection path\n");
		return(-1);
	}
	return(1);

}



ProcessRequest(fd, inputbuf)
int	fd;
char	*inputbuf;
{
	int	n,m, len;
	char	*ptr, *net;

        ptr = &inputbuf[sizeof(int)];
	m = *(int *) ptr;
	ptr += sizeof(int);

        flags = *(int *) ptr;
	ptr += sizeof(int);

        dispno = *(int *) ptr;

        ptr += sizeof(int);
	n = *(int *) ptr;

        ptr += sizeof(int);
	net  = ptr;

        ptr = &inputbuf[m];
	inlen = *(int *) ptr;

        ptr += sizeof(int);
	nhosts = *(int *) ptr;

        inbuf = ptr + sizeof(int);
	TheEnd = &inbuf[inlen];

        nextentry = ((xHostEntry *) inbuf)->length;
	if(InitializeNetPath(net, n) < 0)
	{
		SendNull(fd);
		return;
	}
        ptr = (char *) makePacket();
	
	if(ptr != NULL)
	{
		write(fd, ptr, (*(int *) ptr) + 2*sizeof(int));
#ifdef DEBUG
		write(2, ptr, (*(int *) ptr) + 2*sizeof(int));
		fprintf(stderr, "Sending %d chars with %d hosts\n",
			(*(int *) ptr) + 2*sizeof(int), 
			*(int *) (ptr+sizeof(int)));
#endif
	}
	else   SendNull(fd);
}





int	bufsize = 512;

char	*getnextentry();

struct nd_addrlist *
GetHostServiceByName(host, dispaly)
char	*host;
int	dispaly;
{
	struct nd_hostserv  nd_hostserv;
	struct nd_addrlist *nd_addrlistp = NULL;
	struct netbuf	   *netbufp = NULL;
	char	service[64];

        sprintf(service , "xserver%d", dispaly);
	nd_hostserv.h_host = host;
	nd_hostserv.h_serv = service;

        if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0)
         	return(nd_addrlistp);
	else	return(NULL); 
}

int
ConvertName(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{
 	struct hostent *hp;
	unsigned long	address;
	int	port;
        char    *ptr;
	int	entlen = 8; 
	int	rndlen;
	struct nd_addrlist *nd_addrlistp = NULL;
        struct netbuf   *netbufp;
        char   *addr;
#ifdef DEBUG
fprintf(stderr, "in ConvertName %s\n", entry);
#endif
	if((nd_addrlistp = GetHostServiceByName(entry, dispno)) == NULL)
		return(n);
        netbufp = nd_addrlistp->n_addrs;       /* the netbufs */

	if(	
		strcmp(netconfigp->nc_protofmly, "inet") == 0 && 
		strcmp(netconfigp->nc_proto    , "tcp") == 0
	  ){
		addr = &netbufp->buf[4];
		len = 4;
	}
	else
	{
		addr = netbufp->buf;
		len = netbufp->len;
	}
	rndlen = ((sizeof(xHostEntry) + len + 3) >> 2) << 2;

	if((*pktptr = alialloc(*pktptr, n+rndlen)) == NULL)
	{
		netdir_free(nd_addrlistp, ND_ADDRLIST);
		return(-1);
	}

	ptr = &(*pktptr)[n];
	((xHostEntry *)ptr)->family = 0;
	((xHostEntry *)ptr)->length = len;
	ptr += sizeof(xHostEntry);

	memcpy(ptr, addr, len);
	netdir_free(nd_addrlistp, ND_ADDRLIST);

#ifdef DEBUG
ptr[len] = '\0';
fprintf(stderr, "creating address for host %s address<%d>\n", entry, ptr);
#endif

	return(n+rndlen);
}

struct nd_hostservlist *
GetHostServiceByAddr(addr, len)
char	*addr;
int	len;
{
	struct nd_hostservlist *nd_hostservlist;
	struct netbuf	   netbuf;

	netbuf.buf = addr;
	netbuf.len = len;
	netbuf.maxlen = len;
	
        if(netdir_getbyaddr(netconfigp, &nd_hostservlist, &netbuf) == 0)
		return(nd_hostservlist);
	else	return(NULL);
}


int
ConvertCallToName(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{
 	int	l, rl;
	char	*ptr;
	struct nd_hostservlist *nd_hostservlist;

	if((nd_hostservlist = GetHostServiceByAddr(entry, len)) == NULL)
		return(n);

	l = strlen(nd_hostservlist->h_hostservs->h_host);

        rl = ((sizeof(xHostEntry) + l + 3) >> 2) << 2;

        if((*pktptr = alialloc(*pktptr, n+rl)) == NULL)
	{
		netdir_free(nd_hostservlist, ND_HOSTSERVLIST);
		return(-1);
	}

        ptr = &(*pktptr)[n];
	((xHostEntry *)ptr)->family = 1;
	((xHostEntry *)ptr)->length = l;

	ptr += sizeof(xHostEntry);

	sprintf(ptr, nd_hostservlist->h_hostservs->h_host);
	netdir_free(nd_hostservlist, ND_HOSTSERVLIST);

#ifdef DEBUG
fprintf(stderr, "getting the name for host %s\n", ptr);
#endif

        return(rl+n);
}

int
ConvertAddress(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{
	register i;
 	char	*ptr;
        int     l, rl; 
	struct nd_hostservlist *nd_hostservlist;
	char	*name;
	char	tcpaddr[8], *addr;

	if(
		strcmp(netconfigp->nc_protofmly, "inet") == 0 &&
		strcmp(netconfigp->nc_proto    , "tcp") == 0
	  ){
		addr = tcpaddr;
		memset(tcpaddr, '\0', 4);
		memcpy(&tcpaddr[4], entry, 4);
		len = 8;
	}
	else
	{
		addr = entry;
	}

        if((nd_hostservlist = GetHostServiceByAddr(addr, len)) == NULL)
		return(n);
	name = nd_hostservlist->h_hostservs->h_host;
        l = strlen(name);

        rl = ((sizeof(xHostEntry) + l + 3) >> 2) << 2;

        if((*pktptr = alialloc(*pktptr, n+rl)) == NULL)
	{
		netdir_free(nd_hostservlist, ND_HOSTSERVLIST);
                return(-1);
	}

        ptr = &(*pktptr)[n];
        ((xHostEntry *)ptr)->family = 0;
        ((xHostEntry *)ptr)->length = l;
        ptr += sizeof(xHostEntry);

  	memcpy(ptr, name, l);	
	netdir_free(nd_hostservlist, ND_HOSTSERVLIST);

#ifdef DEBUG
fprintf(stderr, "getting the name for host %s\n", name);
#endif
	
	return(n+rl);
}

char	*
getnextentry(plen)
int	*plen;
{
	char	*ptr;
	int	n = nextentry;

#ifdef DEBUG
fprintf(stderr,"In getnextentry()\n");
#endif
	if(inbuf >= TheEnd)
	{
		*plen = -1;
		return(NULL);	
	}

	*plen = nextentry;
        ptr = inbuf + sizeof(xHostEntry);
	inbuf += ((sizeof(xHostEntry) + *plen + 3) >> 2) << 2;
	nextentry = ((xHostEntry *) inbuf)->length;
	ptr[*plen] = '\0';
	return(ptr);
}

char *
makePacket()
{
    static char *pktptr = NULL, *ptr;
    int len;
    int n, m;

    n = sizeof(int) * 2;
    if(pktptr == NULL)
    {
    	pktptr = (char *) malloc(bufsize);
    }

#ifdef DEBUG
fprintf(stderr,"In makePacket()\n");
#endif

    if(pktptr == NULL)
	return(NULL);
   if(flags == ConvertNameToTliCall)
    {
	nHosts = 0;
	ptr = getnextentry(&len);
	while((m = MakeTliCall(&pktptr, n, ptr, len)) > n)
	{
		nHosts++;
		n = m;
	}

    }
    else
    for(nHosts = 0; nHosts < nhosts;)
    {
	ptr = getnextentry(&len);
	if(len < 0)
		break;
	if(len == 0 || ptr == NULL)
		continue;
	m = addentry(&pktptr, n, ptr, len);
	if(m > n){
		nHosts++;
		n = m;
		}
    }
#ifdef DEBUG
    fprintf(stderr, "packet size is %d\n", n);
#endif

    *(int *) pktptr = n - 2*sizeof(int);
    *(int *) (pktptr+sizeof(int)) = nHosts;

#ifdef DEBUG
    fprintf(stderr, "packet size is %d and nHosts to be returned is %d %d\n", 
			n, nHosts, *(int *) (pktptr+sizeof(int)));
#endif
    return(pktptr);
}

int
addentry(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{

#ifdef DEBUG
	fprintf(stderr, "in addEntry %s\n", entry);
#endif

	switch(flags)
	{
		case	ConvertNameToNetAddr:
			return(ConvertName(pktptr, n, entry, len));
		case	ConvertNetAddrToName:
			return(ConvertAddress(pktptr, n, entry, len));
		case	ConvertTliCallToName:
			return(ConvertCallToName(pktptr, n, entry, len));
	}
	return(-1);
}

char *
alialloc(ptr, size)
char	*ptr;
int	size;
{

	if(bufsize < size)
	{
		bufsize = size + 512;
		if(ptr == NULL)
			ptr = (char *) malloc(bufsize);
		else ptr = realloc(ptr, bufsize);
	}
	return(ptr);
}

int
MakeTliCall(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{
	char	*host;
	int	retval;
	char	service[64];
	int	fd;
	struct nd_hostserv  nd_hostserv; 
	struct netconfig   *netconfigp = NULL;
	struct nd_addrlist *nd_addrlistp = NULL;
	struct netbuf	   *netbufp = NULL;
 

	host = entry;
	entry[len] = '\0';

#ifdef DEBUG
fprintf(stderr, "Calling MakeTliCall(%s, %d)\n", host, len);
#endif

        sprintf(service , "xserver%d", dispno);
	nd_hostserv.h_host = host;
	nd_hostserv.h_serv = service;

#ifdef DEBUG
fprintf(stderr, "Trying to get the binding address for service %s on %s\n", 
					service, host);
#endif

	while((netconfigp = getnetpath(handlep)) != NULL)
	{
#ifdef DEBUG
	  fprintf(stderr, "Trying to bind using %s\n", netconfigp->nc_device);
#endif
	  if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0)
	  {
            netbufp = nd_addrlistp->n_addrs;
            if(nd_addrlistp->n_cnt > 0)
            {
		
#ifdef DEBUG
		fprintf(stderr, "Address: len %d maxlen %d \n", 
			netbufp->len, netbufp->maxlen);

#endif
		if( strcmp(netconfigp->nc_netid, "starlan") >= 0 )
		{
			register char *f, *t;
			int	i, l;

			f = t = netbufp->buf;
			l = 0;
			for(i=0; i< netbufp->len; i++, f++)
				if(*f != '.')
				{
					*t++ = *f;	
					l++;
				}
			*t = '\0';
			netbufp->len = l;
		}
		
#ifdef DEBUG
		netbufp->buf[netbufp->len] = '\0';
		fprintf(stderr, "Address: len %d maxlen %d buf %s\n", 
			netbufp->len, netbufp->maxlen, netbufp->buf);
#endif
		retval = CopyEntry(pktptr, n, netbufp, netconfigp);

	  	/* free(nd_addrlistp) the right way */
		netdir_free(nd_addrlistp, ND_ADDRLIST);
	
		return(retval);
            }
	    netdir_free(nd_addrlistp, ND_ADDRLIST);
	  }
	  else nc_perror("netdir_getbyname() failed\n");
	}
	return(-1);
}

int
CopyEntry(pktptr, n, netbufp, netconfigp)
char	**pktptr;
int	n;
struct  netbuf *netbufp;
struct netconfig   *netconfigp;
{
	char	*ptr;
	int	rndlen;
	int	ra, ro, ru;
	int	a, o, u;
	char	*netid, *device;

#ifdef DEBUG
fprintf(stderr, "in CopyEntry netid %s, netdevice %s\n", 
			netconfigp->nc_netid, netconfigp->nc_device);
#endif
	netid = netconfigp->nc_netid;
	device = netconfigp->nc_device;

#ifdef DEBUG
fprintf(stderr, "in CopyEntry netid %s, netdevice %s\n", 
			netid, device);
#endif

        a  = netbufp->len;
	o  = strlen(netid) + 1;
	u  = strlen(device) + 1;

        ra = ((a + sizeof(xHostEntry) + 3) >> 2) << 2;
	ro = ((o + sizeof(xHostEntry) + 3) >> 2) << 2;
	ru = ((u + sizeof(xHostEntry) + 3) >> 2) << 2;

        rndlen = ra + ro + ru;

        if((*pktptr = alialloc(*pktptr, n+rndlen)) == NULL)
		return(-1);

	ptr = &(*pktptr)[n];
	((xHostEntry *)ptr)->length = a;
	ptr += sizeof(xHostEntry);
	memcpy(ptr, netbufp->buf, a);

	ptr = &(*pktptr)[n+ra];
	((xHostEntry *)ptr)->length = o;
	ptr += sizeof(xHostEntry);
	memcpy(ptr, netid, o);

	ptr = &(*pktptr)[n+ra+ro];
	((xHostEntry *)ptr)->length = u;
	ptr += sizeof(xHostEntry);
	memcpy(ptr, device, u);

	write(2, &(*pktptr)[n], rndlen);
	return(n+rndlen);
}
