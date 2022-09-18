/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:os/access.c	1.7"
/*************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

******************************************************************/

/* $XConsortium: access.c,v 1.44 89/11/12 15:38:59 rws Exp $ */
#include "Xos.h"
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include <errno.h>
# include <sys/utsname.h>

#include <stdio.h>
#include "dixstruct.h"
#include "osdep.h"


#include "Xstreams.h"
#include "site.h"

#include <netconfig.h>

#define	FILENO(who) (((OsCommPtr)who->osPrivate)->fd)
static int nvalidhosts = 0;
static char	*ourself = NULL;

static int  CheckAddr();
static void NewHost();


int AccessEnabled = DEFAULT_ACCESS_CONTROL;

extern char TypeOfStream[];
extern Xstream xstream[];
extern char **addheader();

static HOST *selfhosts = NULL;
static int LocalHostEnabled = FALSE;
static int UsingXdmcp = FALSE;

/*
 * called when authorization is not enabled to add the
 * local host to the access list
 */

EnableLocalHost ()
{
    if (!UsingXdmcp)
    {
	LocalHostEnabled = TRUE;
	AddLocalHosts ();
    }
}

#ifdef XDMCP 	/* funNotUsedByATT, AccessUsingXdmcp, DefineSelf */

/*
 * called at init time when XDMCP will be used; xdmcp always
 * adds local hosts manually when needed
 */

AccessUsingXdmcp ()
{
    UsingXdmcp = TRUE;
    LocalHostEnabled = FALSE;
}

/* Define this host for access control.  Find all the hosts the OS knows about 
 * for this fd and add them to the selfhosts list.
 */
DefineSelf (fd)
    int fd;
{
}

#define FAMILIES ((sizeof familyMap)/(sizeof familyMap[0]))

#if defined (hpux) && ! defined (HAS_IFREQ)
/* Define this host for access control.  Find all the hosts the OS knows about 
 * for this fd and add them to the selfhosts list.
 * HPUX version - hpux does not have SIOCGIFCONF ioctl;
 */
DefineSelf (fd)
    int fd;
{
    register int n;
    int	len;
    caddr_t	addr;
    int		family;
    register HOST	*host;

    struct utsname name;
    register struct hostent  *hp;

    union {
	struct  sockaddr   sa;
	struct  sockaddr_in  in;
    } saddr;
	
    struct	sockaddr_in	*inetaddr;

    /* Why not use gethostname()?  Well, at least on my system, I've had to
     * make an ugly kernel patch to get a name longer than 8 characters, and
     * uname() lets me access to the whole string (it smashes release, you
     * see), whereas gethostname() kindly truncates it for me.
     */
    uname(&name);
    hp = gethostbyname (name.nodename);
    if (hp != NULL)
    {
	saddr.sa.sa_family = hp->h_addrtype;
	inetaddr = (struct sockaddr_in *) (&(saddr.sa));
	acopy ( hp->h_addr, &(inetaddr->sin_addr), hp->h_length);
	len = sizeof(saddr.sa);
	family = ConvertAddr ( &(saddr.sa), &len, &addr);
	if ( family > 0)
	{
	    for (host = selfhosts;
		 host && !addrEqual (family, addr, len, host);
		 host = host->next) ;
	    if (!host)
	    {
		/* add this host to the host list.	*/
		host = (HOST *) xalloc (sizeof (HOST));
		if (host)
		{
		    host->family = family;
		    host->len = len;
		    acopy ( addr, host->addr, len);
		    host->next = selfhosts;
		    selfhosts = host;
		}
	    }
	}
    }
}

#else
/* Define this host for access control.  Find all the hosts the OS knows about 
 * for this fd and add them to the selfhosts list.
 */
