/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)adv:adv.c	1.14.15.4"
#ident  "$Header: adv.c 1.2 91/06/25 $"
#include  <stdio.h>
#include  <fcntl.h>
#include  <ctype.h>
#include  <string.h>
#include  <nserve.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <sys/errno.h>
#include  <sys/rf_sys.h>
#include  <stdlib.h>

#define   ALIAS     "alias"			/* keyword for alias entries */
#define	  ALTAB     "/etc/host.alias"		/* alias file */
#define   SHAREFILE "/etc/dfs/sharetab"		/* share table */
#define   SHARELOCK "/etc/dfs/sharetab.lck"	/* lock file for sharetab */
#define   TEMPSHARE "/etc/dfs/tmp.sharetab"	/* temp file */
#define   NUM_MACHS 100
#define   MASK      (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define   BUFSIZE   512
#define   MAXFIELD  5
#define   RES	    1
#define   MACH      2
#define   SZ_CLIENT (SZ_DELEMENT + SZ_MACH + 1)
#define   SAME 	    0

static	int	pos		= 0;
static	int	aflag		= 0;
static	int	mflag		= 0;
static	int	dflag		= 0;
static	int	aliases		= 0;
static	int	low_index	= 0;

static	int	atbl_syntax();
static	int	isalias();
static	int	in_clist();
static	int	invalid();
static	int	verify();
static	int	ns_adv();
static	void	rpterr();
static	void	get_data();
static	void	add_entry();
static	void	update_entry();
static	void	add_client();
static	void	search_atbl();
static	void	creatlist();
static	void	process_atbl();
static	void	creat_atbl();
static	void	list_advlog();
static	void	alloc_mem();

#define	Printf	(void)printf
#define	Fprintf	(void)fprintf

extern	int	v_resname();	/* from libns */
extern	int	v_uname();	/* from libns */
extern	int	v_dname();	/* from libns */
extern	void	nserror();	/* from libns */
extern	int	rfsys();	/* undocumented system call */
extern	int	setlog();	/* undocumented libns  call */


static	char	*fieldv[MAXFIELD];
static	char	*atab_buf;	      /* pointer to alias table buffer */
static	char	**clist	= NULL;       /* pointer to list of clients    */
static	char	*cmd;

static char	*shareflg[] = {"rw", "ro"};

static	struct	altab {
	char	*alname;	      /* pointer to alias name	       */
	char	*alist;		      /* pointer to corresponding list */
} *altptr;

static	struct	stat	stbuf;

extern	int	errno;
extern	int	optind, opterr;
extern	char	*optarg;
extern	char	*dompart();
extern	char	*namepart();

