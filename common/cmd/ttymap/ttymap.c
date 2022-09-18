/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * ttymap.c - create /etc/ttymap for ttyname(3), so it can do
 * 	fast lookups of /dev pathnames from dev and rdev numbers.
 */

#ident	"@(#)ttymap:ttymap.c	1.1.2.3"
#ident  "$Header: ttymap.c 1.2 91/06/27 $"

#include <stdio.h>
#include <sys/types.h>
/* #include <priv.h> */
#include <mac.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ftw.h>
#include <ttymap.h>
#include <pfmt.h>
#include <locale.h>
#ifdef DEBUG
#include <assert.h>
#define unlink
#else
#define assert(x)
#endif

struct dev_list *froot = NULL;
struct dirt_list *droot = NULL;

const struct entry *srch_dirs;

#define NFD	17

/* ARGSUSED */
main( argc, argv ) 
int argc;
char *argv[];
{
	time_t tstamp;
/*	time_t time(); */
	int insert();
	const struct entry *get_pri_dirs();

	/* Initialize locale, message label, message catalog. */
	setlocale(LC_ALL, "");
	setlabel("UX:ttymap");
	setcat("uxcore");

	if( argc != 1 ) {
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":722:usage: ttymap\n");
		exit( 1 );
	}
/*
 *	get priority directories and flags from /etc/ttysrch 
 */	
	srch_dirs = get_pri_dirs();
/*
 *	time stamp for map file - ttyname assumes any directory
 *	that is newer than now needs to be searched.
 */
	tstamp = time( NULL );
/*
 *	call ftw to build list of /dev/entries
 */
	if( ftw( "/dev", insert, NFD ) ) {
		pfmt(stderr, MM_ERROR, ":723:search of /dev failed\n");
		exit( 1 );
	}
/*
 *	create the map file
 */
	ttymap( tstamp );
	exit( 0 );

}
/*
 * insert - add entries to list of devices / directories
 */
int
insert( path, stp, flg )
char *path;
struct stat *stp;
int flg;
{
	struct rdev_list *rdevp, *rp;
	struct dev_list *devp, *dp;
	struct path_list *pathp, *pp;		
	struct dirt_list *dirtp, *drp;
	int j;
	char *chk_malloc();
	struct entry *sp;

	for( sp = (struct entry *)srch_dirs ; sp->name; sp++ )
		if( !strncmp( sp->name, path, sp->len ) )
			if( sp->flags & IGNORE_DIR )
				return( 0 );
			else
				break;
	switch( flg ) {

	case FTW_F:
/*
 *		found a file, add it to the list
 *
 *		the list is constructed as: 
 *
 *		dev_list:
 *		   dev
 *		   dev ----> rdev_list:
 *		    .		rdev
 *		    .		rdev ----> path_list:
 *		    .		  .		path
 *		   null		  .		path
 *				  .		  .
 *				null		  .
 *						  .
 *						null
 *
 *		the dev and rdev lists are sorted numerically.
 *		paths are sorted by /etc/ttysrch and then alphabetically.
 *		
 *		the goal is to achieve very fast access to path names
 *		given a particular combination of dev and rdev.
 */
		for( devp = (struct dev_list *)&froot ; devp->nxt ; devp=devp->nxt )
			if( devp->nxt->st_dev >= stp->st_dev )
				break;
/*
 *		check to see if we've got a dev entry for it
 */
		if( !devp->nxt || (devp->nxt->st_dev != stp->st_dev) ) {
/*
 *			didn't find it, add a new one
 */
			dp = (struct dev_list *)
			  chk_malloc( sizeof( struct dev_list ) );
			dp->st_dev = stp->st_dev;
			dp->rdev_list = NULL;
			dp->nxt = devp->nxt;
			devp->nxt = dp;
		}
/*
 *		got a dev entry, now search rdev_list
 */
		for(rdevp = (struct rdev_list *)&devp->nxt->rdev_list; rdevp->nxt; rdevp=rdevp->nxt)
			if( rdevp->nxt->st_rdev >= stp->st_rdev )
				break;
/*
 *		check to see if we've got a rdev entry for it
 */
		if( !rdevp->nxt || (rdevp->nxt->st_rdev != stp->st_rdev) ) {
/*
 *			didn't find it, add a new one
 */
			rp = (struct rdev_list *)
			  chk_malloc( sizeof( struct rdev_list ) );
			rp->st_rdev = stp->st_rdev;
			rp->nxt = rdevp->nxt;
			rp->path_list = NULL;
			rdevp->nxt = rp;
		}
/*	
 *		Is the path in the path_list
 */
		for(pathp= (struct path_list *)&rdevp->nxt->path_list; pathp->nxt; pathp=pathp->nxt)
			if( (j = pstrcmp( pathp->nxt->path, path )) >= 0 )
				break;
		if( !pathp->nxt || j ) {
/*
 *			Didn't find it, add a new path
 */
			pp = (struct path_list *)
			  chk_malloc( sizeof( struct path_list ) );
			pp->path = chk_malloc( strlen( path ) + 1 );
			strcpy( pp->path, path );
			pp->nxt = pathp->nxt;
			pathp->nxt = pp;
		}
		break;
	case FTW_D:
/*
 *		found a directory, add it to the directory list
 */

		for( dirtp = (struct dirt_list *)&droot ; dirtp->nxt ; dirtp=dirtp->nxt )
			if( pstrcmp( dirtp->nxt->path, path ) >= 0 )
				break;
		drp = (struct dirt_list *)
		  chk_malloc( sizeof( struct dirt_list ) );
		drp->path = chk_malloc( strlen( path ) + 1 );
		strcpy( drp->path, path );
		drp->nxt = dirtp->nxt;
		dirtp->nxt = drp;
		break;

	default:
/*
 *		We got a directory we can't read or a file we can't stat.
 *		Being root, this shouldn't happen.
 */
		pfmt(stderr, MM_ERROR, ":724:cannot access \"%s\"\n", path);
		exit( 1 );

	}
	return( 0 );
}
/*
 *	compare names based on the order they appear in /etc/ttysrch.
 *	If not found in /etc/ttysrch, use strcmp order.
 */
