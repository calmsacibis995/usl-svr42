/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/getscheme.c	1.15.2.3"
#ident  "$Header: getscheme.c 1.2 91/06/26 $"

#include <tiuser.h>
#include <fcntl.h>
#include <stdio.h>
#include <netconfig.h>
#include <netdir.h>
#include <stropts.h>
#include <dial.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/procset.h>
#include <errno.h>
#include "global.h"
#include "extern.h"
 
static	time_t 	cache_date;
static	struct 	schemelist *schemelistp=NULL;
static	char    *scheme_file=NULL;
static	int 	scheme_error;

static	void	put_in_list(struct schemelist *);
static	void 	store_scheme(char *, char *, char *, char *, char *);
static	struct 	schemelist *check_scheme_list(char *, char *, char *);
static	struct 	schemeinfo *
      	get_from_server(char *, char *, struct netconfig *);

static
void
print_list()
{
	struct schemelist *lp;

	DUMP((msg,"gs: Dumping internal cache:"));
	for (lp=schemelistp; lp != NULL; lp = lp->i_next) {
		DUMP((msg,"gs: <%s> <%s> <%s> <%s> <%s>", lp->i_host,
		      lp->i_service, lp->i_netid, lp->i_scheme, lp->i_role));
	}
	if (schemelistp == NULL) {
		DUMP((msg,"gs: internal cache is NULL"));
	}
}
/*
 *	getscheme() obtains the authentication information needed
 *	for the specified service on the given transport.
 */

int
getscheme(host, service, net, local, scheme)
char *host;
char *service;
struct netconfig *net;
int local;
struct schemeinfo **scheme;
{
	struct	schemelist *listp;

	scheme_error = CS_NO_ERROR;

	if (local) {
		print_list();
		*scheme = (struct schemeinfo *)NULL;
		listp = check_scheme_list(host, service, net->nc_netid);
		if (listp != NULL) {
		    if ((*scheme = (struct schemeinfo *)
		         malloc(sizeof(struct schemeinfo))) == NULL){
                               return(CS_MALLOC);
		    }
		    (*scheme)->s_name = strcmp(listp->i_scheme, "-")==0? 
		         NULL : strdup(listp->i_scheme);
		    (*scheme)->s_flag = strcmp(listp->i_role, "i") == 0?
		         AU_IMPOSER : AU_RESPONDER;
		    DUMP((msg,"gs: internal cache returning <%s>",
	       	         (*scheme)->s_name));
		} else {
		    DUMP((msg,"gs: scheme not in internal cache"));
		}
	} else {
		*scheme = get_from_server(host, service, net);
	}

	return(scheme_error);
}

/*
 *	check_scheme_list loops through the schemelistp checking
 *	for host and service.
 */
static
struct schemelist *
check_scheme_list(i_host, i_service, i_netid)
	char 	*i_host;
	char	*i_service;
	char	*i_netid;
{
	struct 	schemelist *lp;

	for (lp=schemelistp; lp != NULL; lp = lp->i_next) {
		if ((strcmp(lp->i_host,i_host) == 0 ) && 
		    (strcmp(lp->i_service,i_service) == 0) && 
		    (strcmp(lp->i_netid,i_netid) == 0)) {
			DUMP((msg,"gs: auth scheme in scheme list"));
			return(lp);
		}
	}
	return(NULL);
}


/*
 *	Get authentication scheme remotely from server machine.	
 */

static
struct schemeinfo *
get_from_server(host, service, netconfigp)
char *host;
char *service;
struct netconfig *netconfigp;
{
	int fd;				 /* file desc. to transport   */
	int i;				 /* used to loop through addrs*/
	int success;			 /* 1 if an address connects  */
	char buf[BUFSIZ];		 /* holds info from server    */
	struct nd_hostserv nd_hostserv;  /* info for name2addr rtns   */
	struct nd_addrlist *nd_addrlistp;/* info from name2addr rtns  */
	struct t_call *sndcall;		 /* used to connect to server */
	struct netbuf *netbufp;		 /* holds addrs while looping */
	struct schemeinfo *retp;	 /* return value of this rtn  */
	char *retval;			 /* return val from server    */
	char *role;			 /* role from server    */
	char *scheme;			 /* return scheme from server */
	char *s_tag;			/* pmtab's service tag */
	struct con_request	reportrequest;
	struct con_request	*reportrequestp=&reportrequest;
	char	*request_bufp;