int
main(argc,argv)
	int  argc;
	char *argv[];
{

	int	chr, temprec;
	short	req_type;
	short	roflag = A_RDWR, errflag = 0;
	char	*path;
	char	*resource;
	char	*desc = "";
	char	*usage1 = "adv [ [-r] [-d desc] resource pathname [clients..] ]";
	char	*usage2 = "adv -m resource -d desc [clients..]";
	char	*usage3 = "adv -m resource [-d desc] clients..";
	char	**mlist;

	cmd = argv[0];

	/*	
	 *	If no arguments, list the local advertise file, else
	 *	check for optional arguments.
	 */

	if (argc <= 1) {
		list_advlog();
		exit(0);
	}

	/*
	 *	Check for optional arguments.
	 *	-r : read-only
	 *	-m : modify description and/or client information
	 *	-d : description
	 */

	while ((chr = getopt(argc,argv,"rm:d:l:")) != EOF)
		switch(chr) {
		case 'r':
			if (roflag == A_RDONLY || mflag)
				errflag = 1;
			else
				roflag = A_RDONLY;
			break;
		case 'm':
			if (mflag || roflag == A_RDONLY)
				errflag = 1;
			else {
				mflag = 1;
				resource = optarg;
			}
			break;
		case 'd':
			if (dflag)
				errflag = 1;
			else {
				dflag = 1;
				desc = optarg;
				if (strlen(desc) > (size_t)SZ_DESC) {
					optarg[SZ_DESC] = '\0';
					Fprintf(stderr,"%s: warning: description truncated to <%s>\n",cmd,desc);
				}
			}
			break;
		case 'l':
			setlog(optarg);
			break;
		case '?':
			errflag = 1;
			break;
		}

	/*
	 *	If a resource is being advertised, there 
	 *	must be at least 2 arguments, the symbolic
	 *	name and the directory being advertised.
	 */

	if (errflag || (!mflag ? argc <= optind + 1 : 0)) {
		Fprintf(stderr,"Usage:  %s\n\t%s\n\t%s\n",usage1,usage2,usage3);
		exit(1);
	}

	/*
	 *	If information associated with an advertised resource is
	 *	being modified, the description or client fields 
	 *	must be provided.
	 */

	if (mflag && (!dflag && argv[optind] == NULL)) {
		Fprintf(stderr,"Usage:  %s\n\t%s\n\t%s\n",usage1,usage2,usage3);
		exit(1);
	}

#ifdef   OLDSEC
	if (geteuid() != (uid_t)0) {
		Fprintf(stderr,"%s: must be super-user\n",cmd);
		exit(1);
	}
#endif /*OLDSEC*/

	if (!mflag)
		resource = argv[optind];

	/*
	 *	Determine if resource name contains '/', '.', or non-printable
	 *	characters.
	 */

	if (v_resname(resource) != 0) {
		Fprintf(stderr,"%s: invalid characters specified in <%s>\n",cmd,resource);
		Fprintf(stderr,"%s: resource name cannot contain '/', '.' ,' ' or non-printable characters\n",cmd);
		exit(1);
	}

	if (strlen(resource) > (size_t)SZ_RES) {
		resource[SZ_RES] = '\0';
		Fprintf(stderr,"%s: warning: resource name truncated to <%s>\n",cmd,resource);
	}

	if (!mflag) {
		path = argv[optind + 1];
		if (*path != '/') {
			Fprintf(stderr,"%s: full pathname required\n",cmd);
			exit(1);
		}
		mlist = &argv[optind + 2];
	} else
		mlist = &argv[optind];

	/*
	 *	Create and initialize the alias table if the
	 *	client field and /etc/host.alias exist, and form
	 *	the authorized client list.
	 */

	if (mlist[0] != NULL) {
		if ((stat(ALTAB,&stbuf) != -1) && stbuf.st_size != 0)
			creat_atbl(stbuf);

		creatlist(mlist);
		if (clist[0] == NULL) {
			Fprintf(stderr,"%s: no valid client names\n",cmd);
			exit(1);
		}
		roflag |= A_CLIST;
	}

	if (mflag) {
		req_type = NS_MODADV;
		roflag |= A_MODIFY;
	} else
		req_type = NS_ADV;	

	/*
	 *	Lock a temporary file to prevent many advertises from
	 *	updating "/etc/dfs/sharetab" at the same time.
	 */

	if ((temprec = creat(SHARELOCK, (S_IWUSR|S_IRUSR))) == -1 ||
	     lockf(temprec, F_LOCK, 0L) < 0) {
		Fprintf(stderr, "%s: warning: cannot lock temp file <%s>\n",cmd,SHARELOCK);
	}
	
	/*
	 *	Advertise a new resource,
	 *	or modify the client list of an advertised resource.
	 */

	if (rfsys(RF_ADVFS, path, resource, roflag, clist) == -1) {
		rpterr(resource,path);
		exit(1);	
	}

	roflag &= ~(A_CLIST | A_MODIFY);

	/*
	 *	Send the advertise information to the name server.
	 */

	if (ns_adv(req_type,path,resource,roflag,desc,clist) == FAILURE) {
		nserror(cmd);
		if (req_type == NS_ADV)
			rfsys(RF_UNADVFS, resource);
		exit(1);
	}

	if (mflag)
		update_entry(resource,path,roflag,desc,mlist);
	else
		add_entry(resource,path,roflag,desc,mlist);

	exit(0);
	/* NOTREACHED */
}