pstrcmp( p1, p2 )
char *p1, *p2;
{
	const struct entry *dp;
	int t0, t1, t2;
	int d1, d2;

	assert( p1 && p2 );

	if( !(t0 = strcmp( p1, p2 )) )
		return( 0 );
	d1 = depth( p1 );
	d2 = depth( p2 );
/*
 *	search the /etc/ttysrch directories for matches
 */
	for( dp = srch_dirs ; dp->name ; dp++ ) {
		t1 = strncmp( p1, dp->name, dp->len );
		t2 = strncmp( p2, dp->name, dp->len );
/*
 *		if no matches, continue
 */
		if( t1 && t2 )
			continue;
/*
 *		since "/dev" may appear in /etc/ttysrch, don't match
 *		"/dev" for entries with depth > 1.
 */
		if( !strcmp( dp->name, "/dev" ) ) {
			if( d1 != 1 && d2 != 1 )
				continue;
			if( d1 == 1 && d2 != 1 )
				return( -1 );
			if( d1 != 1 && d2 == 1 )
				return( 1 );				
			break;
		}
/*
 *		if one entry matched and the other didn't, return
 */
		if( !t1 && t2 )
			return( -1 );
		if( t1 && !t2 )
			return( 1 );
		break;
	}
/*
 *	either they both matched, or neither of them matched,
 *	return the one with the smallest depth
 */
	if( d1 < d2 )
		return( -1 );
	if( d1 > d2 )
		return( 1 );
/*
 *	use strcmp order
 */
	return( t0 );
}
depth( path )
char *path;
{
	int depth = 0;

	if( !*path )
		return( 0 );

	while( *path == '/' )			/* skip leading /'s	*/
		path++;

	while( *path ) {
		if( *path++ == '/' ) {
			depth++;
			while( *path == '/' )	/* drop multiple /'s	*/
				path++;
			if( !*path )		/* drop trailing /'s	*/
				depth--;
		}

	}		
	return( depth );
}