	DUMP((msg,"gs: asking reportscheme service for scheme"));

	(void)memset((void *)buf, '\0', BUFSIZ);

	if ((fd = t_open(netconfigp->nc_device, O_RDWR, NULL)) == -1) {
		CS_LOG((msg,"t_open fail reportscheme not called"));
		return(NULL);
	}

	if(t_bind(fd, NULL, NULL) == -1) {
		CS_LOG((msg,"t_bind fail reportscheme not called"));
		t_close(fd);
		return(NULL);
	}

	nd_hostserv.h_host = host;
	nd_hostserv.h_serv = "reportscheme";

	if (netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) != 0) {
		CS_LOG((msg,"getbyname fail reportscheme not called"));
		CS_LOG((msg,"ensure that _pmtab & services list:"));
		CS_LOG((msg,"    \"reportscheme\" NOT \"rptsch\""));
		t_close(fd);
		return(NULL);
	}

	/* Connect to the reportscheme service.  */

	if((sndcall = (struct t_call *)t_alloc(fd,T_CALL,T_ALL)) == NULL) {
		CS_LOG((msg,"gs: sndcall fail reportscheme not called"));
		t_close(fd);
		return(NULL);
	}

	success = 0;
	netbufp = nd_addrlistp->n_addrs;
	for (i = 0; i < nd_addrlistp->n_cnt; i++) {
		(void) memcpy(sndcall->addr.buf, netbufp->buf, netbufp->len);
		sndcall->addr.len = netbufp->len;
		sndcall->opt.len = 0;	/* default options are used */
		sndcall->udata.len = 0;
	
		if (t_connect(fd, sndcall, (struct t_call *)NULL) == 0) {
			DUMP((msg,"gs: connected to reportscheme"));
			success = 1;
			break;
		}
		netbufp ++;
	}

	if (!success) {
		CS_LOG((msg,"could not connect to reportscheme"));
		CS_LOG((msg,"NULL authentication scheme ASSUMED"));
		t_free(sndcall, T_CALL);
		t_close(fd);
		return(NULL);
	}

	/* Push on tirdwr since reportscheme simply writes the data.*/

	if (ioctl(fd, I_PUSH, "tirdwr") < 0) {
		CS_LOG((msg,"ioctl() I_PUSH failed errno=%d", errno));
		CS_LOG((msg,"NULL authentication scheme ASSUMED"));
		t_free(sndcall, T_CALL);
		t_close(fd);
		return(NULL);
	}

	/* Send the service name and transport; receive the scheme 
	 * name and role.
	 * NOTE: if the service is listen its service-tag is "0" not
	 *       "listen" so change service to "0"
	 */

	s_tag = (strcmp(service, "listen") == 0)? strdup("0"): service;
	if (write(fd, s_tag, strlen(s_tag) + 1) < 0) {
		CS_LOG((msg,"write failed, errno=%d", errno));
		CS_LOG((msg,"NULL authentication scheme ASSUMED"));
		t_free(sndcall, T_CALL);
		t_close(fd);
		return(NULL);
	}
	
	if (read(fd, buf, BUFSIZ) <= 0) {
		CS_LOG((msg,"read failed, errno=%d", errno));
		CS_LOG((msg,"NULL authentication scheme ASSUMED"));
		t_free(sndcall, T_CALL);
		t_close(fd);
		return(NULL);
	}
	
	DUMP((msg,"gs: read from reportscheme <%s>",buf));

	retval = strtok(buf, ":");
	scheme = strtok(NULL, ":");
	if (strcmp(scheme, "0") == 0)
		scheme = "";
	role = strtok(NULL, ":");

	/*  	A return value of 0 means something is messed up
	 *	on the server, so return NULL here.
	 *	A return value of -1 means service not found.
	 */

	if (strcmp(retval, "0") == 0) {
		t_free(sndcall, T_CALL);
		t_close(fd);
		return(NULL);
	}

	if (strcmp(retval, "-1") == 0) {
		t_free(sndcall, T_CALL);
		t_close(fd);
		scheme_error = CS_NOTFOUND;
		return(NULL);
	}

	/* set up the return value...  */

	if ((retp = (struct schemeinfo *)
	     malloc(sizeof(struct schemeinfo))) == NULL) {
		t_free(sndcall, T_CALL);
		t_close(fd);
		CS_LOG((msg,"malloc failed, errno=%d", errno));
		CS_LOG((msg,"NULL authentication scheme ASSUMED"));
		return(NULL);
	}
	retp->s_name = *scheme == '\0'? NULL: strdup(scheme);
	retp->s_flag = strcmp(role, "i") == 0? AU_IMPOSER : AU_RESPONDER;
	
	/* write the data to the cache file */
	store_scheme(host, service, (*scheme == '\0'? "-": scheme),
		     role, netconfigp->nc_netid);

	/* close and free everything, and return the information 
	 * to the application.
	 */

	t_free(sndcall, T_CALL);
	t_close(fd);
	return(retp);
}

