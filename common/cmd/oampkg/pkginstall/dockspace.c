/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/dockspace.c	1.7.13.6"
#ident  "$Header: $"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/param.h>
#include <limits.h>
#include <pkgstrct.h>

extern struct cfent
		**eptlist;
extern char	*basedir;
extern char	**class;

#define PBLK	512	/* 512 byte "physical" block */
#define LSIZE	256
#define NFSYS 10
#define LIM_BFREE	150
#define LIM_FFREE	25
#define WRN_STATVFS	"WARNING: unable to stat filesystem mounted on <%s>"
#define INDIRECT	10	/*Number of logical blocks before indirection*/

extern char	*strrchr(),
		*malloc();
extern void	progerr(),
		logerr(),
		mappath(),
		basepath();

static int	nfsys = 0;
static int	rootfsys = 0;
static void	warn();
static int	fsys(), fsyscmp(), readspace(), readmap();
static long	nblk();

static struct tbl {
	char	name[PATH_MAX];
	u_long	bsize;	/* fundamental file system block size */
	u_long	bfree;	/* total # of free blocks */
	u_long	bused;	/* total # of free blocks */
	u_long	ffree;	/* total # of free file nodes */
	u_long	fused;	/* total # of free file nodes */
} *table;

int
dockspace(spacefile)
char	*spacefile;
{
	struct statvfs statvfsbuf;
	FILE	*pp;
	char	*path, line[LSIZE];
	long	bfree, ffree, frsize;
	int	i, error, repeat;

	if((pp=popen("/sbin/mount", "r")) == NULL) {
		progerr("unable to create pipe to /sbin/mount");
		return(-1);
	}

	if((table = (struct tbl *)malloc(NFSYS * sizeof(struct tbl))) == (struct tbl *)NULL) {
		progerr("unable to malloc space\n");
		return(-1);
	}
	nfsys = error = 0;
	repeat = 1;
	while(fgets(line, LSIZE, pp)) {
		path = strtok(line, " \t\n");
		if(!strcmp(path, "/"))
			rootfsys = nfsys;
		if(statvfs(path, &statvfsbuf)) {
			logerr(WRN_STATVFS, path);
			error++;
			continue;
		}
		if(nfsys == (repeat * NFSYS)) {
			repeat++;
			if((table = (struct tbl *)realloc(table, NFSYS * repeat * sizeof(struct tbl))) == (struct tbl *)NULL) {
				progerr("unable to malloc space\n");
				return(-1);
			}
		}	
		(void) strcpy(table[nfsys].name, path);
		/* statvfs returns number of fragment size blocks
		 * so will change this to number of 512 byte blocks
		 */
		table[nfsys].bfree = statvfsbuf.f_bavail;
		frsize = statvfsbuf.f_frsize;
		table[nfsys].bfree *= (((frsize - 1) / PBLK) + 1);
		table[nfsys].ffree = statvfsbuf.f_favail;
		table[nfsys].bsize = statvfsbuf.f_bsize;
		table[nfsys].bused = (u_long) 0;
		table[nfsys].fused = (u_long) 0;
		nfsys++;
	}
	if(pclose(pp)) {
		progerr("unable to obtain mounted filesystems from /sbin/mount");
		return(-1);
	}

	(void) readmap();
	if((spacefile) && (readspace(spacefile) < 0))
		return(-1);

	for(i=0; i < nfsys; ++i) {
		if((!table[i].fused) && (!table[i].bused))
			continue; /* not used by us */
		bfree = (long) table[i].bfree - (long) table[i].bused;
		ffree = (long) table[i].ffree - (long) table[i].fused;
		if(bfree < LIM_BFREE) {
			warn("blocks", table[i].name, 
				table[i].bused + LIM_BFREE, table[i].bfree);
			error++;
		}
		if(ffree < LIM_FFREE) {
			warn("file nodes", table[i].name, 
				table[i].fused + LIM_FFREE, table[i].ffree);
			error++;
		}
	}
	return(error);
}

static void
warn(type, name, need, avail)
char *type, *name;
ulong need, avail;
{
	extern	int logmode;
	int	reset = 0;

	if(logmode) {
		reset++;
		logmode = 0;
	}

	logerr("WARNING:");
	logerr("%lu free %s are needed in the %s filesystem,",
		 need, type, name);
	logerr("but only %lu %s are currently available.", avail, type);

	if(reset)
		logmode++;
}