char *
chk_malloc( size )
uint size;
{
	char *p;
	char *malloc();
	if( !(p = malloc( size )) ){
		pfmt(stderr, MM_ERROR, ":725:out of memory\n");
 		exit( 1 );
	}
	return( p );
}		
/*
 *	ttymap - create the map file
 */
ttymap( timestamp ) 
time_t timestamp;
{
	struct dev_list *dp;
	struct rdev_list *rp;
	struct path_list *pp;
	struct dirt_list *tp;

	struct tty_map *ttymap;
	struct dent *fsid, *fsp;
	struct rent *majmin, *mmp;
	struct dirmap *dirmap, *drp;
	char *names, *nmp;
	size_t rent_off, name_off;
	struct stat st_buf;
	char *dirname();

	int fd;
/*
 *	allocate header
 */
	ttymap = (struct tty_map *)chk_malloc( sizeof( struct tty_map ) );

/*
 * 	size everything up
 */
	ttymap->dent_sz = 0; 
	ttymap->rent_sz = 0;
	ttymap->name_sz = 0;
	for( dp = froot ; dp ; dp = dp->nxt ) {
		ttymap->dent_sz++; 		
		for ( rp = dp->rdev_list; rp; rp = rp->nxt ) {
			ttymap->rent_sz++; 		
			for( pp = rp->path_list ; pp ; pp = pp->nxt ) {
				ttymap->name_sz += strlen( pp->path ) + 1;
			}
		}
	}
	ttymap->dirmap_sz = 0;
	for( tp = droot; tp ; tp = tp->nxt ) {
		ttymap->dirmap_sz++;
		ttymap->name_sz += strlen( tp->path ) + 1;
	}
/*
 *	allocate tables
 */	
	fsid = (struct dent *)
	  chk_malloc( ttymap->dent_sz * sizeof( struct dent ) );
	majmin = (struct rent *)
	  chk_malloc( ttymap->rent_sz * sizeof( struct rent ) );
	dirmap = (struct dirmap *)
	  chk_malloc( ttymap->dirmap_sz * sizeof( struct dirmap ) );
	names = chk_malloc( ttymap->name_sz );
/*
 *	fill tables
 */
	fsp = fsid;
	mmp = majmin;
	nmp = names;
	rent_off = 0;
	name_off = 0;	
	for( dp = froot ; dp ; dp = dp->nxt, fsp++ ) {
		fsp->dev = dp->st_dev;
		fsp->nrent = 0;
		for ( rp = dp->rdev_list; rp; rp = rp->nxt, mmp++ ) {
			fsp->nrent++;
			mmp->rdev = rp->st_rdev;
			mmp->nm_offset = name_off;
			mmp->nm_size = 0;
			for( pp = rp->path_list ; pp ; pp = pp->nxt ) {
				strcpy( nmp, pp->path );
				nmp += strlen( pp->path ) + 1;
				mmp->nm_size += strlen( pp->path ) + 1;
			}
			name_off += mmp->nm_size;
		}
		fsp->rent_offset = rent_off;
		rent_off += fsp->nrent * sizeof( struct rent );
	}
/*
 *	fill dirmap
 */
	drp = dirmap;
	for( tp = droot; tp ; tp = tp->nxt ) {
		drp->len = strlen( tp->path );
		drp->depth = depth( tp->path );
		drp->flags = getflags( tp->path );
		drp->nm_offset = name_off;
		name_off += drp->len + 1;
		strcpy( nmp, tp->path );
		nmp += drp->len + 1;
		drp++;
	}
/*
 *	set offsets in header
 */
	ttymap->dent_tbl = sizeof( struct tty_map );	
	ttymap->rent_tbl = ttymap->dent_tbl 
	  + ttymap->dent_sz * sizeof( struct dent );
	ttymap->dirmap_tbl = ttymap->rent_tbl 
	  + ttymap->rent_sz * sizeof( struct rent );
	ttymap->name_tbl = ttymap->dirmap_tbl 
	  + ttymap->dirmap_sz * sizeof( struct dirmap );
	ttymap->date = timestamp;

/*
 *	write the map file
 */
	(void) unlink( MAP );
	if( ( fd = open( MAP, O_CREAT | O_TRUNC | O_WRONLY, 0644 )) < 0 ) {
		pfmt(stderr, MM_ERROR, ":726:cannot open \"%s\"\n", MAP);
		exit( 1 );
	}

	if( write( fd, ttymap, sizeof( struct tty_map ) ) !=
	  sizeof( struct tty_map ) ) {
		pfmt(stderr, MM_ERROR, ":727:cannot write \"%s\"\n", MAP);
		(void) close( fd );
		(void) unlink( MAP );
		exit( 1 );
	}

	if( write( fd, fsid, ttymap->dent_sz * sizeof( struct dent ) ) !=
	  ttymap->dent_sz * sizeof( struct dent ) ) {
		pfmt(stderr, MM_ERROR, ":727:cannot write \"%s\"\n", MAP);
		(void) close( fd );
		(void) unlink( MAP );
		exit( 1 );
	}

	if( write( fd, majmin, ttymap->rent_sz * sizeof( struct rent ) ) !=
	  ttymap->rent_sz * sizeof( struct rent ) ) {
		pfmt(stderr, MM_ERROR, ":727:cannot write \"%s\"\n", MAP);
		(void) close( fd );
		(void) unlink( MAP );
		exit( 1 );
	}

	if( write( fd, dirmap, ttymap->dirmap_sz * sizeof( struct dirmap ) ) !=
	  ttymap->dirmap_sz * sizeof( struct dirmap ) ) {
		pfmt(stderr, MM_ERROR, ":727:cannot write \"%s\"\n", MAP);
		(void) close( fd );
		(void) unlink( MAP );
		exit( 1 );
	}

	if( write( fd, names, ttymap->name_sz ) != ttymap->name_sz ) {
		pfmt(stderr, MM_ERROR, ":727:cannot write \"%s\"\n", MAP);
		(void) close( fd );
		(void) unlink( MAP );
		exit( 1 );
	}

	if( write( fd, "\0\0\0\0", 4) != 4) {
		pfmt(stderr, MM_ERROR, ":727:cannot write \"%s\"\n", MAP);
		(void) close( fd );
		(void) unlink( MAP );
		exit( 1 );
	}

	(void) close( fd );

	if( stat( dirname( MAP ), &st_buf ) < 0 ) {
		pfmt(stderr, MM_ERROR, ":728:cannot stat \"%s\"\n", dirname(MAP));
		exit( 1 );
	}

	(void) lvlfile( MAP, MAC_SET, &st_buf.st_level );

	if( chmod( MAP, 0644 ) < 0 ) {
		pfmt(stderr, MM_ERROR, ":729:cannot chmod \"%s\"\n", MAP);
		exit( 1 );
	}

	if( chown( MAP, st_buf.st_uid, st_buf.st_gid ) < 0 ) {
		pfmt(stderr, MM_ERROR, ":730:cannot chown \"%s\"\n", MAP);
		exit( 1 );
	}
}
/*
 * getflags - search for longest priority name that fits name
 *	if found, use its flags, otherwise, return MATCH_ALL
 */
