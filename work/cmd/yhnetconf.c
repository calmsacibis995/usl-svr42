

/*****************************************************
 *
 * UNIX SVR 4.2 STREAMS YHTP/YHNP(YHARP/YHAPP)
 *                                     - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/

/* 
 * set the ethernet driver's charactor;
 * set the dlpi's interface name (IF);
 * set the IF's OSI-address;
 * config the router's address;
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <net/yhif.h>
#include <netinet/yhin.h>
#include <net/yhif_arp.h>
#include <netinet/yhif_ether.h>
#include <stropts.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>

#define	bcopy(a,b,l) 	memcpy((b),(a),(l))
#define	bzero(s,n)   	memset((s),0,(n))

extern int      errno;
struct ifreq    ifr;
struct sockaddr_in sin = {AF_OSI};
struct sockaddr_in broadaddr;
struct sockaddr_in netmask = {AF_OSI};
struct sockaddr_in ipdst = {AF_OSI};
int             flags;
int             metric;
int             setaddr;
int             setmask;
int             setbroadaddr;
int             setipdst;
int             s;

int             setifflags(), setifaddr(), setifdstaddr(), setifnetmask();
int             setifmetric(), setifbroadaddr(), setifipdst(), ifdetach();

#define NEXTARG         0xffffff

struct cmd {
        char           *c_name;
        int             c_parameter;    /* NEXTARG means next argv */
        int             (*c_func) ();
}               cmds[] = {
        {
                                "up", IFF_UP, setifflags
        }              ,
        {
                                "down", -IFF_UP, setifflags
        }              ,
        {
                                "trailers", -IFF_NOTRAILERS, setifflags
        }              ,
        {
                                "-trailers", IFF_NOTRAILERS, setifflags
        }              ,
        {
                                "yharp", -IFF_NOARP, setifflags
        }              ,
        {
                                "-yharp", IFF_NOARP, setifflags
        }              ,
        {
                                "debug", IFF_DEBUG, setifflags
        }              ,
        {
                                "-debug", -IFF_DEBUG, setifflags
        }              ,
        {
                                "metric", NEXTARG, setifmetric
        }              ,
        {
                                "broadcast", NEXTARG, setifbroadaddr
        }              ,
        {
                                "ipdst", NEXTARG, setifipdst
        }              ,
        {
                                "detach", 0, ifdetach
        }              ,
        {
                                0, 0, setifaddr
        }              ,
        {
                                0, 0, setifdstaddr
        }              ,
};

int             in_status(), in_getaddr();

/* Known address families */
struct afswtch {
        char           *af_name;
        short           af_af;
        char           *af_dev;
        int             (*af_status) ();
        int             (*af_getaddr) ();
}               afs[] = {
        {
                                "osi", AF_OSI, "/dev/yhnp", in_status, in_getaddr
        }              ,
        {
                                0, 0, 0, 0, 0
        }
};

struct afswtch *afp;            /* the address family being set or asked
                                 * about */

main(argc, argv)
        int             argc;
        char           *argv[];
{
	char *name;

        if (argc < 2) {
                usage();
        }
        argc--, argv++;
	name = *argv;
        argc--, argv++;
	if (strcmp(name, "-f") == 0) {
		if (!(argc == 1)){
			usage();
		}
		if (file(argv[0]))
			exit(1);
		exit(0);
	}
	if (!strcmp(name, "-s")){ 
		setyharp(argc, argv);
	} else if (strcmp(name, "-a")) {
		doyhnetconf(name, argc, argv);
	} else {
		struct ifreq *ir = NULL;
		struct strioctl ioc;
		int i, entries = 8, s;

		/* do SIOCGIFCONF until we are sure we have everything */
		s = open("/dev/yhnp", O_RDONLY);
		if (s < 0)
			Perror("yhnetconf: open");

		do {
			entries *= 2;
			ir = (struct ifreq *)realloc(ir, sizeof(struct ifreq) * entries);
			if (ir == NULL)
				Perror("VM : Not enough memory");

			ioc.ic_cmd = SIOCGIFCONF;
			ioc.ic_timout = 0;
			ioc.ic_len = sizeof(struct ifreq) * entries;
			ioc.ic_dp = (void *)ir;

			if (ioctl(s, I_STR, (char *) &ioc) < 0)
				Perror("ioctl SIOCGIFCONF");
		} while ((ioc.ic_len/sizeof(struct ifreq)) == entries);

		close(s);

		for (i=0; i < (ioc.ic_len/(sizeof(struct ifreq))); ++i)
			doyhnetconf(ir[i].ifr_name, argc, argv);
	}

        exit(0);
}