/* list data in /etc/dfs/sharetab in adv(1) format */

static void
list_advlog()
{
	char	advbuf[BUFSIZE];
	char	*clnt, *s;
	FILE	*fp;


	if (stat(SHAREFILE,&stbuf) != -1) {
		if ((fp = fopen(SHAREFILE, "r")) == NULL) {
			Fprintf(stderr,"%s: cannot open <%s>\n",cmd,SHAREFILE);
			exit(1);
		}
		while (fgets(advbuf,BUFSIZE,fp)) {
			get_data(advbuf);
			if (strcmp(fieldv[2], "rfs") == 0) {
				Printf("%-14.14s", fieldv[1]);
				Printf("  %s  ", fieldv[0]);
				Printf((strncmp(fieldv[3], "ro", 2) == 0) ?
					"read-only" : "read/write");
				/* ala dfs.cmds:share/share.c list_res() */
				if (*fieldv[4])		/* description */
					Printf("  \"%s\" ", fieldv[4]);
				else	Printf("  \"\"  ");
				/* show clients, if any */
				if (fieldv[3][0] && fieldv[3][1] &&
				    (fieldv[3][2] == '='))
					for (clnt=s=fieldv[3]+3; s && *s; clnt = s){
						if (s = strchr(s, ':'))
							*s++ = '\0';
						Printf(" %s", clnt);
					}
				else
					Printf(" unrestricted");
				Printf("\n");
			}
		}
		(void)fclose(fp);
	}
}

static void
creat_atbl(atab)
struct	stat	atab;
{

	int	fd;
	char    *end_of_atab;


	/*
	 *	Allocate a buffer for the alias table and read
	 *	in the contents of /etc/host.alias (alias file).
	 */

	if ((atab_buf = malloc((unsigned)(atab.st_size +1))) == NULL) {
		Fprintf(stderr,"%s: cannot allocate memory for the alias table\n",cmd);
		exit(1);
	}

	if ((fd = open(ALTAB, O_RDONLY)) == -1) {
		Fprintf(stderr,"%s: cannot open <%s>\n",cmd,ALTAB);
		exit(1);
	}

	if (read(fd,atab_buf,(unsigned)atab.st_size) != atab.st_size) {
		Fprintf(stderr,"%s: cannot read <%s>\n",cmd,ALTAB);
		exit(1);
	}
	end_of_atab = atab_buf + atab.st_size;
	*end_of_atab = '\0';
	(void)close(fd);
	
	/*
	 *	Check the syntax of each alias entry in /etc/host.alias
	 *	and determine the total number of alias entries.
	 */

	if ((aliases = atbl_syntax(atab_buf)) == -1)
		exit(1);

	/*
	 *	Allocate a structure for each alias entry. Each 
	 *	structure consists of two pointers, one for the 
	 *	alias name and the other for its corresponding list.
	 */
	
	if ((altptr = malloc(sizeof(struct altab)*aliases))==NULL) {
		Fprintf(stderr,"%s: cannot allocate memory for the alias table\n",cmd);
		exit(1);
	}
	process_atbl(altptr,atab_buf);
	aflag++;
}

