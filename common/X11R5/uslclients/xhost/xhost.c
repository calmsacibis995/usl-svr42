/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xhost:xhost.c	1.4"
#endif
/*copyright "%c%"*/

/*
 xhost.c (C source file)
	Acc: 573434423 Thu Mar  3 18:20:23 1988
	Mod: 572849598 Thu Feb 25 23:53:18 1988
	Sta: 573774634 Mon Mar  7 16:50:34 1988
	Owner: 2011
	Group: 1985
	Permissions: 444
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/*

Copyright 1985, 1986, 1987 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.
M.I.T. makes no representations about the suitability of
this software for any purpose.  It is provided "as is"
without express or implied warranty.

*/

#define _USHORT_H 
#if !defined(SYSV) && !defined(SVR4)
#include <signal.h>
#include <setjmp.h>
#endif /* SYSV */
#include <ctype.h>
#if !defined(SYSV) && !defined(SVR4)
#include <sys/socket.h>
#endif /* SYSV */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#if !defined(SYSV) && !defined(SVR4)

#include <netdb.h>
#include <netinet/in.h>
#if 0
#include <arpa/inet.h>
	/* bogus definition of inet_makeaddr() in BSD 4.2 and Ultrix */
#else
extern unsigned long inet_makeaddr();
#endif
#ifdef DNETCONN
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif

#endif /* SYSV */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include "Xstreams.h"
 
static void		add_host();
static int		dpy_num();
static void		error();
static char * 		extract_host();
static XHostAddress	*get_address();
static int		local_xerror();
static void		remove_all();
static void		restore();

#define DELETE_ALL	"none"
#define MAGIC_DIR	"lib"
#define MAGIC_FILE	".local_servers"
#define MAXHOSTLEN	1024
#define MAXLINELEN	1024
#define MAXPATHLEN	1024
#define RESTORE		"default"

#if !defined(SYSV) && !defined(SVR4)

#define NAMESERVER_TIMEOUT 5	/* time to wait for nameserver */


typedef struct {
	int af, xf;
} FamilyMap;

static FamilyMap familyMap[] = {
#ifdef	AF_DECnet
    {AF_DECnet, FamilyDECnet},
#endif
#ifdef	AF_CHAOS
    {AF_CHAOS, FamilyChaos},
#endif
#ifdef	AF_INET
    {AF_INET, FamilyInternet}
#endif
};

#define FAMILIES ((sizeof familyMap)/(sizeof familyMap[0]))

int nameserver_timedout;
 
static int XFamily(af)
    int af;
{
    int i;
    for (i = 0; i < FAMILIES; i++)
	if (familyMap[i].af == af)
	    return familyMap[i].xf;
    return -1;
}

#endif /* SYSV */

Display *dpy;
char *progname;
extern char TypeOfStream[];