/*
 * Store the scheme for given host and service	
 */

static
void
store_scheme(i_host,i_service,i_scheme,i_role,i_netid)
	char 	*i_host;
	char	*i_service;
	char	*i_scheme;
	char	*i_role;
	char	*i_netid;
{
	char 	buf[BUFSIZ];	/* holds info from server    */
	int	cache;
	int	size;

	if ((cache = open(CACHEFILE, O_APPEND|O_WRONLY|O_CREAT)) <= 0) {
		CS_LOG((msg,"unable to open %s, errno=%d", 
			CACHEFILE, errno));
		return;
	}

	/* It is unnecessary to lock the file before writing since
	 * writes of this size (<BLOCKSIZE) are autonomous.  Even
	 * if problems do arise they are unlikely and have only 
	 * slight impact on performance (a garballed or ommited 
	 * write here will just be ignored later).
	 */

	sprintf(buf, "%s\t%s\t%s\t%s\t%s\n", i_host, i_service,
		i_netid, i_scheme, i_role);
	size = strlen(buf);
	if (write(cache, buf, size) != size) {
		CS_LOG((msg,"write scheme to %s failed", CACHEFILE));
	} else {
		DUMP((msg,"gs: stored <%s> <%s> <%s>...", i_host, 
		      i_service, i_scheme));
	}

	if (close(cache) != 0) {
		CS_LOG((msg,"unable to close %s, errno=%d", 
			CACHEFILE, errno));
		return;
	}

	/* notify daemon to read cache file */
	sigsend(P_PID, getppid(), SIGUSR1);

}


/*
 *	checkscheme() reads the SERVEALLOW file to see if the
 *	given scheme, service, and host triple is acceptable
 *	to the client machine.  This allows the client machine
 *	to protect itself from malicious servers.
 */