static int
atbl_syntax(atab_ptr)
char	*atab_ptr;
{

	int	line = 0;
	int	errflg = 0;
	int	num_aliases = 0;

	/*
	 *	Check the alias table for syntactic errors,
	 * 	and determine the number of alias entries.
	 *	If the new line character ('\n') of an alias entry
	 *	is immediately preceded by a backslash then the 
	 *	next line is a continuation, or else it represents
	 *	the next alias entry.
	 */

	while (*atab_ptr != '\0') {
		atab_ptr += strspn(atab_ptr," \t");
		while (*atab_ptr == '\n') {
			atab_ptr++;
			atab_ptr += strspn(atab_ptr," \t");
			line++;
		}
		if (*atab_ptr == '\0')
			break;

		if (strncmp(atab_ptr,ALIAS,strlen(ALIAS)) == SAME) {
			atab_ptr = strpbrk(atab_ptr,"\n");
			line++;
			while (*(atab_ptr - 1) == '\\') {
				if (*(atab_ptr + 1) == '\0') {
					Fprintf(stderr,"%s: syntax error in <%s>, line <%d>\n",cmd,ALTAB,line);
					errflg++;
					break;
				}
				*(atab_ptr - 1) = ' ';
				*atab_ptr = ' ';
				atab_ptr = strpbrk(atab_ptr,"\n");
				line++;
			}
			num_aliases++;
		} else {
			atab_ptr = strpbrk(atab_ptr,"\n");
			line++;
			errflg++;
			Fprintf(stderr,"%s: syntax error in <%s>, line <%d>\n",cmd,ALTAB,line);
		}
		atab_ptr++;
	}
	if (errflg)
		return -1;
	else
		return num_aliases;
}

static void
process_atbl(altbl,atab_ptr)
struct	altab	*altbl;
char	*atab_ptr;
{

	register int	i;
	int	 	inval_cnt = 0;
	register char	*buf;

	/*
	 *	Set the pointers for each alias entry in the alias table
	 *	so that the first pointer points to the alias name 
	 * 	while the second points to the alias list.
	 *	NOTE: alias entries without an alias list are ignored.
	 */

	for (i = 0; i < aliases; i++) {
		atab_ptr += strspn(atab_ptr," \t\n");
		atab_ptr += strlen(ALIAS);
		atab_ptr += strspn(atab_ptr," \t");
		buf = strpbrk(atab_ptr," \t\n");
		if (*buf == '\n' || *(buf + strspn(buf," \t")) == '\n') {
			atab_ptr = buf + strspn(buf," \t\n");
			inval_cnt++;
			continue;
		}
		*buf++ = '\0';
		altbl->alname = atab_ptr;
		buf += strspn(buf," \t");
		atab_ptr = strpbrk(buf,"\n");
		*atab_ptr++ = '\0';
		altbl->alist = buf;
		altbl++;
	}
	aliases -= inval_cnt;
}

static void
creatlist(clients)
char	*clients[];
{

	register int index = 0;
	int	 old_pos;
	char	 *clname;


	if ((clist = (char **)malloc(NUM_MACHS * sizeof(char *))) == NULL) {
		Fprintf(stderr,"%s: cannot allocate memory for client list\n",cmd);
		exit(1);
	}

	/*
	 *	If the alias table exists, replace names defined 
	 *	as aliases, or else assume the names to be
	 *	valid client names and form a client list.
	 */

	while ((clname = clients[index]) != NULL) {
		if (*clname == '\0' || verify(clname)) {
			clients[index++][0] = '\0';
			continue;
		}

		if (aflag && isalias(clname,altptr)) {
			old_pos = pos;
			search_atbl(clname,aliases);
			if (old_pos == pos)
				clients[index][0] = '\0';
		} else {
			if (!invalid(clname) && !in_clist(clname))
				add_client(clname);
			else
				clients[index][0] = '\0';
		}
		index++;
	}

	if (pos == (low_index + NUM_MACHS))
		alloc_mem();
	clist[pos] = NULL;
}

static int
isalias(name,altbl)
char	*name;
struct	altab	*altbl;
{

	register int i;

	for (i = 0; i < aliases; i++) {
		if (strcmp(name,altbl->alname) == SAME)
			return 1;
		altbl++;
	}
	return 0;
}