DefineSelf (fd)
    int fd;
{
    char		buf[2048];
    struct ifconf	ifc;
    register int	n;
    int 		len;
    pointer 		addr;
    int 		family;
    register HOST 	*host;
    register struct ifreq *ifr;
    
#ifdef DNETCONN
    struct dn_naddr *dnaddr = getnodeadd(0);
    /*
     * AF_DECnet may not be listed in the interface list.  Instead use
     * the supported library call to find out the local address (if any).
     */
    if (dnaddr)
    {    
	addr = (pointer) dnaddr;
	len = dnaddr->a_len + sizeof(dnaddr->a_len);
	family = AF_DECnet;
	for (host = selfhosts;
	     host && !addrEqual (family, addr, len, host);
	     host = host->next)
	    ;
        if (!host)
	{
	    host = (HOST *) xalloc (sizeof (HOST));
	    if (host)
	    {
		host->family = family;
		host->len = len;
		acopy(addr, host->addr, len);
		host->next = selfhosts;
		selfhosts = host;
	    }
	}
    }
#endif
    ifc.ifc_len = sizeof (buf);
    ifc.ifc_buf = buf;
    if (ioctl (fd, (int) SIOCGIFCONF, (pointer) &ifc) < 0)
        Error ("Getting interface configuration");
    for (ifr = ifc.ifc_req, n = ifc.ifc_len / sizeof (struct ifreq); --n >= 0;
     ifr++)
    {
	len = sizeof(ifr->ifr_addr);
#ifdef DNETCONN
	/*
	 * DECnet was handled up above.
	 */
	if (ifr->ifr_addr.sa_family == AF_DECnet)
	    continue;
#endif /* DNETCONN */
        if ((family = ConvertAddr (&ifr->ifr_addr, &len, &addr)) <= 0)
	    continue;
        for (host = selfhosts;
 	     host && !addrEqual (family, addr, len, host);
	     host = host->next)
	    ;
        if (host)
	    continue;
        host = (HOST *) xalloc (sizeof (HOST));
	if (host)
	{
	    host->family = family;
	    host->len = len;
	    acopy(addr, host->addr, len);
	    host->next = selfhosts;
	    selfhosts = host;
	}
#ifdef XDMCP
	{
	    struct sockaddr broad_addr;

	    /*
	     * If this isn't an Internet Address, don't register it.
	     */
	    if (family != AF_INET)
		continue;

	    XdmcpRegisterConnection (FamilyInternet, (char *)addr, len);
	    broad_addr = ifr->ifr_addr;
	    ((struct sockaddr_in *) &broad_addr)->sin_addr.s_addr =
		htonl (INADDR_BROADCAST);
#ifdef SIOCGIFBRDADDR
	    {
	    	struct ifreq    broad_req;
    
	    	broad_req = *ifr;
		if (ioctl (fd, SIOCGIFFLAGS, (char *) &broad_req) != -1 &&
		    (broad_req.ifr_flags & IFF_BROADCAST) &&
		    (broad_req.ifr_flags & IFF_UP)
		    )
		{
		    broad_req = *ifr;
		    if (ioctl (fd, SIOCGIFBRDADDR, &broad_req) != -1)
			broad_addr = broad_req.ifr_addr;
		    else
			continue;
		}
		else
		    continue;
	    }
#endif
	    XdmcpRegisterBroadcastAddress ((struct sockaddr_in *) &broad_addr);
	}
#endif
    }
}
#endif /* hpux && !HAS_IFREQ */

void
AugmentSelf(fd)
    int fd;
{
    int len;
    struct sockaddr from;
    int family;
    pointer addr;
    register HOST *host;

    len = sizeof(from);
    if (getpeername(fd, &from, &len))
	return;
    family = ConvertAddr(&from, &len, &addr);
    if (family <= 0)
	return;
    for (host = selfhosts; host; host = host->next)
    {
	if (addrEqual(family, addr, len, host))
	    return;
    }
    host = (HOST *)xalloc(sizeof(HOST));
    if (!host)
	return;
    host->family = family;
    host->len = len;
    acopy(addr, host->addr, len);
    host->next = selfhosts;
    selfhosts = host;
}