main(argc, argv)
	int argc;
	char **argv;
{
	extern char *GetXWINHome ();
	char host[256];
	register char *arg;
	int display, i, w, nhosts;
	XHostAddress *address;
	char *hostname, *get_hostname();
	XHostAddress *list;
	Bool enabled = False;
#if !defined(SYSV) && !defined(SVR4)
#ifdef DNETCONN
	char *dnet_htoa();
	struct nodeent *np;
	struct dn_naddr *nlist, dnaddr, *dnaddrp, *dnet_addr();
	char *cp;
#endif
#endif /* SYSV */

	progname = argv[0];

	if ((dpy = XOpenDisplay(NULL)) == NULL) {
	    fprintf(stderr, "%s: Can't open display \"%s\"\n",
		    argv[0], XDisplayName(NULL));
	    exit(1);
	}

#if !defined(SYSV) && !defined(SVR4)
	XSetCloseDownMode(dpy, RetainPermanent);
#endif

	XSetErrorHandler(local_xerror);
 
/* TypeOfStream != local */
	if(TypeOfStream[dpy->fd]  >= X_TLI_STREAM &&
  	   GetNetworkInfo (dpy->fd, NULL, OpenDaemonConnection) < 0)
	{
		fprintf(stderr, "xhost: Cannot open a channel to xdaemon\n"); 
		exit(1);
	} 
	if (argc == 1) {

#if !defined(SYSV) && !defined(SVR4)
#ifdef DNETCONN
		setnodeent(1); /* keep the database accessed */
#endif
		sethostent(1); /* don't close the data base each time */
#endif /* SYSV */

		list = XListHosts(dpy, &nhosts, &enabled);
		printf ("Host Access Control %s.\n", 
			enabled ? "enabled": "disabled");
			
		if (nhosts != 0) {
#if defined(SYSV) || defined(SVR4)
		    PrintHostsNames(list, nhosts);
#else
		    for (i = 0; i < nhosts; i++ )  {
		      hostname = get_hostname(&list[i]);
		      printf("%s\n", hostname);
		      if (nameserver_timedout)
			printf("(nameserver did not respond in %d seconds)\n",
			        NAMESERVER_TIMEOUT);
		      else printf("\n");
		    }
#endif /* SYSV */
		    XFree(list);
#if !defined(SYSV) && !defined(SVR4)
		    endhostent();
#endif /* SYSV */
		}
		exit(0);
	}
 
	for (i = 1; i < argc; i++)
	  {
	    arg = argv[i];
	    if (*arg == '-')
	      {
	        if (!argv[i][1] && ((i+1) == argc))
		  XEnableAccessControl(dpy);
		else
		  {
		    arg = argv[i][1]? &argv[i][1] : argv[++i];
                    if ((address = get_address(arg)) == NULL) 
		      fprintf(stderr, "%s: bad host: %s\n", argv[0], arg);
                    else
		      XRemoveHost(dpy, address);
		  }
	      }
	    else if (*arg == '+')
	      {
		if (!argv[i][1] && ((i+1) == argc))
		  XDisableAccessControl(dpy);
		else
		  {
		    arg = argv[i][1]? &argv[i][1] : argv[++i];
		    add_host(arg);
		  }
	      }
	    else if (strcmp(arg, DELETE_ALL) == 0)
	      remove_all();
	    else if (strcmp(arg, RESTORE) == 0)
	      restore();
	    else {
		fprintf (stderr, "Unknown argument: %s\n", arg, 2);
		fprintf (stderr, "Usage: xhost [+|-]<hostname> 	; enable/disable host(s)\n");
		fprintf (stderr, "       xhost default 		; restore default host list from /etc/X?.hosts\n");
		fprintf (stderr, "       xhost none 		; remove all hosts from access list\n");
		exit;
	    }
	  }

	XCloseDisplay (dpy);  /* does an XSync first */
	exit(0);
}

 
/*
 * error - print error message and exit.
 */
static void
error(format, string, exit_stat)
  char	*format;
  char	*string;
  int	exit_stat;
{

  fprintf(stderr, "%s: ", progname);
  fprintf(stderr, format, string);
  fprintf(stderr, "\n");
  exit(exit_stat);
} /* error() */


/*
static int
is_local_server()
{
  char			*cptr;
  char			*display_host;
  char			*display_string;
  FILE			*magic_fd;
  char			magic_file_name[MAXPATHLEN];
  char			magic_file_line[MAXLINELEN];
  char			magic_file_host[MAXHOSTLEN];
#ifndef MEMUTIL
  extern char		*malloc();
#endif
  extern char		*GetXWINHome ();
  struct utsname	uname_stuff;


  display_string = DisplayString(dpy);

  if ((display_host = malloc(strlen(display_string) + 1)) == NULL)
    error("Out of memory", NULL, 2);

  strcpy(display_host, display_string);

  if ((cptr = strpbrk(display_host, ":")) != NULL)
    *cptr = '\0';

  if (strcmp(display_host, "unix") == 0)
    return 1;
  else
    {
      if (uname(&uname_stuff) == -1)
	error("Failed to get the uname", NULL, 2);
      if (strcmp(display_host, uname_stuff.nodename) == 0)
	return 1;
      else
	{
	  (void)sprintf(magic_file_name, "%s/%s", GetXWINHome(MAGIC_DIR), MAGIC_FILE);
	  if ((magic_fd = fopen(magic_file_name, "r")) == NULL)
	    return 0;
	  else
	    {
	      while (fgets(magic_file_line, MAXLINELEN, magic_fd) != NULL)
		{
		  extract_host(magic_file_line, magic_file_host);
		  if (strcmp(display_host, magic_file_host ) == 0)
		    return 1;
		}
	      return 0;
	    }
	}
    }
} 

*/


/*
 * remove_all - remove all of the hosts in the server's list.
 */
static void
remove_all()
{
  Bool		enabled;
  XHostAddress	*hosts;
  int		nhosts;


  hosts = XListHosts(dpy, &nhosts, &enabled);
  if (!enabled)
    XEnableAccessControl(dpy);
  if (nhosts != 0)
    {
      XRemoveHosts(dpy, hosts, nhosts);
      XFree(hosts);
      fprintf (stderr, "All hosts removed from access list\n");
    }
} /* remove_all() */


/*
 * restore - Add the hosts in /etc/X?.hosts to the server's list.
 */
