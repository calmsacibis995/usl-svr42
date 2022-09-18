/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libns:common/lib/libns/negotiate.c	1.9.12.3"
#ident  "$Header: negotiate.c 1.2 91/06/26 $"
#include "grp.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include <tiuser.h>
#include <nsaddr.h>
#include <nserve.h>
#include "sys/rf_sys.h"
#include "sys/rf_cirmgr.h"
#include "sys/rf_messg.h"
#include "pn.h"
#include "sys/hetero.h"
#include "nslog.h"
#include <mac.h>

#define NDATA_CANON	"lc20ic64c64ii"
#define NDATA_CLEN	176			/* length of canonical ndata */
#define LONG_CLEN	4			/* length of canonical long */

/* return error codes for negotiate() */
#define	N_ERROR		-1		/* error is in errno */
#define	N_TERROR	-2		/* error is in t_errno */
#define N_VERROR	-3		/* version mismatch */
#define	N_CERROR	-4		/* fcanon/tcanon failed */
#define	N_FERROR	-5		/* bad flag as argument */
#define	N_PERROR	-6		/* passwords did not match */


/* negotiate data packect */

ndata_t ndata;

struct n_data	{
	ndata_t nd_ndata;
	int	n_vhigh;
	int	n_vlow;
};

/* local products, see below */
int checkpw();
int sndrcv_int();

/* imports */
extern int	rfsys();	/* a system call */
extern char	*malloc();	/* from standard place */
extern int	t_snd();	/* from standard place */
extern int	netname();	/* from this library */
extern int	getoken();	/* from this library */
extern int	tcanon();	/* from this library */
extern int	fcanon();	/* from this library */
extern int	rf_rcv();	/* from this library */

int
negotiate(fd, passwd, flag, ngroups_maxp, aclmaxp, macp)
	int	fd;
	char	*passwd;
	long	flag;
	int	*ngroups_maxp;
	int	*aclmaxp;
	int	*macp;
{
	struct rf_token rf_token;
	struct n_data n_buf;
	char netnam[MAXDNAME];
	char nbuf[200];
	char *domnam, *unam;
	int nbytes, rfversion;
	int flgs = 0;
	size_t dlen, ulen;
	int error = 0;

	LOG2(L_TRACE, "(%5d) enter: negotiate\n", Logstamp);
	if (netname(netnam) < 0) {
		return(N_ERROR);
	}
	if (getoken(&rf_token) < 0) {
		return(N_ERROR);
	}

	/* fill in ndata structure */
	(void) strncpy(&ndata.n_passwd[0], passwd ? passwd : "", PASSWDLEN);
	(void) strncpy(&ndata.n_netname[0], netnam, MAXDNAME);
	ndata.n_hetero = (long)MACHTYPE;
	ndata.n_token = rf_token;
	n_buf.nd_ndata = ndata;

	if (rfsys(RF_VERSION,VER_GET,&n_buf.n_vhigh,&n_buf.n_vlow) < 0) {
		return(N_ERROR);
	}

	/* exchange ndata structures */

	if ((nbytes = tcanon(NDATA_CANON, (char *)&n_buf, &nbuf[0], 0)) == 0) {
		return(N_CERROR);
	}
	if (t_snd(fd, nbuf, nbytes, 0) != nbytes) {
		return(N_TERROR);
	}

	if (rf_rcv(fd, &nbuf[0], NDATA_CLEN, &flgs) != NDATA_CLEN) {
		LOG2(L_TRACE, "(%5d) leave: negotiate-rf_rcv\n", Logstamp);
		return(N_TERROR);
	}

	if (fcanon(NDATA_CANON, &nbuf[0], (char *)&n_buf) == 0) {
		return(N_CERROR);
	}

	/* check version number, and calculate value for gdpmisc */
	if ((rfversion = rfsys(RF_VERSION, VER_CHECK, &n_buf.n_vhigh,
	    &n_buf.n_vlow)) < 0) {
		return(N_VERROR);
	}

	ndata = n_buf.nd_ndata;

	/* ndata now contains the other machine's data */
	/* do server side passwd verification			*/

	if (flag == SERVER) {
		/* extract domain name and uname from netname */

		dlen = strcspn(&ndata.n_netname[0], ".");
		ulen = strlen (&ndata.n_netname[0]) - dlen - 1;
		if ((domnam = malloc(dlen + 1)) == NULL) {
			return(N_ERROR);
		}

		if ((unam = malloc(ulen + 1)) == NULL) {
			return(N_ERROR);
		}

		(void) strncpy(domnam, &ndata.n_netname[0],  dlen);
		(void) strncpy(  unam, &ndata.n_netname[0] + dlen + 1, ulen);
		domnam[dlen] = unam[ulen] = '\0'; /*in case strncpy() didn't */

		/* verify passwd */

		switch (checkpw(DOMPASSWD, ndata.n_passwd, domnam, unam)) {
		case 0:
			if (rfsys(RF_VFLAG, V_GET, (int *)0, (int *)0) == V_SET) {
				LOG2(L_TRACE, "(%5d) leave: negotiate-RF_VFLAG checkpw\n", Logstamp);
				return(N_ERROR);
			}
			break;
		case 1:
			break;
		case 2:
			LOG2(L_TRACE, "(%5d) leave: negotiate-checkpw\n", Logstamp);
			return(N_PERROR);
		} /* end switch */

		/* send/receive proper machine response for type */

		if (ndata.n_hetero == ((long)MACHTYPE)) {
			ndata.n_hetero = NO_CONV;
		} else if ((ndata.n_hetero & BYTE_MASK)!=((long)MACHTYPE & BYTE_MASK)){
			ndata.n_hetero = ALL_CONV;
		} else {
			ndata.n_hetero = DATA_CONV;
		}
		if ((nbytes = tcanon("l", (char  *)&ndata.n_hetero, &nbuf[0], 0))==0){
			LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
			return(N_CERROR);
		}
		if (t_snd(fd, nbuf, nbytes, 0) != nbytes) {
			LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
			return(N_TERROR);
		}
	} else if (flag == CLIENT) { 
		/* CLIENT -- handle hetero input from SERVER	
		 */
		if (rf_rcv(fd, &nbuf[0], LONG_CLEN, &flgs) != LONG_CLEN) {
			LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
			return(N_TERROR);
		}
		if (fcanon("l", &nbuf[0], (char *)&ndata.n_hetero) == 0) {
			LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
			return(N_CERROR);
		}
	} else {
		LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
		return(N_FERROR);
	}
	
	if (rfversion > RFS1DOT0) {
		long  local_ngrpmax;
		long remote_ngrpmax;

		local_ngrpmax = (long)sysconf(_SC_NGROUPS_MAX);
		if (local_ngrpmax < 0) {
			LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
			return(N_CERROR);
		}
		local_ngrpmax = MIN(RF_MAXGROUPS, local_ngrpmax);
		if( error = sndrcv_int(fd, local_ngrpmax, &remote_ngrpmax))
			return error;
		*ngroups_maxp = (int)MIN(local_ngrpmax, remote_ngrpmax);
	} else {
		*ngroups_maxp = 0;
	}
        if (rfversion >= RFS2DOT1){
		long  local_aclmax;
		long remote_aclmax;

                local_aclmax = (long)sysconf(_SC_NACLS_MAX);
		if (local_aclmax < 0) {
                        LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
                        return(N_CERROR);
		}
		local_aclmax = MIN(RF_MAXACL, local_aclmax);
		if( error = sndrcv_int(fd, local_aclmax, &remote_aclmax))
                        return error;
		*aclmaxp = (int)MIN(local_aclmax, remote_aclmax);
	} else {
		*aclmaxp = 0;
	}

	if (rfversion >= RFS2DOT1){
                long	 local_mac;
		long	remote_mac;
		level_t level;

                local_mac =
                        lvlproc(MAC_GET, &level) == -1 && errno == ENOPKG
                        ? (long)RFS_NO_MAC
                        : (long)RFS_MAC;
		if( error = sndrcv_int(fd, local_mac, &remote_mac))
                        return error;
		*macp = (int)remote_mac;
	} else {
		*macp = RFS_NO_MAC;
	}
	return(rfversion);
}