#endif	/* XDMCP */

AddLocalHosts ()
{
    HOST    *self;

    for (self = selfhosts; self; self = self->next)
	NewHost (self->host_name);
}

/* Reset access control list to initial hosts */
ResetHosts (display)
    char *display;
{
     register HOST	*host;
     FILE    *f;
     char    *ptr;
     char    line[128];
     extern  char    *erazeComment();
     char    buf[128];
     static struct  utsname machine;
     int     n;


     while (host = validhosts)
     {
        validhosts =(HOST *) host->next;
        Xfree (host);
     }
     if(ourself == NULL)
     { 
       	if(uname(&machine) != -1)
		ourself = machine.nodename;
	else	ourself = "unix";
     }
     NewHost(ourself);

/*
     if(NetworkInfo->_nnets > 1)
     {
*/

	sprintf(buf, "/etc/X%s.hosts", display);
	if ((f = fopen(buf, "r")) == NULL)
	{
#ifdef DEBUG
	     fprintf(stderr, "Cannot open %s\n", buf);
#endif
	     return;
	}
	else	
                while(fgets(line, 128, f) != NULL)
	        {
         	        if((n = strlen(line)) > 1)
                                line[n-1] = '\0';
                        if((ptr = erazeComment(line)) != NULL)
                        {
                   	    if(sscanf(ptr, "%s", buf) >= 1)
	        			NewHost(buf);
	        	}
	        }
	fclose(f);
/*
     }
*/
}

Bool
AuthorizedClient(fd)
    int    fd;
{
    char	*hname;
    register HOST	*host;

/* Enhanced Application Compatibility Support */
/* TypeOfStream == local */

    if(TypeOfStream[fd] < X_TLI_STREAM ||
	 !AccessEnabled)
		return TRUE;
/* End Enhanced Application Compatibility Support */

    if(GetNetworkInfo (fd, NULL, PEER_NAME, NULL, &hname) < 0)
    {
    	return FALSE;
    }

    for (host = validhosts; host; host = host->next)
    {
/************
	if(strcmp(hname, host->host_name) == 0){
		    return TRUE;
		    }
*************/
	if (strcmp(NetworkInfo->_net[TypeOfStream[fd]]->nc_netid,"spx") == 0 )
	{
		if(strcmpi(hname, host->host_name) == 0)
			return TRUE;
	}
	else {
		if(strcmp(hname, host->host_name) == 0)
				return TRUE;
	}
    }

    return FALSE;
}

/* Add a host to the access control list.  This is the external interface
 * called from the dispatcher */

int
AddHost (client, family, length, pAddr)
    ClientPtr		client;
    int                 family;
    unsigned            length;        /* of bytes in pAddr */
    pointer             pAddr;
{
    int			len;
    register HOST	*host;
    int                 unixFamily;
#ifdef SVR4
    struct netconfig *nettype;
#else
    char *nettype;
#endif
    char		*packet, *hname;
    int			type, fd;


    if (!AuthorizedClient(FILENO(client)))
	return(BadAccess);

    fd = FILENO(client);
    type = TypeOfStream[fd];
    nettype = NetworkInfo->_net[type]; 
    hname = (char *) pAddr;
    packet = NULL;
    

/* only allow change hostaccesslist at local */
/* type != local */

    if(type >= X_TLI_STREAM)
	return(BadAccess);
    
    NewHost(hname); 

    if(packet != NULL)
    		Xfree(packet);
    return(Success);
}


/* Add a host to the access control list. This is the internal interface 
 * called when starting or resetting the server */

static void
NewHost (hname)
    char *hname;
{
    int		len;
    register HOST *host;

    for (host = validhosts; host; host = host->next)
    {
    	if (strcmp(hname, host->host_name) == 0)
    	return;
    }
    host = (HOST *) Xalloc (sizeof (HOST));
    host->host_len = strlen(hname) + 1;
    bcopy(hname, host->host_name, host->host_len);
    host->next = validhosts;
    validhosts = (HOST *) host;
}
/* Remove a host from the access control list */