static void
restore()
{
  char			host[MAXHOSTLEN];
  FILE			*hosts_fd;
  char			hosts_file[MAXPATHLEN];
  char			*host_line[MAXLINELEN];
#if 0
  struct utsname	uname_stuff;
#endif

  (void)sprintf(hosts_file, "/etc/X%d.hosts", dpy_num(DisplayString(dpy)));
  if ((hosts_fd = fopen(hosts_file, "r")) == (FILE *)NULL)
    error("Can't open %s", hosts_file, 2);

  remove_all();

#if 0
  if (uname(&uname_stuff) == -1)
    error("Failed to get the uname", NULL, 2);
  add_host(uname_stuff.nodename);
#endif
  
  while (fgets((char *)host_line, MAXLINELEN, hosts_fd) != NULL)
    {
      extract_host(host_line, host);
      if (*host != '\0')
	add_host(host);
    }
} /* restore() */


static void
add_host(host)
  char	*host;
{
  XHostAddress		*address;


  if ((address = get_address(host)) == NULL) 
    fprintf(stderr, "%s: Bad host: %s\n", progname, host);
  else
    {
      XAddHost(dpy, address);
#ifdef DEBUG
      {
	int i;

	fprintf(stderr, "Added %s: family=%d, length=%d, address=|",
		host, address->family, address->length);
	for (i=0; i < address->length; ++i)
	  if (isprint(*address->address))
	    putc(*(address->address++), stderr);
	  else
	    fprintf(stderr, "?%d?", *address->address);
	fprintf(stderr, "|\n");
      }
#endif
    }
} /* add_host() */


static char *
extract_host(host_line, host)
  char	*host_line;
  char	*host;
{
  char	*cptr;


  if ((cptr = strpbrk(host_line, "#\n")) != NULL)
    *cptr = '\0';

  cptr = host_line;

  while (*cptr != '\0' && isspace(*cptr))
    ++cptr;

  while (*cptr != '\0' && !isspace(*cptr))
    *(host++) = *(cptr++);

  *host = '\0';
  return (cptr);
} /* extract_host() */


static int
dpy_num(dpy_string)
  char	*dpy_string;
{
  char	*cptr;

  
  if((cptr = strrchr(dpy_string, (int)':')) == NULL)
    return 0;
  else
    {
      ++cptr;
      return atoi(cptr);
    }
} /* dpy_num() */


/*
 * get_address - return a pointer to an internet address given
 * either a name (CHARON.MIT.EDU) or a string with the raw
 * address (18.58.0.13)
 */

static XHostAddress
*get_address (name) 
char *name;
{
  static XHostAddress ha;
  static char buf[128];
  char	 *ptr, *packet, *retptr, pktbuf[128];
  int	 n;

/* TypeOfStream == LOCAL */

  if((TypeOfStream[dpy->fd]  < X_TLI_STREAM))
  {
	ha.family = FamilyUname;
	ha.length = strlen(name) +1;
	ha.address = name;
	if (check_hostname(name))
		return(&ha);
	else
		return NULL;
  }

  packet = pktbuf;
  ptr = &packet[2*sizeof(int)];

  n = strlen(name) + 1;
  ((xHostEntry *) ptr)->length = n;
  ptr += sizeof(xHostEntry);
  memcpy(ptr, name, n);

  retptr = packet;
   *(int *) retptr = n+sizeof(xHostEntry);
   *(int *) (retptr + sizeof(int)) = 1;

  if(GetNetworkInfo (dpy->fd, NULL, ConvertNameToNetAddr, &packet, &retptr, NULL)<0)
           {
		return(&ha);
           }
   ha.family = ((xHostEntry *) retptr)->family;
   ha.length = ((xHostEntry *) retptr)->length;
   ha.address = buf;
  
   if(ha.length > 127)
   	ha.length = 127;

   ptr = &retptr[sizeof(xHostEntry)];
   ptr[ha.length] = '\0';
   memcpy(buf, ptr, ha.length +1);
   return(&ha);   
}