doyhnetconf(name, argc, argv)
	char *name;
	int argc;
	char **argv;
{
        int             af = AF_OSI;

        if (argc > 0) {
                struct afswtch *myafp;

                for (myafp = afp = afs; myafp->af_name; myafp++)
                        if (strcmp(myafp->af_name, *argv) == 0) {
                                afp = myafp;
                                argc--;
                                argv++;
                                break;
                        }
                af = ifr.ifr_addr.sa_family = afp->af_af;
        } else {
                afp = afs;
        }
        s = open(afp->af_dev, O_RDONLY);
        if (s < 0) {
                perror("yhnetconf: open");
		close(s);
		return 1;
        }
        strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
        if (ifioctl(s, SIOCGIFFLAGS, (caddr_t) & ifr) < 0) {
                Perror("ioctl (SIOCGIFFLAGS)");
        }
        flags = ifr.ifr_flags;
        if (ifioctl(s, SIOCGIFMETRIC, (caddr_t) & ifr) < 0)
                perror("ioctl (SIOCGIFMETRIC)");
        else
                metric = ifr.ifr_metric;
        if (argc == 0) {
                status(name);
		close(s);
		return 0;
        }
        while (argc > 0) {
                register struct cmd *p;

                for (p = cmds; p->c_name; p++)
                        if (strcmp(*argv, p->c_name) == 0)
                                break;
                if (p->c_name == 0 && setaddr)
                        p++;    /* got src, do dst */
                if (p->c_func) {
                        if (p->c_parameter == NEXTARG) {
                                (*p->c_func) (argv[1]);
                                argc--, argv++;
                        } else
                                (*p->c_func) (*argv, p->c_parameter);
                }
                argc--, argv++;
        }
        if ((setmask || setaddr) && (af == AF_OSI)) {
                /*
                 * If setting the address and not the mask, clear any
                 * existing mask and the kernel will then assign the default.
                 * If setting both, set the mask first, so the address will
                 * be interpreted correctly. 
                 */
                ifr.ifr_addr = *(struct sockaddr *) & netmask;
                if (ifioctl(s, SIOCSIFNETMASK, (caddr_t) & ifr) < 0)
                        Perror("ioctl (SIOCSIFNETMASK)");
        }
        if (setaddr) {
                ifr.ifr_addr = *(struct sockaddr *) & sin;
                if (ifioctl(s, SIOCSIFADDR, (caddr_t) & ifr) < 0)
                        Perror("ioctl (SIOCSIFADDR)");
        }
        if (setbroadaddr) {
                ifr.ifr_addr = *(struct sockaddr *) & broadaddr;
                if (ifioctl(s, SIOCSIFBRDADDR, (caddr_t) & ifr) < 0)
                        Perror("ioctl (SIOCSIFBRDADDR)");
        }

	close(s);
	return 0;
}

/* ARGSUSED */
setifaddr(addr, param)
        char           *addr;
        short           param;
{
        /*
         * Delay the ioctl to set the interface addr until flags are all set.
         * The address interpretation may depend on the flags, and the flags
         * may change when the address is set. 
         */
        setaddr++;
        (*afp->af_getaddr) (addr, &sin);
}

setifnetmask(addr)
        char           *addr;
{
        in_getaddr(addr, &netmask);
        setmask++;
}

setifbroadaddr(addr)
        char           *addr;
{
        (*afp->af_getaddr) (addr, &broadaddr);
        setbroadaddr++;
}

setifipdst(addr)
        char           *addr;
{
        in_getaddr(addr, &ipdst);
        setipdst++;
}