static void
search_atbl(name,index)
char	*name;
int	index;
{

	register int i, level;
	register char *list, *curr_pos;
	char	buffer[SZ_CLIENT + 1];
	struct	altab	*altbl;

	/*
	 *	Search the alias table to determine if "name" is an
	 *	alias. If it is an alias obtain its corresponding
	 *	list of names, and determine if they are aliases by
	 *	searching preceding entries in the alias table. This
	 *	is done to eliminate loops. When "name" is no longer
	 * 	an alias copy it into the client list.		
	 */

	altbl = altptr;
	level = 0;
	for (i = 0; i < index; i++) {
		if (strcmp(name,altbl->alname) == SAME) {
			list = altbl->alist;
			curr_pos = strpbrk(list," \t");
			while (curr_pos != NULL) {
				if (curr_pos-list <= SZ_CLIENT) {
					(void)strncpy(buffer,list,curr_pos-list);
					buffer[curr_pos-list] = '\0';
					search_atbl(buffer,level);
				} else {
					*curr_pos = '\0';
					Fprintf(stderr,"%s: client/alias name <%s> exceeds <%d> characters, ignored\n",cmd,list,SZ_CLIENT);
					*curr_pos = ' ';
				}
				list = curr_pos + strspn(curr_pos," \t");
				curr_pos = strpbrk(list," \t");
			}
			if (*list == '\0')
				return;

			if (strlen(list) <= (size_t)SZ_CLIENT) {
				(void)strcpy(buffer,list);
				search_atbl(buffer,level);
			} else
				Fprintf(stderr,"%s: client/alias name <%s> exceeds <%d> characters, ignored\n",cmd,list,SZ_CLIENT);
			return;
		} else {
			level++;
			altbl++;
		}
	}
	if (!invalid(name) && !in_clist(name))
		add_client(name);
	return;
}

static int
in_clist(name)
char	*name;
{

 	register int i;

	for (i = 0; i < pos; i++) {
		if (strcmp(name,clist[i]) == SAME)
			return 1;
	}
	return 0;
}

static void
add_client(name)
char	*name;
{

	if (pos == (low_index + NUM_MACHS))
		alloc_mem();
	if ((clist[pos] = malloc((unsigned)strlen(name)+1)) == NULL ) {
		Fprintf(stderr,"%s: cannot allocate memory for client list\n",cmd);
		exit(1);
	}
	(void)strcpy(clist[pos++],name);
}

static int
verify(name)
char	*name;
{

	register char *chr;

	if (strlen(name) > (size_t)SZ_CLIENT) {
		Fprintf(stderr,"%s: client/alias name <%s> exceeds <%d> characters, ignored\n",cmd,name,SZ_CLIENT);
		return 1;
	}

	chr = name;
	while (*chr != '\0') {
		if (!isprint(*chr)) {
			Fprintf(stderr,"%s: client/alias name <%s> contains non-printable characters, ignored\n",cmd,name);
			return 1;
		}
		chr++;
	}
	return 0;
}

static void
alloc_mem()
{

	if ((clist = (char **)realloc((char *)clist,(NUM_MACHS + pos) * sizeof (char *))) == NULL) {
		Fprintf(stderr,"%s: cannot allocate memory for client list\n",cmd);
		exit(1);
	}
	low_index = pos;
}

static int
invalid(name)
char	*name;
{

	char	*mach;
	char	*domain;
	int	qname = 0, dname = 0;

	if (name[strlen(name)-1] == SEPARATOR)
		dname = 1;

	if (*(domain = dompart(name)) != '\0') {
		qname = 1;
		if (strlen(domain) > (size_t)SZ_DELEMENT) {
			Fprintf(stderr,"%s: domain name %s<%s> exceeds <%d> characters, ignored\n",cmd,dname ? "":"in ",name,SZ_DELEMENT);
			return 1;
		}

		if (v_dname(domain) != 0) {
			Fprintf(stderr,"%s: domain name %s<%s> contains invalid characters, ignored\n",cmd,dname ? "":"in ",name);
			return 1;
		}
	}

	if (*(mach = namepart(name)) != '\0') {
		if (strlen(mach) > (size_t)SZ_MACH) {
			Fprintf(stderr,"%s: nodename %s<%s> exceeds <%d> characters, ignored\n",cmd,qname ? "in ":"",name,SZ_MACH);
			return 1;
		}

		if (v_uname(mach) != 0) {
			Fprintf(stderr,"%s: nodename %s<%s> contains invalid characters, ignored\n",cmd,qname ? "in ":"",name);
			return 1;
		}
	}
	return 0;
}