int
checkpw(fname, pass, domnam, unam)
char *fname;
char *pass;
char *domnam;
char *unam;
{
	char filename[BUFSIZ];
	int buflen = 0;
	char buf[BUFSIZ];
	char *pw;
	char *m_name, *m_pass, *ptr;
	FILE *fp;

	char *crypt();

/*
 * checkpw returns 0 if the file was not found or the entry did not exist
 *		   1 if the passwd matched
 *		   2 if the passwd did not match
 */

	LOG2(L_TRACE, "(%5d) enter: checkpw\n", Logstamp);
	(void) sprintf(filename, fname, domnam);
	if ((fp = fopen(filename, "r")) != NULL) {
		while(fgets(buf, 512, fp) != NULL) {
			if (buf[strlen(buf)-1] == '\n')
				buf[strlen(buf)-1] = '\0';
			m_name = ptr = buf;
			buflen = strlen(ptr);
			while (*ptr != ':' && *ptr != '\0')
				ptr++;
			if (*ptr == ':')
				buflen--;
			*ptr = '\0';
			if (strcmp(unam, m_name) == 0) {
				if (buflen == strlen(unam)) {
					/* only name in passwd file */
					--ptr; /* point at NULL */
				}
				m_pass = ++ptr;
				while (*ptr != ':' && *ptr != '\0')
					ptr++;
				*ptr = '\0';
				pw = crypt(pass, m_pass);
				(void) fclose(fp);
				if (strcmp(pw, m_pass) == 0) {
					LOG2(L_TRACE, "(%5d) leave: checkpw\n", Logstamp);
					return(1);  /* correct */
				} else {
					LOG2(L_TRACE, "(%5d) leave: checkpw\n", Logstamp);
					return(2);  /* incorrect */
				}
			}
		} /* end while */
		(void) fclose(fp);
	}
	LOG2(L_TRACE, "(%5d) leave: checkpw\n", Logstamp);
	return(0);
}

int
sndrcv_int(fd, local, remotep)
	int	fd;
	long	local;
	long	*remotep;
{
	char	nbuf[200];
	int	flgs = 0;
	int	nbytes;

	if ((nbytes = tcanon("l", (char *)&local, &nbuf[0], 0)) == 0) {
		LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
		return(N_CERROR);
	}
	if (t_snd(fd, &nbuf[0], nbytes, 0) != nbytes) {
		LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
		return(N_TERROR);
	}
	if (rf_rcv(fd, &nbuf[0], LONG_CLEN, &flgs) != LONG_CLEN) {
		LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
		return(N_TERROR);
	}
	if (fcanon("l", &nbuf[0], (char *)remotep) == 0) {
		LOG2(L_TRACE, "(%5d) leave: negotiate\n", Logstamp);
		return(N_CERROR);
	}
	return 0;
}