/* ARGSUSED */
setifdstaddr(addr, param)
        char           *addr;
        int             param;
{

        (*afp->af_getaddr) (addr, &ifr.ifr_addr);
        if (ifioctl(s, SIOCSIFDSTADDR, (caddr_t) & ifr) < 0)
                Perror("ioctl (SIOCSIFDSTADDR)");
}

setifflags(vname, value)
        char           *vname;
        short           value;
{
        if (ifioctl(s, SIOCGIFFLAGS, (caddr_t) & ifr) < 0) {
                Perror("ioctl (SIOCGIFFLAGS)");
                exit(1);
        }
        flags = ifr.ifr_flags;

        if (value < 0) {
                value = -value;
                flags &= ~value;
        } else
                flags |= value;
        ifr.ifr_flags = flags;
        if (ifioctl(s, SIOCSIFFLAGS, (caddr_t) & ifr) < 0)
                Perror(vname);
}

setifmetric(val)
        char           *val;
{
        ifr.ifr_metric = atoi(val);
        if (ifioctl(s, SIOCSIFMETRIC, (caddr_t) & ifr) < 0)
                perror("ioctl (set metric)");
}

ifdetach()
{
#ifdef NOTDEF
        if (ifioctl(s, SIOCIFDETACH, (caddr_t) & ifr) < 0)
                perror("ioctl (detach)");
#endif NOTDEF
}

#define IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\011INTELLIGENT"

/*
 * Print the status of the interface.  If an address family was specified,
 * show it and it only; otherwise, show them all. 
 */
status(name)
	char *name;
{
        register struct afswtch *p = afp;
        short           af = ifr.ifr_addr.sa_family;

        printf("%s: ", name);
/*
        printb("flags", flags, IFFBITS);
        if (metric)
			printf(" metric %d", metric);
		putchar('\n');
*/
		if ((p = afp) != NULL) {
			(*p->af_status) (1);
		} else
			for (p = afs; p->af_name; p++) {
				ifr.ifr_addr.sa_family = p->af_af;
				afp = p;
				close(s);
				s = open(afp->af_dev, O_RDONLY);
				if (s < 0) {
					if (errno == EPROTONOSUPPORT || errno == ENODEV)
						continue;
					Perror("yhnetconf: status open");
				}
				(*p->af_status) (0);
			}
	}

	in_status(force)
		int             force;
	{
		struct sockaddr_in *sin;
		char           *yhnet_ntoa();

		if (ifioctl(s, SIOCGIFADDR, (caddr_t) & ifr) < 0) {
			if (errno == EADDRNOTAVAIL || errno == EPROTONOSUPPORT) {
				if (!force)
					return;
				memset((char *) &ifr.ifr_addr, '\0',
					sizeof(ifr.ifr_addr));
			} else
				perror("ioctl (SIOCGIFADDR)");
		}
		sin = (struct sockaddr_in *) & ifr.ifr_addr;
		printf("\tYHNET  %s ", yhnet_ntoa(sin->sin_addr));
        if (flags & IFF_BROADCAST) {
                if (ifioctl(s, SIOCGIFBRDADDR, (caddr_t) & ifr) < 0) {
                        if (errno == EADDRNOTAVAIL)
                                memset((char *) &ifr.ifr_addr,
                                       '\0', sizeof(ifr.ifr_addr));
                        else
                                perror("ioctl (SIOCGIFADDR)");
                }
                sin = (struct sockaddr_in *) & ifr.ifr_addr;
                if (sin->sin_addr.s_addr != 0)
                        printf("broadcast %s", yhnet_ntoa(sin->sin_addr));
        }
        putchar('\n');
}

Perror(cmd)
        char           *cmd;
{
        extern int      errno;

        fprintf(stderr, "yhnetconf: ");
        switch (errno) {

        case ENXIO:
                fprintf(stderr, "%s: no such interface\n", cmd);
                break;

        case EPERM:
                fprintf(stderr, "%s: permission denied\n", cmd);
                break;

        case ENODEV:
                fprintf(stderr, "%s: family not loaded\n", cmd);
                break;

        default:
                perror(cmd);
        }
        exit(1);
}

struct in_addr  inet_makeaddr();