static void
update_entry(res,path,rflag,desc,clients)
short	rflag;
char	*res, *path, *desc;
char	*clients[];
{

	register FILE  *fp, *fp1;
	int	index = 0;
	char	advbuf[BUFSIZE];
	register int firstclient=1;

	if ((fp = fopen(SHAREFILE, "r")) == NULL) {
		add_entry(res,path,rflag,desc,clients);
		return;
	}

	/*	
	 *	Update the client and/or description
	 *	fields of an existing entry.
	 */

	(void)stat(SHAREFILE,&stbuf);
	if ((fp1 = fopen(TEMPSHARE, "w")) == NULL) {
		Fprintf(stderr,"%s: cannot create temporary advertise file <%s>\n",cmd,TEMPSHARE);
		exit(1);
	}

	while (fgets(advbuf,BUFSIZE,fp)) {
		get_data(advbuf);
		if (!strcmp(res,fieldv[1]) &&
		    !strcmp(fieldv[2], "rfs")) {
			Fprintf(fp1,"%s %s rfs %2.2s",
				fieldv[0], res, fieldv[3]);

			if (clients[index] != NULL)
				while (clients[index] != NULL) {
					if (clients[index][0] != '\0') {
						Fprintf(fp1,"%c%s",
						   (firstclient) ? '=':':',
						   clients[index]);
						firstclient = 0;
					}
					index++;
				}	

			if (!dflag)
				Fprintf(fp1," %s\n", fieldv[4]);
			else
				/* ala rfs.cmds:share/share.c add_entry() */
				Fprintf(fp1, " %s\n", desc);
		} else 
			Fprintf(fp1,"%s %s %s %s %s\n",fieldv[0],fieldv[1],fieldv[2],fieldv[3],fieldv[4]);
	}
	(void)fclose(fp);
	(void)fclose(fp1);
	(void)rename(TEMPSHARE, SHAREFILE);
	(void)chmod(SHAREFILE,MASK);
	(void)chown(SHAREFILE,stbuf.st_uid,stbuf.st_gid);
}

static void
add_entry(res,path,rflag,desc,clients)
short	rflag;
char	*res, *path, *desc;
char	*clients[];
{
	register FILE	*fp;
	int	index = 0;
	register int firstclient = 1;


	if ((fp = fopen(SHAREFILE, "a")) == NULL) {
		Fprintf(stderr,"%s: cannot open <%s>\n",cmd,SHAREFILE);
		exit(1);
	}
	
	Fprintf(fp,"%s %s rfs %s",path, res, shareflg[rflag]);

	/* modify "ro" or "rw" option with an '=' followed
	   by a client list */

	if (clients[index])
		while (clients[index] != NULL) {
			if (clients[index][0] != '\0') {
				Fprintf(fp,"%c%s", (firstclient)?'=':':',
					clients[index]);
				firstclient = 0;
			}
			index++;
		}
	Fprintf(fp, " %s\n", desc); /* ala rfs.cmds:share/share.c add_entry() */
	(void)fclose(fp);
}

