/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XConnIM.c	1.1"
/*
 * $XConsortium: XConnIM.c,v 1.16 91/08/12 17:22:22 rws Exp $
 */

/*
 * Copyright 1990, 1991 by OMRON Corporation
 * Copyright 1991 by the Massachusetts Institute of Technology
 *
 *
 * OMRON AND MIT DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * EVENT SHALL OMRON OR MIT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *
 *	Author:	Seiji Kuwari	OMRON Corporation
 *				kuwa@omron.co.jp
 *				kuwa%omron.co.jp@uunet.uu.net
 */				

/*
 * This is an OS dependent file. this should work on 4.3BSD.
 */
#include "Xlibint.h"
#include "Xlibnet.h"
#include "Xi18nint.h"
#include "XIMlibint.h"
#include <X11/Xos.h>
#ifdef TCPCONN
#include <sys/socket.h>
#endif

#ifdef	UNIXCONN
#include <sys/un.h>
#ifndef	XIM_UNIX_PATH
#define	XIM_UNIX_PATH	"/tmp/.X11-unix/XIM"
#endif	/* XIM_UNIX_PATH */
#endif	/* UNIXCONN */

/*
 * Attempts to connect to an input manager, given atom id.
 */
Bool
_XipConnectIM (im, im_atom, displaybuf)
    XipIM im;
    Atom im_atom;
    char *displaybuf;
{
#ifdef STREAMSCONN
    im->fd = -1;
    return(False);
#else
#ifdef	UNIXCONN
    char		hostname[256];	/* My host name buffer */
#endif
    char		im_hostname[256];/* Input manager host name buffer */
    Atom		actual_type;
    int			actual_format;
    unsigned long	nitems;
    unsigned long	byte_after;
    unsigned char	*prop;
#ifdef	UNIXCONN
    struct sockaddr_un	saddr;		/* UNIX domain socket address */
#endif	/* UNIXCONN */
#ifdef TCPCONN
    struct sockaddr_in	saddr_in;	/* INET domain socket address */
    struct hostent	*hp;
    unsigned short	port;
#endif
    int			sd = -1;	/* File disclipter */
    int			indian;
    ximConnClient	client;
    ximNormalReply	reply;
    unsigned long	i;
    unsigned short	s;

    im->fd = -1;
    /*
     * Get a property of input manager. Format of the property is
     *		char hostname[128] ( host name )
     *		unsigned short portnumber    ( port number )
     *		long major_version    ( major version of protocol )
     *		long minor_version    ( minor version of protocol )
     */
    XGetWindowProperty(im->core.display, DefaultRootWindow(im->core.display),
		       im_atom, 0L, 256L, 0, AnyPropertyType,
		       &actual_type, &actual_format, &nitems,
		       &byte_after, &prop);
    bcopy((char *)prop, im_hostname, hname_size);
    bcopy((char *)(prop + offset_of_portnumber), (char *)&s, portnumber_size);
#ifdef TCPCONN
    port = ntohs(s);
#endif
    bcopy((char *)(prop + offset_of_version), (char *)&i, version_size);
    im->major_version = (long)ntohl(i);
    bcopy((char *)(prop + offset_of_minor_version), (char *)&i, version_size);
    im->minor_version = (long)ntohl(i);
    if (im->major_version != XIM_MAJOR_VERSION) return(False);
    if (!(im->minor_version >= XIM_MINOR_VERSION)) return(False);

#ifdef	UNIXCONN
    if (gethostname(hostname, 256) < 0) {
	hostname[0] = '\0';
    }
    /*
     * If the hostname of gethostname is null or the hostname of the input
     * manager is null, attempts to open UNIX domain socket.
     */
    if ((hostname[0] == '\0') || (im_hostname[0] == '\0')) {
	saddr.sun_family = AF_UNIX;
	strcpy(saddr.sun_path, XIM_UNIX_PATH);
	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
	    return(False);
	}
	if (connect(sd, &saddr, strlen(saddr.sun_path)+sizeof(saddr.sun_family)) < 0) {
	    close(sd);
	    return(False);
	}
    } else {
#endif	/* UNIXCONN */
#ifdef TCPCONN
	/*
	 * Attempts to open INET domain socket.
	 */
	if ((hp = gethostbyname(im_hostname)) == NULL) {
	    return(False);
	}
	bzero((char *)&saddr_in, (int)sizeof(saddr_in));
	bcopy(hp->h_addr, (char *)&saddr_in.sin_addr, hp->h_length);
	saddr_in.sin_family = AF_INET;
	saddr_in.sin_port = htons(port);
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    return(False);
	}
	if (connect(sd, &saddr_in, sizeof(saddr_in)) < 0) {
	    close(sd);
	    return(False);
	}
#endif
#ifdef	UNIXCONN
    }
#endif	/* UNIXCONN */

    /*
     * Send the display name to the input manager. 
     */
    indian = 1;
    if (*(char *) &indian) {
	client.byteOrder = 'l';
    } else {
	client.byteOrder = 'B';
    }
    client.length = strlen(displaybuf);

    im->fd = sd;
    if ((_XipWriteToIM(im, (char *)&client, sizeof(ximConnClient)) < 0) ||
	(_XipWriteToIM(im, (char *)displaybuf, (int)client.length) < 0) ||
	(_XipFlushToIM(im) < 0)) {
	return(False);
    }


    /*
     * Now see, if connection was accepted.
     */
    if (_XipReadFromIM(im, (char *)&reply, sz_ximNormalReply) < 0) {
	return(False);
    }
    if (reply.state != 0) {
	close(sd);
	return(False);
    }
    return(True);
#endif
}

/*
 * Disconnect from the input manager.
 */

void
_XipDisconnectIM(server)
    int server;
{
    (void) close(server);
}

