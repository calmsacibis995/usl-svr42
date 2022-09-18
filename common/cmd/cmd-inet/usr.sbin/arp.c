/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/arp.c	1.1.9.3"
#ident  "$Header: arp.c 1.3 91/09/19 $"

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


/*
 * arp - display, set, and delete arp table entries
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <sys/ksym.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <sys/sockio.h>
#include <fcntl.h>
#include <stropts.h>

#ifdef SYSV
#define bzero(s,n)	memset((s), 0, (n))
#define bcopy(f,t,l)	memcpy((t), (f), (l))
#endif /* SYSV */

extern int errno;
boolean_t memflg = B_FALSE;

main(argc, argv)
	char **argv;
{
	if (argc >= 2 && strcmp(argv[1], "-a") == 0) {
#ifdef SYSV
		char *kernel = "/unix";
#else
		char *kernel = "/vmunix";
#endif /* SYSV */
		char *mem = "/dev/kmem";

		if (argc >= 3)
			kernel = argv[2];
		if (argc >= 4) {
			mem = argv[3];
			memflg = B_TRUE;	/* non-default memory device 
							being used */
		}
		dump(kernel, mem);
		exit(0);
	}
	if (argc == 2) {
		get(argv[1]);
		exit(0);
	}
	if (argc >= 4 && strcmp(argv[1], "-s") == 0) {
		if (set(argc-2, &argv[2]))
			exit(1);
		exit(0);
	}
	if (argc == 3 && strcmp(argv[1], "-d") == 0) {
		delete(argv[2]);
		exit(0);
	}
	if (argc == 3 && strcmp(argv[1], "-f") == 0) {
		if (file(argv[2]))
			exit(1);
		exit(0);
	}
	usage();
	exit(1);
	/* NOTREACHED */
}

/*
 * Process a file to set standard arp entries
 */
file(name)
	char *name;
{
	FILE *fp;
	int i;
	char line[100], arg[5][50], *args[5];
	register int retval;

	if ((fp = fopen(name, "r")) == NULL) {
		fprintf(stderr, "arp: cannot open %s\n", name);
		exit(1);
	}
	args[0] = &arg[0][0];
	args[1] = &arg[1][0];
	args[2] = &arg[2][0];
	args[3] = &arg[3][0];
	args[4] = &arg[4][0];
	retval = 0;
	while(fgets(line, 100, fp) != NULL) {
		i = sscanf(line, "%s %s %s %s %s", arg[0], arg[1], arg[2],
			arg[3], arg[4]);
		if (i < 2) {
			fprintf(stderr, "arp: bad line: %s\n", line);
			retval = 1;
			continue;
		}
		if (set(i, args))
			retval = 1;
	}
	fclose(fp);
	return (retval);
}

/*
 * Set an individual arp entry 
 */
set(argc, argv)
	char **argv;
{
	struct arpreq ar;
	struct hostent *hp;
	struct sockaddr_in *sin;
	u_char *ea;
	char *host = argv[0], *eaddr = argv[1];

	argc -= 2;
	argv += 2;
	bzero((caddr_t)&ar, sizeof ar);
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == -1) {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "arp: %s: unknown host\n", host);
			return (1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
	ea = (u_char *)ar.arp_ha.sa_data;
	if (ether_aton(eaddr, ea))
		return (1);
	ar.arp_flags = ATF_PERM;
	while (argc-- > 0) {
		if (strncmp(argv[0], "temp", 4) == 0)
			ar.arp_flags &= ~ATF_PERM;
		if (strncmp(argv[0], "pub", 3) == 0)
			ar.arp_flags |= ATF_PUBL;
		if (strncmp(argv[0], "trail", 5) == 0)
			ar.arp_flags |= ATF_USETRAILERS;
		argv++;
	}
	
	if (arpioctl (SIOCSARP, (caddr_t) &ar) < 0) {
		perror(host);
		exit(1);
	}

	return (0);
}


/*
 * Display an individual arp entry
 */
get(host)
	char *host;
{
	struct arpreq ar;
	struct hostent *hp;
	struct sockaddr_in *sin;
	u_char *ea;

	bzero((caddr_t)&ar, sizeof ar);
	ar.arp_pa.sa_family = AF_INET;
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == -1) {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "arp: %s: unknown host\n", host);
			exit(1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}

	if (arpioctl(SIOCGARP, (caddr_t)&ar) < 0) {
		if (errno == ENXIO)
			printf("%s (%s) -- no entry\n",
			    host, inet_ntoa(sin->sin_addr));
		else
			perror("SIOCGARP");
		exit(1);
	}

	ea = (u_char *)ar.arp_ha.sa_data;
	printf("%s (%s) at ", host, inet_ntoa(sin->sin_addr));
	if (ar.arp_flags & ATF_COM)
		ether_print(ea);
	else
		printf("(incomplete)");
	if (ar.arp_flags & ATF_PERM) printf(" permanent");
	if (ar.arp_flags & ATF_PUBL) printf(" published");
	if (ar.arp_flags & ATF_USETRAILERS) printf(" trailers");
	printf("\n");
}

/*
 * Delete an arp entry 
 */