int
RemoveHost (client, family, length, pAddr)
    ClientPtr		client;
    int                 family;
    unsigned            length;        /* of bytes in pAddr */
    pointer             pAddr;
{
    register HOST	*host, **prev;
    char	*hname, *packet;
    int		type;
#ifdef SVR4
    struct netconfig *nettype;
#else
    char *nettype;
#endif
    int		fd = FILENO(client);

    if (!AuthorizedClient(FILENO(client)))
	return(BadAccess);

    type = TypeOfStream[fd];
    nettype = NetworkInfo->_net[type];
   
    hname = (char *) pAddr;
    packet = NULL;

/* type != local */

    if((type >= X_TLI_STREAM ))
	return (BadAccess);

/*
    if(GetNetworkInfo (fd, nettype, ConvertNetAddrToName,
	addheader(pAddr, length), &packet, &length) < 0)
			return(BadAccess);
    hname = sizeof(xHostEntry) + packet;
    hname[((xHostEntry *) packet)->length] = '\0';
*/

    if(strcmp(hname, ourself) == 0)
	return(Success);

    for (prev = &validhosts;
         (host = *prev) && ( strcmp(hname, host->host_name) != 0);
         prev = &host->next)
        ;
    if (host)
    {
        *prev = host->next;
        Xfree (host);
    }
    if(packet != NULL)
    		Xfree(packet);
    return(Success);
}

/* Get all hosts in the access control list */
int
GetHosts (client, data, pnHosts, pLen, pEnabled)
    ClientPtr		client;
    pointer		*data;
    int			*pnHosts;
    int			*pLen;
    BOOL		*pEnabled;
{
    register int 	n = 0, m;
    char		*ptr = NULL, *packet;
    register HOST	*host;
    int			nhosts = 0;
    int			type, fd;
#ifdef SVR4
    struct netconfig *nettype;
#else
    char *nettype;
#endif
    int			buflen = 128;

    fd = FILENO(client);
    type = TypeOfStream[fd];
    nettype = NetworkInfo->_net[type];

    *pEnabled = AccessEnabled ? EnableAccess : DisableAccess;
    m = 2 * sizeof(int);
    packet = (char *) Xalloc (buflen);
    ptr = &packet[m];

    for (host = validhosts; host; host = host->next)
    {
	n = (((host->host_len + 3) >> 2) << 2) + sizeof(xHostEntry);
	m += n;
        if(m > buflen){
		buflen = m + 128;
		packet = (char *) xrealloc(packet, buflen);
		}
	ptr = &packet[m - n];	
	((xHostEntry *) ptr)->length  = host->host_len;
	((xHostEntry *) ptr)->family  = FamilyUname;
	ptr += sizeof(xHostEntry);
	bcopy (host->host_name, ptr, host->host_len);
	nhosts++;
    }

/* TypeOfStream == local */

    if((TypeOfStream[fd] < X_TLI_STREAM))
    {
	n = m - 2*sizeof(int);
	*pnHosts = nhosts;
	*data = (pointer) Xalloc(n);
	memcpy(*data, &packet[2*sizeof(int)], n);
	Xfree(packet);
	*pLen = n;
	return(Success);
    }

    n = 0;
    *pnHosts = 0;

    if(m > 2 * sizeof(int))
    {
	*(int *) packet = m;
	*(int *) (packet + sizeof(int)) = nhosts; 
	n = GetNetworkInfo (fd, nettype, ConvertNameToNetAddr, &packet, data, pnHosts);
	if( n <= 0)
        {
    		*pnHosts = 0;
		*data = NULL;
		n = 0;
        }
    }

    Xfree(packet);
    *pLen = n;
    return (Success);
}