getflags( name )
char *name;
{
	const struct entry *dlong, *dp;
	char *p;

	dlong = NULL;
	for( dp = srch_dirs; dp->name; dp++ ) {
		if( !strncmp( name, dp->name, dp->len ) )
			if( !dlong || dp->len > dlong->len )
				dlong = dp;
	}
	if( !dlong || !dlong->flags )
		return( MATCH_ALL );
/*
 *	Don't use /dev flags beyond a path depth of 1
 */
	if( !strcmp( dlong->name, DEV ) ) {
		for( p=name+DEVLEN; *p; p++ )
			if ( *p == '/' ) 
				return( MATCH_ALL );
	}
	return( dlong->flags );
}
/*
 * note - this function was borrowed from SVR4.0 libc/port/gen/ttyname.c
 *
 * get_pri_dirs() - returns a pointer to an array of strings, where each 
 * string is a priority directory name.  The end of the array is marked 
 * by a NULL pointer.  The priority directories' names are obtained from 
 * the file /etc/ttysrch if it exists and is readable, or if not, a default
 * hard-coded list of directories.
 */

#define	START_STATE	1
#define	COMMENT_STATE	2
#define	DIRNAME_STATE	3
#define FLAG_STATE	4
#define CHECK_STATE	5

#define COMMENT_CHAR	'#'
#define EOLN_CHAR	'\n'