static int
fsys(path)
char *path;
{
	register int i;
	int n, level, found;

	found = rootfsys;
	level = 0;
	for(i=0; i < nfsys; i++) {
		if(i == rootfsys)
			continue;
		if(n = fsyscmp(table[i].name, path)) {
			if(n > level) {
				level = n;
				found = i;
			}
		}
	}
	return(found);
}

static int
fsyscmp(fsystem, path)
char *fsystem, *path;
{
	int level;

	level = 0;
	while(*fsystem) {
		if(*fsystem != *path)
			break;
		if(*fsystem++ == '/')
			level++;
		path++;
	}
	if((*fsystem != '\0') || (*path && (*path != '/')))
		return(0);
	return(level);
}

static int
readmap()
{
	struct cfent *ept;
	struct stat statbuf;
	long	blk;
	int	i, n;

	for(i=0; (ept = eptlist[i]) != NULL; i++) {
		if(ept->ftype == 'i')
			continue;
		n = fsys(ept->path);
		if(stat(ept->path, &statbuf)) {
			/* path cannot be accessed */
			table[n].fused++;
			if(strchr("dxlspcb", ept->ftype))
				blk = nblk((long)table[n].bsize,table[n].bsize);
			else if((ept->ftype != 'e') && 
			(ept->cinfo.size != BADCONT))
				blk = nblk(ept->cinfo.size, table[n].bsize);
			else
				blk = 0;
		} else {
			/* path already exists */
			if(strchr("dxlspcb", ept->ftype))
				blk = 0;
			else if((ept->ftype != 'e') && 
			(ept->cinfo.size != BADCONT)) {
				blk = nblk(ept->cinfo.size, table[n].bsize);
				blk -= nblk(statbuf.st_size, table[n].bsize);
				/* negative blocks show room freed, but since
				 * order of installation is uncertain show
				 * 0 blocks usage 
				 */
				if(blk < 0)
					blk = 0;
			} else
				blk = 0;
		}
		table[n].bused += blk;
	}
	return(0);
}

static long 
nblk(size, bsize)
long	size;
ulong	bsize;
{
	long	tot, count, count1, d_indirect, t_indirect, ind;

	if(size == 0)
		return(1);

	/*Need to keep track of indirect blocks.
	 *However, for convenience, will assume an s5 type
	 *of structure, which should be approximately
	 *correct for most file system types
	 */
	ind=(bsize + 1)/sizeof(daddr_t);
	d_indirect=ind + INDIRECT; 	/*double indirection*/
	t_indirect=ind*(ind + 1) + INDIRECT;	/*triple indirection*/

	tot = ((size - 1) / bsize) + 1; 
	if (tot > t_indirect) {
		count1 = (tot - ind*ind - (INDIRECT + 1))/ind;
		count = count1 + count1/ind + ind + 3;
	}
	else if (tot > d_indirect) 
		count = (tot - (INDIRECT + 1))/ind + 2;
	     else if (tot > INDIRECT)
			count = 1;
	     	  else
			count = 0;	


	/*Accounting for the indirect blocks, the total becomes*/
	tot += count;

	/* calculate number of 512 byte blocks */
	tot *= (((bsize-1)/PBLK)+1);
	return(tot);
}

static int
readspace(spacefile)
char	*spacefile;
{
	FILE	*fp;
	char	*pt, path[256], line[LSIZE];
	long	blocks = 0, nodes = 0;
	int	n;


	if(spacefile == NULL)
		return(0);

	if((fp=fopen(spacefile, "r")) == NULL) {
		progerr("unable to open spacefile %s", spacefile);
		return(-1);
	}

	while(fgets(line, LSIZE, fp)) {
		for(pt=line; isspace(*pt);)
			pt++;
		if((*line == '#') || !*line)
			continue;

		(void) sscanf(line, "%s %ld %ld", path, &blocks, &nodes); 
		mappath(2, path);
		(void)basepath(path, basedir);

		n = fsys(path);
		table[n].bused += blocks;
		table[n].fused += nodes;
	}
	(void) fclose(fp);
	return(0);
}