in_getaddr(s, saddr)
        char           *s;
        struct sockaddr *saddr;
{
        register struct sockaddr_in *sin = (struct sockaddr_in *) saddr;
        struct hostent *hp;
        struct netent  *np;
        int             val;
	u_long yhnet_addr();

        if (s == NULL) {
                fprintf(stderr, "yhnetconf: address argument required\n");
                usage();
        }
        sin->sin_family = AF_OSI;
        val = yhnet_addr(s);
        if (val != -1) {
                sin->sin_addr.s_addr = val;
                return;
        }
        hp = gethostbyname(s);
        if (hp) {
                sin->sin_family = AF_OSI;
                memcpy((char *) &sin->sin_addr, hp->h_addr, hp->h_length);
                return;
        }
        np = getnetbyname(s);
        if (np) {
                sin->sin_family = AF_OSI; /*np->n_addrtype;*/
                sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
                return;
        }
      	fprintf(stderr, "yhnetconf: %s: bad value\n", s);
        exit(1);
}

/*
 * Print a value a la the %b format of the kernel's printf 
 */
printb(s, v, bits)
        char           *s;
        register char  *bits;
        register unsigned short v;
{
        register int    i, any = 0;
        register char   c;

        if (bits && *bits == 8)
                printf("%s=%o", s, v);
        else
                printf("%s=%x", s, v);
        bits++;
        if (bits) {
                putchar('<');
                while (i = *bits++) {
                        if (v & (1 << (i - 1))) {
                                if (any)
                                        putchar(',');
                                any = 1;
                                for (; (c = *bits) > 32; bits++)
                                        putchar(c);
                        } else
                                for (; *bits > 32; bits++);
                }
                putchar('>');
        }
}

ifioctl(s, cmd, arg)
        char           *arg;
{
        struct strioctl ioc;

        ioc.ic_cmd = cmd;
        ioc.ic_timout = 0;
        ioc.ic_len = sizeof(struct ifreq);
        ioc.ic_dp = arg;
        return (ioctl(s, I_STR, (char *) &ioc));
}


/*
 * Convert network-format YHNET address
 * to base 256 d.d.d representation.
 */
/*
char *
yhnet_ntoa(in)
	struct in_addr in;
{
	static char b[18];
	register char *p;
	register int i1,i2,hid;

	p = (char *)&in;
	i1 = (int )p[2];
	i2 = (int )p[3];
	hid = (i1 << 8) | (i2 & 0x0ff);
#define	UC(b)	(((int)b)&0xff)
	sprintf(b, "%d:%d:%d", p[0]&0x3f, UC(p[1]), hid&0xffff);
	return (b);
}
*/
/*
 * YHNET address interpretation routine.
 * All the network library routines call this
 * routine to interpret entries in the data bases
 * which are expected to be an address.
 * The value returned is in network order.
 */
/*
u_long
yhnet_addr(cp)
	register char *cp;
{
	register char c;
	register u_long val;
	u_long parts[3], *pp=parts;
	register int first = 0;

again:
	val = 0;
	while(c = *cp){
	    if (isdigit(c)){
		if((c - '0') >= 10)
			break;
		val = val*10 + (c - '0');
		cp++;
		continue;
	    }
	    break;
	}
	if (!first){
		first++;
		val = val + 128;
	}
	if (*cp == ':'){
		if (pp >= parts + 3)
			return (-1);
		*pp++ = val, cp++;
		goto again;
	}
	if(*cp && !isspace(*cp))
		return (-1);
	*pp++ = val;
	val = ((parts[0]&0xff)<<24)|((parts[1]&0xff)<<16)|(parts[2]&0xffff);
	val = htonl(val);
	return (val);
}

*/
/*
 * Return the network number from an internet
 * address; handles class a/b/c network #'s.
 */