static char *dir_buf;		/* directory buffer for ttysrch body */
static struct entry *dir_vec;	/* directory vector for ttysrch ptrs */

const struct entry *
get_pri_dirs()
{
	int fd, size, state;
	size_t sz;
	struct stat sb;
	register char *buf, *ebuf;
	register struct entry *vec;
	char *malloc();


	/* if no /etc/ttysrch, use defaults */

	if  ( (fd = open(TTYSRCH, 0)) < 0 || stat(TTYSRCH,&sb) < 0 )
		return(def_srch_dirs);
	sz = sb.st_size;
	if ((dir_buf = malloc(sz + 1)) == NULL
		|| (size = read(fd, dir_buf, sz)) < 0) {
		(void)close(fd);
		return(def_srch_dirs);
	}
	(void)close(fd);

	/*	ensure newline termination for buffer.  Add an extra
	 *	entry to dir_vec for null terminator
	 */

	ebuf = &dir_buf[size];
	*ebuf++ = '\n';
	for (sz = 1, buf = dir_buf; buf < ebuf; ++buf)
		if (*buf == '\n')
			++sz;
	if ((dir_vec = (struct entry *)malloc(sz * sizeof(*dir_vec))) == NULL)
		return(def_srch_dirs);

	state = START_STATE;
	for (buf = dir_buf, vec = dir_vec; buf < ebuf; ++buf) {
		switch (state) {

		case START_STATE:
			if (*buf == COMMENT_CHAR) {
				state = COMMENT_STATE;
				break;
			}
			if (!isspace(*buf))	/* skip leading white space */
				state = DIRNAME_STATE;
				vec->name = buf;
				vec->flags = 0;
			break;

		case COMMENT_STATE:
			if (*buf == EOLN_CHAR)
				state = START_STATE;
			break;

		case DIRNAME_STATE:
			if ( *buf == EOLN_CHAR ) {
				state = CHECK_STATE;
				*buf = '\0';
			} else if (isspace(*buf)) {	/* skip trailing white space */
				state = FLAG_STATE;
				*buf = '\0';
			}
			break;

		case FLAG_STATE:
			switch(*buf) {
				case 'M':
					vec->flags |= MATCH_MM;
					break;
				case 'F':
					vec->flags |= MATCH_FS;
					break;
				case 'I':
					vec->flags |= MATCH_INO;
					break;
				case 'X':
					vec->flags |= IGNORE_DIR;
					break;
				case EOLN_CHAR:
					state = CHECK_STATE;
					break;
			}
			break;

		case CHECK_STATE:
			if (strncmp(vec->name, DEV, strlen(DEV)) != 0
			  || (vec->flags & IGNORE_DIR 
			      && vec->flags & MATCH_ALL) ) {
				int tfd = open("/dev/console", O_WRONLY);
				if (tfd >= 0) {
					write(tfd, MSG_1, strlen(MSG_1));
					write(tfd, vec->name, strlen(vec->name));
					write(tfd, MSG_2, strlen(MSG_2));
					close(tfd);
				}
			} else {
				char *slash;
				vec->len = strlen( vec->name );
				slash = vec->name + vec->len - 1;
				while ( *slash == '/' ) {
					*slash-- = '\0';
					vec->len--;
				}
				if ( vec->flags == 0 )
					vec->flags = MATCH_ALL;
				vec++;
			}
			state = START_STATE;
			/*
			 * This state does not consume a character, so
			 * reposition the pointer.
			 */
			buf--;
			break;

		}
	}
	vec->name = NULL;
	return((const struct entry *)dir_vec);
}
char * 
dirname( path )
char *path;
{
	char *buf;
	char *p;
	int len;

	len = strlen( path );

	buf = chk_malloc( len + 1 );

	strcpy( buf, path );

	p = buf + len; 
	while( p > buf && *p != '/')
		p--;
	if( *p != '/' )
		return( "." );
	if( p == buf )
		return( "/" );
	*p = '\0';
	return( buf );
}