delete(host)
	char *host;
{
	struct arpreq ar;
	struct hostent *hp;
	struct sockaddr_in *sin;

	bzero((caddr_t)&ar, sizeof ar);
	ar.arp_pa.sa_family = AF_INET;
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr == -1) {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "arp: %s: unknown host\n", host);
			exit(1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}

	if (arpioctl(SIOCDARP, (caddr_t)&ar) < 0) {
		if (errno == ENXIO)
			printf("%s (%s) -- no entry\n",
			    host, inet_ntoa(sin->sin_addr));
		else {
			printf("errno: %d\n", errno);
			perror("SIOCDARP");
		}
		exit(1);
	}

	printf("%s (%s) deleted\n", host, inet_ntoa(sin->sin_addr));
}

#define	X_ARPTAB	0
#define	X_ARPTAB_SIZE	1
struct nlist nl[] = {
	{ "arptab" },
	{ "arptab_size" },
	{ "" },
};

/*
 *  Get information from kernel either through ioctl on /dev/kmem or by reading
 */
static int
getinfo(fd, index, buf, buflen)
int fd, index;
void *buf;
size_t buflen;
{
	struct mioc_rksym rks;
	if(!memflg) {
		rks.mirk_symname = nl[index].n_name;
		rks.mirk_buf = buf;
		rks.mirk_buflen = buflen;
		return(ioctl(fd,MIOC_READKSYM,&rks));
	}
	lseek(fd, (long)nl[X_ARPTAB_SIZE].n_value,0);
	if(read(fd, buf, buflen) != buflen)
		return(-1);
	else return(0);
}

		

/*
 * Dump the entire arp table
 */
dump(kernel, mem)
	char *kernel, *mem;
{
	int mf, arptab_size, sz;
	struct arptab *at; 
	struct hostent *hp;
	char *host;
	int bynumber = 0;

	if ( memflg && nlist(kernel, nl) == -1 ) {
		perror("nlist");
		exit(1);
	}
	mf = open(mem, 0);
	if(mf < 0) {
		fprintf(stderr, "arp: cannot open %s\n", mem);
		exit(1);
	}
	if(getinfo(mf,X_ARPTAB_SIZE, &arptab_size, sizeof(arptab_size)) == 0) {
		if (arptab_size <=0 || arptab_size > 1000) {
			fprintf(stderr, "arp: %s: invalid arptab_size %d\n", kernel,arptab_size);
			exit(1);
		}
		sz = arptab_size * sizeof (struct arptab);
		at = (struct arptab *)malloc(sz);
		if (at == NULL) {
			fprintf(stderr, "arp: can't get memory for arptab\n");
			exit(1);
		}
		if(getinfo(mf, X_ARPTAB, at, sz) != 0) {
			perror("arp: error reading arptab");
			exit(1);
		}
	}
	else {
		fprintf(stderr,"arp: cannot read arptab_size from %s\n",kernel);
		exit(1);
	}
	close(mf);
	for (; arptab_size-- > 0; at++) {
		if (at->at_iaddr.s_addr == 0 || at->at_flags == 0)
			continue;
		if (bynumber == 0)
			hp = gethostbyaddr((caddr_t)&at->at_iaddr,
			    sizeof at->at_iaddr, AF_INET);
		else
			hp = 0;
		if (hp)
			host = hp->h_name;
		else {
			host = "?";
			if (h_errno == TRY_AGAIN)
				bynumber = 1;
		}
		printf("%s (%s) at ", host, inet_ntoa(at->at_iaddr));
		if (at->at_flags & ATF_COM)
			ether_print(at->at_enaddr);
		else
			printf("(incomplete)");
		if (at->at_flags & ATF_PERM) printf(" permanent");
		if (at->at_flags & ATF_PUBL) printf(" published");
		if (at->at_flags & ATF_USETRAILERS) printf(" trailers");
		printf("\n");
	}
}

ether_print(cp)
	u_char *cp;
{

	printf("%x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
}

ether_aton(a, n)
	char *a;
	u_char *n;
{
	int i, o[6];

	i = sscanf(a, "%x:%x:%x:%x:%x:%x", &o[0], &o[1], &o[2],
					   &o[3], &o[4], &o[5]);
	if (i != 6) {
		fprintf(stderr, "arp: invalid Ethernet address '%s'\n", a);
		return (1);
	}
	for (i=0; i<6; i++)
		n[i] = o[i];
	return (0);
}

usage()
{
	fprintf(stderr,"Usage: arp hostname\n");
#ifdef SYSV
	fprintf(stderr,"       arp -a [/unix] [/dev/kmem]\n");
#else
	fprintf(stderr,"       arp -a [/vmunix] [/dev/kmem]\n");
#endif SYSV
	fprintf(stderr,"       arp -d hostname\n");
	fprintf(stderr,"       arp -s hostname ether_addr [temp] [pub] [trail]\n");
	fprintf(stderr,"       arp -f filename\n");
}

arpioctl (name, arg)
int name;
caddr_t arg;
{
	int d;
	struct strioctl sti;
	int ret;

	d = open ("/dev/arp", O_RDWR);
	if (d < 0) {
                perror("arp: open");
                exit(1);
        }
	sti.ic_cmd = name;
	sti.ic_timout = 0;
	sti.ic_len = sizeof (struct arpreq);
	sti.ic_dp = arg;
	ret = ioctl(d, I_STR, (caddr_t)&sti);
	close(d);
	return (ret);
}
