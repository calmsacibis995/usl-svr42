/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/resolv/libresolv/res_init.c	1.1.8.4"
#ident  "$Header: res_init.c 1.3 91/09/20 $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <errno.h>
#include "res.h"

#pragma weak res_init=_rs_res_init

/*
 * Resolver configuration file. Contains the address of the
 * inital name server to query and the default domain for
 * non fully qualified domain names.
 */

#ifndef	CONFFILE
#define	CONFFILE	"/etc/resolv.conf"
#endif

/*
 * Resolver state default settings
 */

struct state _res = {
    RES_TIMEOUT,               	/* retransmition time interval */
    4,                         	/* number of times to retransmit */
    RES_DEFAULT,		/* options flags */
    1,                         	/* number of name servers */
};

/*
 * Set up default settings.  If the configuration file exist, the values
 * there will have precedence.  Otherwise, the server address is set to
 * INADDR_ANY and the default domain name comes from the gethostname().
 *
 * The configuration file should only be used if you want to redefine your
 * domain or run without a server on your machine.
 *
 * Return 0 if completes successfully, -1 on error
 */
_rs_res_init()
{
    register FILE *fp;
    register char *cp, **pp;
    char buf[BUFSIZ];
    extern u_long _rs_inet_addr();
    extern char *index();
    extern char *strcpy(), *strncpy();
    extern char *getenv();
    int n = 0;    /* number of nameserver records read from file */

    _res.nsaddr.sin_addr.s_addr =  _rs_htonl(INADDR_LOOPBACK); /*** INADDR_ANY; ***/
    _res.nsaddr.sin_family = AF_INET;
    _res.nsaddr.sin_port = _rs_htons(NAMESERVER_PORT);
    _res.nscount = 1;
      /*
       * for the benefit of hidden YP domains, we use the same procedure
       * as sendmail: convert leading + to dot, then drop to first dot
       */
    getdomainname( buf, BUFSIZ);
    if (buf[0] == '+')
	buf[0] = '.';
    cp = index(buf, '.');
    if (cp == NULL)
    	strcpy(_res.defdname, buf);
    else 
    	strcpy(_res.defdname, cp+1);

    if ((fp = fopen(CONFFILE, "r")) != NULL) {
        /* read the config file */
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            /* read default domain name */
            if (!strncmp(buf, "domain", sizeof("domain") - 1)) {
                cp = buf + sizeof("domain") - 1;
                while (*cp == ' ' || *cp == '\t')
                    cp++;
                if (*cp == '\0')
                    continue;
                (void)strncpy(_res.defdname, cp, sizeof(_res.defdname));
                _res.defdname[sizeof(_res.defdname) - 1] = '\0';
                if ((cp = index(_res.defdname, '\n')) != NULL)
                    *cp = '\0';
                continue;
            }
            /* read nameservers to query */
            if (!strncmp(buf, "nameserver", 
               sizeof("nameserver") - 1) && (n < MAXNS)) {
                cp = buf + sizeof("nameserver") - 1;
                while (*cp == ' ' || *cp == '\t')
                    cp++;
                if (*cp == '\0')
                    continue;
                _res.nsaddr_list[n].sin_addr.s_addr = _rs_inet_addr(cp);
                if (_res.nsaddr_list[n].sin_addr.s_addr == (unsigned)-1) 
                    _res.nsaddr_list[n].sin_addr.s_addr = INADDR_ANY;
                _res.nsaddr_list[n].sin_family = AF_INET;
                _res.nsaddr_list[n].sin_port = _rs_htons(NAMESERVER_PORT);
                if ( ++n >= MAXNS) { 
                    n = MAXNS;
#ifdef DEBUG
                    if ( _res.options & RES_DEBUG )
                        printf("MAXNS reached, reading resolv.conf\n");
#endif DEBUG
                }
                continue;
            }
        }
        if ( n > 1 ) 
            _res.nscount = n;
        (void) fclose(fp);
    }
    if (n == 0) {
	    /*
	     * we didn't find a nameserver record, so look if
	     * nameserver is running on local host.
	     */
	     int s;
	     extern int errno;

	     s = _rs_socket(AF_INET, SOCK_DGRAM, 0);
	     if (s < 0)
		    return(-1);
	     if (_rs_bind(s, &_res.nsaddr, sizeof(_res.nsaddr)) >= 0 ||
		 errno != EADDRINUSE) {
		    (void) close(s);
		    return(-1);
	     }
	     (void) close(s);
    }
    if (_res.defdname[0] == 0) {
        if (gethostname(buf, sizeof(_res.defdname)) == 0 &&
           (cp = index(buf, '.')))
             (void)strcpy(_res.defdname, cp + 1);
    }

    /* Allow user to override the local domain definition */
    if ((cp = getenv("LOCALDOMAIN")) != NULL)
        (void)strncpy(_res.defdname, cp, sizeof(_res.defdname));

    /* find components of local domain that might be searched */
    pp = _res.dnsrch;
    *pp++ = _res.defdname;
    for (cp = _res.defdname, n = 0; *cp; cp++)
	if (*cp == '.')
	    n++;
    cp = _res.defdname;
    for (; n >= LOCALDOMAINPARTS && pp < _res.dnsrch + MAXDNSRCH; n--) {
	cp = index(cp, '.');
	*pp++ = ++cp;
    }
    _res.options |= RES_INIT;
    return(0);
}