int
checkscheme(scheme, host, service)
char *scheme;
char *host;
char *service;
{
	FILE *fp;	  /* hold open of the SERVEALLOW file */
	char *i_host;	  /* host field of file               */
	char *i_service;  /* service field of file            */
	char *i_scheme;	  /* each valid scheme name           */
	char buf[BUFSIZ]; /* buffer to read line of file into */

	
	if (scheme == NULL) {
		scheme = "-";
	}

	/* 	Return success if no SERVEALLOW file exists (assume
	 *	any scheme is acceptable)
	 */

	if ((fp = fopen(SERVEALLOW, "r")) == NULL) {
		DUMP((msg,"gs: %s file does not exist", SERVEALLOW));
		return(1);
	}

	/* Read the file to obtain host and service */

	while((fgets(buf, BUFSIZ, fp)) != NULL) {
	    if (!(blank(buf) || comment(buf))) { 
		if ((i_host = strtok(buf, " \t")) == NULL
		 || (i_service = strtok(NULL, " \t")) == NULL) {
			/*
			 *	Error in format of the file, return
			 *	stating the scheme in not acceptable...
			 */
			DUMP((msg,"gs: %s in wrong format", SERVEALLOW));
			fclose(fp);
			return(0);
		}
		if (strcmp(i_host, host) == 0
		 && strcmp(i_service, service) == 0) {
			/*
			 *	Got it.  Check to make sure the scheme
			 *	is listed in the file.
			 */
			while ((i_scheme = strtok(NULL, ", \t\n")) != NULL) {
				if (strcmp(i_scheme, scheme) == 0) {
					DUMP((msg,"gs: scheme FOUND in %s file", SERVEALLOW));
					fclose(fp);
					return(1);
				}
			}
			CS_LOG((msg,"%s does not permit %s scheme with %s:%s",
			    SERVEALLOW, scheme, host, service));
			fclose(fp);
			return(0);
		}
	    } 
	}

	/*
	 *	If entry is not found, return stating that the scheme
	 *	id OK (since any scheme is OK)...
	 */

	DUMP((msg,"gs: scheme not found in %s file", SERVEALLOW));
	fclose(fp);
	return(1);
}

/*
 *	blank() returns true if the line is a blank line, 0 otherwise
 */

int
blank(cp)
char *cp;
{
	while (*cp && isspace(*cp)) {
		cp++;
	}
	return(*cp == '\0');
}

/*
 *	comment() returns true if the line is a comment, 0 otherwise.
 */

int
comment(cp)
char *cp;
{
	while (*cp && isspace(*cp)) {
		cp ++;
	}
	return(*cp == '#');
}

/*
 *	Get_alias() reads /etc/iaf/serv.alias searching for
 *	an alias service name for a given host service pair.
 *	If an alias service name is found then the service
 *	name in the host service pair is replaced by the
 *	alias service name.
 */

int
get_alias()
{
	FILE *fp;	  /* hold open of the SERVE.ALIAS     */
	char *i_host;	  /* host field of file               */
	char *i_service;  /* service field of file            */
	char *i_servalias;/* alias field of file	      */
	char buf[BUFSIZ]; /* buffer to read line of file into */

	/* Return success if no SERVE.ALIAS file exists */
	if ((fp = fopen(SERVEALIAS, "r")) == NULL) {
		DUMP((msg,"gs: get_alias: no %s file",SERVEALIAS));
		return(1);
	}

	while((fgets(buf, BUFSIZ, fp)) != NULL) {
	    if (!(blank(buf) || comment(buf))) {
		if ((i_host = strtok(buf, " \t")) == NULL
		 || (i_service = strtok(NULL, " \t")) == NULL 
		 || (i_servalias = strtok(NULL, " \t\n")) == NULL) {
			/*
			 *	Error in format of the file, return
			 *	stating the alias is not acceptable...
			 */
			CS_LOG((msg,"gs: %s file in wrong format",SERVEALIAS));
			fclose(fp);
			return(0);
		}
		if (strcmp(i_host, Nd_hostserv.h_host) == 0
		 && strcmp(i_service, Nd_hostserv.h_serv) == 0) {
			DUMP((msg,"gs: alias found in %s",SERVEALIAS));
			DUMP((msg,"    host<%s> service<%s> alias<%s>", 
			      Nd_hostserv.h_host, Nd_hostserv.h_serv, 
			      i_servalias));
			Nd_hostserv.h_serv = strdup(i_servalias);
			fclose(fp);
			return(1);
		}
	    }
	}

	/* 	If entry is not found, return stating that the scheme
	 *	id OK (since any scheme is OK)...
	 */

	DUMP((msg,"gs: alias NOT found in %s",SERVEALIAS));
	fclose(fp);
	return(1);
}