static void
get_data(s)
	char	*s;
{
	int	i;
	char	*fp;
	char	*fsep	= " \t\n";	
	char	*eol	= "\n";	
	char	*empty	= "";

	/*
 	 *	This function parses an advertise entry from 
 	 *	/etc/dfs/sharetab and sets the pointers appropriately.
	 *	fieldv[0] :  pathname
	 *	fieldv[1] :  resource
	 *	fieldv[2] :  fstype
	 *	fieldv[3] :  options
	 *	fieldv[4] :  description
 	 */

	/* initialize with defaults */ 
	for( i = 0; i < MAXFIELD; i++)
		fieldv[i] = empty;

	/* parse line into fields
	 * note that the last field, description, covers rest of the line
	 */
	for(	i = 0,		fp  = strtok(s, fsep);
		i < MAXFIELD && fp != NULL;
		i++,		fp  = strtok(NULL, i == MAXFIELD-1 ? eol : fsep)
	){
		fieldv[i] = fp;
	}
}

static int
ns_adv(req_type,path,res,roflg,desc,namelist)
short	req_type, roflg;
char	*path, *res, *desc;
char	**namelist;
{

	struct	nssend	send;
	struct	nssend	*ns_getblock();

	/*	
	 *	Initialize structure with information to be sent
	 *	to the name server.
	 */

	send.ns_code = req_type;
	send.ns_type = 1;
	send.ns_flag = roflg;
	send.ns_name = res;
	send.ns_path = path;
	send.ns_desc = desc;
	send.ns_mach = namelist;

	/*
	 *	Send the structure using the name server function
	 *	ns_getblock().
	 */

	if (ns_getblock(&send) == NULL)
		return FAILURE;
	
	return SUCCESS;
} 

static void
rpterr(res,dir)
char	*res;
char	*dir;
{

	switch(errno) {
	case EPERM:
		Fprintf(stderr,"%s: must be privileged\n",cmd);
		break;
	case ENOENT:
		Fprintf(stderr,"%s: <%s> no such file or directory\n",cmd,dir);
		break;
	case ENONET:
		Fprintf(stderr,"%s: machine not on the network\n",cmd);
		break;
	case ENOTDIR:
		Fprintf(stderr,"%s: <%s> not a directory\n",cmd,dir);
		break;
	case EREMOTE:
		Fprintf(stderr,"%s: <%s> is remote\n",cmd,dir);
		break;
	case EADV:
		Fprintf(stderr,"%s: <%s> already advertised\n",cmd,dir);
		break;
	case EROFS:
		Fprintf(stderr,"%s: <%s> write-protected\n",cmd,dir);
		break;
	case EINTR:
		Fprintf(stderr,"%s: system call interrupted\n",cmd);
		break;
	case EBUSY:
		Fprintf(stderr,"%s: resource <%s> currently advertises a different directory\n",cmd,res);
		break;
	case EEXIST:
		Fprintf(stderr,"%s: re-advertise error: <%s> was originally advertised under\n     a different resource name\n",cmd,dir);
		break;
	case ENOSPC:
		Fprintf(stderr,"%s: advertise table overflow\n",cmd);
		break;
	case EINVAL:
		Fprintf(stderr,"%s: invalid resource name\n",cmd);
		break;
	case EFAULT:
		Fprintf(stderr,"%s: bad user address\n",cmd);
		break;
	case ENOMEM:
		Fprintf(stderr,"%s: not enough memory\n",cmd);
		break;
	case ENODEV:
		Fprintf(stderr,"%s: <%s> not advertised\n",cmd,res);
		break;
	case ENOPKG:
		Fprintf(stderr,"%s: RFS package not installed\n",cmd);
		break;
	case ESRMNT:
		Fprintf(stderr,"%s: re-advertise error: a client that is not in the specified\n     client list currently has <%s> mounted\n",cmd,res);
		break;
	case EACCES:
		Fprintf(stderr,"%s: re-advertise error: resource <%s> originally advertised\n     with different permissions\n",cmd,res);
		break;
	default:
		Fprintf(stderr,"%s: errno <%d>, cannot advertise <%s>\n",cmd,errno,res);
		break;
	}
}