yhnet_netof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return (((i)&IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);
	else if (IN_CLASSB(i))
		return (((i)&IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);
	else
		return (((i)&IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
}

usage()
{
        fprintf(stderr, "usage: yhnetconf interface\n%s%s",
                "\t\t[ [ address ] [ up | down ] ]",
                " [ trailers | -trailers ]\n");
	fprintf(stderr,"       yhnetconf -s routeaddr ether_addr \n");
	fprintf(stderr,"       yhnetconf -f filename\n");
	
        exit(1);
}

/*
 * Process a file to set standard router's yharp entries
 */
file(name)
	char *name;
{
	FILE *fp;
	int i;
	char line[100], arg[5][50], *args[5];
	register int retval;

	if ((fp = fopen(name, "r")) == NULL) {
		fprintf(stderr, "yhnetconf: cannot open %s\n", name);
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
			fprintf(stderr, "yhnetconf: bad line: %s\n", line);
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
 * Set an individual yharp entry 
 */
set(argc, argv)
	char **argv;
{
	struct arpreq ar;
	struct hostent *hp;
	struct sockaddr_in *sin;
	u_char *ea;
	char *host = argv[0], *eaddr = argv[1];
	u_long yhnet_addr();

	argc -= 2;
	argv += 2;
	bzero((caddr_t)&ar, sizeof ar);
	sin = (struct sockaddr_in *)&ar.yharp_pa;
	sin->sin_family = AF_OSI;
	sin->sin_addr.s_addr = yhnet_addr(host);
	if (sin->sin_addr.s_addr == -1) {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "yhnetconf: %s: unknown host\n", host);
			return (1);
		}
		bcopy((char *)hp->h_addr, (char *)&sin->sin_addr,
		    sizeof sin->sin_addr);
	}
	ea = (u_char *)ar.yharp_ha.sa_data;
	if (ether_aton(eaddr, ea))
		return (1);
	ar.yharp_flags = ATF_PERM;
	while (argc-- > 0) {
		if (strncmp(argv[0], "temp", 4) == 0)
			ar.yharp_flags &= ~ATF_PERM;
		if (strncmp(argv[0], "pub", 3) == 0)
			ar.yharp_flags |= ATF_PUBL;
		if (strncmp(argv[0], "trail", 5) == 0)
			ar.yharp_flags |= ATF_USETRAILERS;
		argv++;
	}
	
	if (yharpioctl (SIOCSROUTETAB, (caddr_t) &ar) < 0) {
		perror(host);
		exit(1);
	}

	return (0);
}

/*
 * yharp - display, set, and delete yharp table entries
 */

setyharp(argc, argv)
	int argc;
	char **argv;
{
	struct arpreq ar;
	struct hostent *hp;
	struct sockaddr_in *lsin;
	u_char *ea;
	char *host = argv[0], *eaddr = argv[1];
	u_long yhnet_addr();

	argc -= 2;
	argv += 2;
	bzero((caddr_t)&ar, sizeof ar);
	lsin = (struct sockaddr_in *)&ar.yharp_pa;
	lsin->sin_family = AF_OSI;
	lsin->sin_addr.s_addr = yhnet_addr(host);
	if (lsin->sin_addr.s_addr == -1) {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "yharp: %s: unknown host\n", host);
			exit(-1);
		}
		bcopy((char *)hp->h_addr, (char *)&lsin->sin_addr,
		    sizeof lsin->sin_addr);
	}
	ea = (u_char *)ar.yharp_ha.sa_data;
	if (ether_aton(eaddr, ea))
		exit(-1);
	ar.yharp_flags = ATF_PERM;
	while (argc-- > 0) {
		if (strncmp(argv[0], "temp", 4) == 0)
			ar.yharp_flags &= ~ATF_PERM;
		if (strncmp(argv[0], "pub", 3) == 0)
			ar.yharp_flags |= ATF_PUBL;
		if (strncmp(argv[0], "trail", 5) == 0)
			ar.yharp_flags |= ATF_USETRAILERS;
		argv++;
	}
	
	if (yharpioctl (SIOCSARP, (caddr_t) &ar) < 0) {
		perror("yhnetconf set route");
		exit(1);
	}

	return (0);
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
		fprintf(stderr, "yhnetconf: invalid Ethernet address '%s'\n", a);
		return (1);
	}
	for (i=0; i<6; i++)
		n[i] = o[i];
	return (0);
}


yharpioctl (name, arg)
int name;
caddr_t arg;
{
	int d;
	struct strioctl sti;
	int ret;
	
	d = open ("/dev/yharp", O_RDWR);
	if (d < 0) {
                perror("yhnetconf : open yharp");
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