/*
 * Check if the internal cache alread has a copy of version
 * of the info we want to store.  If it does, update the cache
 * if either the role or scheme has changed.  Don't check for
 * failure from strdup() since bad information in the cache is
 * not fatal.  If the internal cache doesn't have the information, 
 * add it to the head of the list.
 */
static
void
put_in_list(add)
struct schemelist *add;
{
	struct 	schemelist *listp;

	if ((listp = check_scheme_list(add->i_host, add->i_service, 
	     add->i_netid)) != NULL) {
		/* it's already in cache, so update if different */
		if (listp->i_scheme != add->i_scheme) {
			listp->i_scheme = strdup(add->i_scheme);
		}
		if (listp->i_role != add->i_role) {
			listp->i_role = strdup(add->i_role);
		}
	} else {
		/* it's new, add to the head of internal cache */
		add->i_next = schemelistp;
		schemelistp = add;
		DUMP((msg,"gs: added scheme to internal cache"));
	} 
}

/*
 * Read the cache file "cache_name" and add the information to
 * the internal cache.  update_cache_list is passed either
 * AUTHFILE or CACHEFILE.
 * When the daemon is initialized this routine is called to read
 * in the system administrator's authentication file, AUTHFILE.
 * This file may not exist in which case the routine just returns.
 * When the daemon is signaled it calls this routine to read CACHEFILE
 * and update its internal cache.
 */
void
update_cache_list(cache_name)
char	*cache_name;
{
	FILE	*fp;		/* file pointer to cache file */
	char	buf[BUFSIZ];	
	struct	schemelist *new;
	struct	schemelist temp;
	int	i;

        DUMP((msg,"gs: reading schemes from %s", cache_name));

	if ((fp = fopen(cache_name, "r")) == NULL) {
		/* Don't log this since it's not really an error */
		DUMP((msg,"unable to open %s, errno=%d", 
			cache_name, errno));
		return;
	}

	/* Read the file to obtain scheme info.  */
	while((fgets(buf, BUFSIZ, fp)) != NULL) {
	    if (!(blank(buf) || comment(buf))) { 
		if ((new = (struct schemelist *)
		     malloc(sizeof(struct schemelist))) == NULL) {
			CS_LOG((msg,"Malloc error, couldn't cache \
				scheme from %s", cache_name));
                	return;
		}
		/* retrieve the info */
		if ((temp.i_host = strtok(buf, " \t")) == NULL
		 || (temp.i_service = strtok(NULL, " \t")) == NULL
		 || (temp.i_netid = strtok(NULL, " \t")) == NULL
		 || (temp.i_scheme = strtok(NULL, " \t")) == NULL
		 || (temp.i_role = strtok(NULL, " \t\n")) == NULL
		 || strtok(NULL, " \t\n") != NULL) {
			/* Just ignore errors in the format */
			CS_LOG((msg, "error in format of %s", cache_name));
			free(new);
			continue;
		}

		/* fill in the new struct */
		if ((new->i_host = strdup(temp.i_host)) == NULL
		 || (new->i_service = strdup(temp.i_service)) == NULL
		 || (new->i_netid = strdup(temp.i_netid)) == NULL
		 || (new->i_scheme = strdup(temp.i_scheme)) == NULL
		 || (new->i_role = strdup(temp.i_role)) == NULL) {
			CS_LOG((msg,"Malloc error, couldn't cache \
				scheme from %s", cache_name));
                	return;
		}

		DUMP((msg,"gs: cache entry <%s> <%s> <%s> <%s> <%s>", 
		      new->i_host, new->i_service, new->i_netid, 
		      new->i_scheme, new->i_role));
	
		put_in_list(new);
	    }
	}

	if ((fclose(fp)) != NULL) {
		CS_LOG((msg,"unsuccessful close of %s, errno=%d",
			cache_name, errno));
		return;
	}

	/* if this is the temp cache file, clear it out */
	if (strcmp(cache_name, CACHEFILE) == 0) {
		DUMP((msg,"gs: removing (emptying) %s", cache_name));
		unlink(cache_name);
	}
}