PrintHostsNames(list, nhosts)
XHostAddress *list;
int	nhosts;
{
    int  m, n, i;
    char *ptr, *retptr;
    static char *packet = NULL;
    static int buflen = 0;
    int	   *iptr;
#ifndef MEMUTIL
  extern char		*malloc();
#endif /* MEMUTIL */
 
    if(buflen == 0)
		buflen = 512;

    m = 2 * sizeof(int);
    packet = (char  *) malloc((int)buflen);
    if(packet == NULL){
	fprintf(stderr, "Cannot malloc %d chars \n", buflen);
	return;
	}
    ptr = &packet[m];

    for (i=0; i< nhosts; i++)
    {
	n = (((list[i].length + 3) >> 2) << 2) + sizeof(xHostEntry);
	m += n;
	if(m > buflen){
		buflen = m + 128;
		packet = (char *) realloc(packet, buflen);
    		if(packet == NULL){
			fprintf(stderr, "Cannot realloc %d chars \n", buflen);
			return;
			}
		}
	ptr = &packet[m - n];
	((xHostEntry *) ptr)->length  = list[i].length;
	((xHostEntry *) ptr)->family  = list[i].family;
	ptr += sizeof(xHostEntry);
	bcopy (list[i].address, ptr, list[i].length);
    }
    iptr = (int *)packet;
    iptr[0] = m;
    iptr[1]   = nhosts;

#ifdef DEBUG
fprintf(stderr, "%d length, %d nhosts\n", iptr[0], iptr[1]);
#endif
	
/* TypeOfStream != LOCAL */

    if((TypeOfStream[dpy->fd] >= X_TLI_STREAM))	{

    	n =
 GetNetworkInfo (dpy->fd, NULL,ConvertNetAddrToName, &packet, &retptr, &nhosts);
	if( n <= 0){
		fprintf(stderr, "No reply from the nameserver\n");
		return;
		}
	}
  	else retptr = &packet[2*sizeof(int)];
     m = 0;
     for(i=0; i<nhosts; i++){
	ptr = &retptr[m];
     	n = ((xHostEntry *) ptr)->length;
	n = (((n + 3) >> 2) << 2) + sizeof(xHostEntry);
	m += n;
     	ptr += sizeof(xHostEntry);
 	fprintf(stderr, "%s\n", ptr);	
 	}		
     free(retptr);
}

/*
 * get_hostname - Given an internet address, return a name (CHARON.MIT.EDU)
 * or a string representing the address (18.58.0.13) if the name cannot
 * be found.
 */

#if !defined(SYSV) && !defined(SVR4)
jmp_buf env;
#endif /* SYSV */


char *get_hostname (ha)
XHostAddress *ha;
{
  static char buf[128];
  char	 *ptr, *packet, pktbuf[128], *retptr;
  int	 n;

/* TypeOfStream==local */

   if(TypeOfStream[dpy->fd] < X_TLI_STREAM || ha->family == FamilyUname)
	return(ha->address);

  packet = pktbuf;
  ptr = &packet[2*sizeof(int)];

  ((xHostEntry *) ptr)->length = ha->length;
  ((xHostEntry *) ptr)->family = ha->family;

  ptr += sizeof(xHostEntry);
  memcpy(ptr, ha->address, ha->length);

   retptr = packet;
   *(int *) retptr = ha->length+sizeof(xHostEntry);
   *(int *) (retptr + sizeof(int)) = 1;

  if(GetNetworkInfo (dpy->fd, NULL, ConvertNetAddrToName, &packet, &retptr, NULL)<0)
           {
		ha->address[ha->length] = '\0';
		return(ha->address);
           }
   ptr = &retptr[sizeof(xHostEntry)];
   ptr[((xHostEntry *) retptr)->length] = '\0';

   memcpy(buf, ptr, ((xHostEntry *) retptr)->length +1);
   return(buf);
}


#if !defined(SYSV) && !defined(SVR4)
nameserver_lost()
{
  nameserver_timedout = 1;
  longjmp(env, -1);
}
#endif /* SYSV */

/*
 * local_xerror - local non-fatal error handling routine. If the error was
 * that an X_GetHosts request for an unknown address format was received, just
 * return, otherwise call the default error handler _XDefaultError.
 */
static int
local_xerror (dpy, rep)
    Display *dpy;
    XErrorEvent *rep;
{
    if ((rep->error_code == BadValue) && (rep->request_code == X_ListHosts)) {
	return;
    } else {
	_XDefaultError(dpy, rep);
    }
}

/*
 * check_hostname - checks to see if the host name is valid
 */
check_hostname(name)
char *name;
{
  char 	host[MAXHOSTLEN];
  FILE	*hosts_fd;
  char	*host_line[MAXLINELEN];
  char  *np;

  if ((hosts_fd = fopen("/etc/hosts", "r")) == (FILE *)NULL) {
 	fprintf(stderr, "Cannot open /etc/hosts\n");
	return False;
  }
  
  while (fgets((char *)host_line, MAXLINELEN, hosts_fd) != NULL)
  {
	/* the call to extract_host returns the first token in the
	 * 2nd arg, ie: host and the ptr (return code) to the next
	 * token.
	 * The 2nd call gets the 2nd token, which should be the
	 * host name
	 * Then check if the 2nd token is same as "name"
	 */
	np = extract_host(host_line, host);
	np = extract_host(np,host);
	if (strcmp(name,host) == 0) {
		return True;
	}
  }

  return False;
}