#ifndef i386	/* funNotUsedByATT, CheckAddr, InvalidHost */

/* Check for valid address family and length, and return address length. */

/*ARGSUSED*/
static int
CheckAddr (family, pAddr, length)
    int			family;
    pointer		pAddr;
    unsigned		length;
{
    int	len;

    switch (family)
    {
#ifdef TCPCONN
      case AF_INET:
	if (length == sizeof (struct in_addr))
	    len = length;
	else
	    len = -1;
        break;
#endif 
#ifdef DNETCONN
      case AF_DECnet:
        {
	    struct dn_naddr *dnaddr = (struct dn_naddr *) pAddr;

	    if ((length < sizeof(dnaddr->a_len)) ||
		(length < dnaddr->a_len + sizeof(dnaddr->a_len)))
		len = -1;
	    else
		len = dnaddr->a_len + sizeof(dnaddr->a_len);
	    if (len > sizeof(struct dn_naddr))
		len = -1;
	}
        break;
#endif
      default:
        len = -1;
    }
    return (len);
}

/* Check if a host is not in the access control list. 
 * Returns 1 if host is invalid, 0 if we've found it. */


InvalidHost (hname)
char	*hname;
{
    register HOST 		*host;
    if (!AccessEnabled)   /* just let them in */
        return(0);    
    for (host = validhosts; host; host = host->next)
    {
        if (strcmp(hname, host->host_name) == 0)
    	    return (0);
    }
    return (1);
}

#endif	/* i386, funNotUsedByATT */

int
ChangeAccessControl(client, fEnabled)
    ClientPtr client;
    int fEnabled;
{
    if (!AuthorizedClient(FILENO(client)))
	return BadAccess;
    AccessEnabled = fEnabled;
    return Success;
}


char	*erazeComment(line)
char	*line;
{
 	char	*ptr = line;
	
 	while(*ptr <= ' ' && *ptr != '\0')
                        ptr++;
/*	
 *	If you want to check the version do it here 
 *if( strncmp(ptr, "#VERSION", 8) == 0)
 *                      return(NULL);
 */
	if(*ptr == '\0' || *ptr == '#'){
                        return(NULL);
                        }
        line = ptr;
	while(*ptr != '\0' && *ptr != '#')
                        ptr++;
	*ptr = '\0';
	return(line);
}

/* code eliminated from here, find in Xlib 

static	int	_hlen = 0;
static	char	*_hptr = NULL;

static char	**
addheader(string, len)
char	*string;
int	len;
{

	int	n, m, p;
	char	*ptr;

        n = len;                                                                        m = n + sizeof(xHostEntry);
	p = m + 2 * sizeof(int);

        if(p > _hlen){
		if(_hptr == NULL)
                        _hptr = (char *) Xalloc(p);
		else	_hptr = (char *) Xrealloc(_hptr, p);
		}
	if(_hptr == NULL){
		fprintf(stderr, "addheader(): malloc failed\n");
		attexit(1);
		}
	else if(p > _hlen)
		_hlen = p;

        ptr = _hptr;

        *(int *) ptr = m;
	ptr += sizeof(int);
	*(int *) ptr = 1;
	ptr += sizeof(int);

        ((xHostEntry *) ptr)-> length = n;
	ptr += sizeof(xHostEntry);
	memcpy(ptr, string, n);

        return(&_hptr);
}

*/

/*
 * Case insensitive string compare 
 *     Could have done a table compare ala strcasecmp
 *     but I think size is more important than speed
 *     in this case. Save 276 bytes this way.
 */
strcmpi(const char *string1, const char *string2)
{
   while(*string1 != '\0')
   {
	 if (	(*string1 != (*string2)) &&
	 	(*string1 != toupper(*string2)) &&
	 	(*string1 != tolower(*string2)))
	 {
	 	return(*string1 - *string2);
	 }
	 string1++ , string2++;
   }
   return(0);
}
