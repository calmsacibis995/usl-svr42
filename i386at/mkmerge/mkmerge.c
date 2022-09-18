/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mk:i386at/mkmerge/mkmerge.c	1.3"
#ident  "$Header: $"

/*	Options:
**	-c: clean (unlink or rm all files from merged tree)
**	-d: specify a directory
**	-h: use symbolis links to merge
**	-l: use `hard' links to merge trees
**	-m: non-linked copies of makefiles to merge trees
**	-n: show what would be merged, but don't merge
**	-u: update (for non-symbolic linked trees), only merge
**		the files that have different checksums
**	-v: verbose
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <regexpr.h>
#include <stdlib.h>
#ifdef SVR4
#include <libgen.h>
#else
extern char *basename();
extern int mkdirp();
extern char *strerror();
typedef int mode_t;
#define MAXPATHLEN 1024
#endif

#define SUCCESS	1
#define FAIL	0

extern unsigned int csum();

char *targetdir, *cmd, **dirs, **trees;
int	ndir,
	clean,
	use_link,
	show,
	verbose,
	error,
	makefile_copy,
	update,
	can_copy,
	use_symbolic;
char root[MAXPATHLEN];
FILE *dotf = NULL;

main(argc, argv)
int argc;
char **argv;
{
	extern int optind;
	extern char *optarg;
	int opt, i;
	struct stat statb;
	char pathname[MAXPATHLEN];
	char dot_tree[MAXPATHLEN];

	cmd = basename(argv[0]);

	while ((opt = getopt(argc, argv, "cd:hlmnuv")) != EOF){
		switch(opt){
		case 'c':
			++clean;
			break;
		case 'd': /* Specify a directory */
			if (ndir == 0){
				if ((dirs = (char **)malloc(sizeof(char *)))
					 			== NULL)
					out_of_mem();
			} else {
				if ((dirs = (char **)realloc(dirs,
					    sizeof(char *) * (ndir + 1)))
								== NULL)
					out_of_mem();
			}
			dirs[ndir++] = optarg;
			break;
		case 'h': /* Use symbolic links instead of hard links */
			use_symbolic++;
			break;
		case 'l':
			++use_link;
			break;
		case 'm':
			++makefile_copy;
			break;
		case 'n':
			++show;
			break;
		case 'u':
			++update;
			fprintf(stderr,
				"%s: `-%c' option not implemented, yet...ignored\n",
					cmd, opt);
			up_date();
			break;
		case 'v': /* Verbose mode */
			verbose++;
			break;
		default:
			usage();
		}
	}

	/*
	 * Verify the arguments
	 */
	if (optind >= argc - 1){
		fprintf(stderr, "%s: Must specify at least two trees\n", cmd);
		usage();
	}

	trees = argv + optind;

	for (i = 0; trees[i] ; ++i) {
		targetdir = trees[i];
	}
	--i;
	trees[i] = NULL;

	trees = argv + optind;

	if (clean)
		cleanup();

	/*
	 * Verify the existence of source trees
	 */
	for (i = 0 ; trees[i] ; i++){
		if (stat(trees[i], &statb) == -1){
			fprintf(stderr, "%s: Cannot access tree %s: %s\n",
				cmd, trees[i], strerror(errno));
			exit(1);
		}
		if ((statb.st_mode & S_IFMT) != S_IFDIR){
			fprintf(stderr, "%s: Not a directory: %s\n", cmd,
				trees[i]);
			exit(1);
		}
	}
	for (i = 0 ; i < ndir ; i++){
		int sdir = 0;
		int j;
		for (j = 0 ; trees[j] ; j++){
			sprintf(pathname, "%s/%s", trees[j], dirs[i]);
			if (stat(pathname, &statb) == -1)
				continue;
			if ((statb.st_mode & S_IFMT) != S_IFDIR){
				fprintf(stderr, "%s: Not a directory: %s\n",
					cmd, pathname);
				exit(1);
			}
			sdir++;
		}
		if (!sdir){
			fprintf(stderr,
				"%s: Directory does not exist in the trees: %s\n",
				cmd, dirs[i]);
			exit(1);
		}
	}

	/*
	 * Need the current directory for symbolic links
	 */
	if (use_symbolic){
		if (getcwd(root, sizeof root) == NULL){
			fprintf(stderr,
				"%s: Cannot determine current directory: %s\n",
				cmd, strerror(errno));
		}
	}

	/*
	 * Go for it
	 */
	for (i = 0 ; trees[i] ; i++){

		/*
		 * update local .tree file
		 */
		if (!show) {
		sprintf(dot_tree, "%s/.%s", targetdir, trees[i]);
		if ((access(targetdir, 00)) == -1) {
			if (mkdirp(targetdir, 0777) == -1){
				perror(targetdir);
				exit (1);
			}
		}
		if ((dotf = fopen(dot_tree, "w")) == NULL){
			fprintf(stderr, "%s: Cannot create %s: %s\n",
				cmd, dot_tree, strerror(errno));
			exit (1);
		}
		}

		if (ndir){
			int j;
			for (j = 0 ; j < ndir ; j++){
				char pathtarget[MAXPATHLEN];
				if (use_symbolic)
					sprintf(pathname, "%s/%s/%s", root,
						trees[i], dirs[j]);
				else
					sprintf(pathname, "%s/%s", trees[i],
						dirs[j]);
				(void)stat(pathname, &statb);
				sprintf(pathtarget, "%s/%s", targetdir, dirs[j]);
				if (!mkmerge(basename(trees[i]), pathname,
					pathtarget, statb.st_mode))
					exit(1);
			}
		} else {
			(void)stat(trees[i], &statb);
			if (use_symbolic){
				sprintf(pathname, "%s/%s", root, trees[i]);
				if (!mkmerge(basename(trees[i]), pathname,
					targetdir, statb.st_mode)){
					exit(1);
				}
			} else if (!mkmerge(basename(trees[i]), trees[i],
				targetdir, statb.st_mode)){
				exit(1);
			}
		}

		fclose(dotf);
	}
	exit(error ? 1 : 0);
}

out_of_mem()
{
	fprintf(stderr, "%s: Out of memory\n", cmd);
	exit(1);
}

usage()
{
	fprintf(stderr,
		"%s: Usage: %s [-chlnuv] [-d dir] tree1 tree2...target_tree\n",
		cmd, cmd);
	exit(2);
}

#define SYM_LINK 1
#define HARD_LINK 2
#define HARD_COPY 3

mkmerge(tree, source, dest, mode)
char *tree, *source, *dest;
mode_t mode;
{
	struct stat statb;
	int copy_mode = HARD_COPY;
	DIR *dirp;
	struct dirent *entryp;

	if ((dirp = opendir(source)) == NULL){
		return SUCCESS;
	}

	/*
	 *
	 * Create target directory
	 */
	if (!show) {
	if (stat(dest, &statb) == 0){
		if ((statb.st_mode & S_IFMT) != S_IFDIR){
			fprintf(stderr,
				"%s: Target %s exists and is not a directory\n",
				cmd, dest);
			closedir(dirp);
			return FAIL;
		}
	} else {
		if (mkdirp(dest, mode | (mode_t) S_IWUSR ) == -1){
			fprintf(stderr,
				"%s: Cannot create target directory %s: %s\n",
				cmd, dest, strerror(errno));
			closedir(dirp);
			return FAIL;
		}
		if (verbose)
			fprintf(stdout, "d %s\n", dest);
	}
	}

	/*
	 * Skip . and ..
	 */
	(void)readdir(dirp);
	(void)readdir(dirp);

	for (entryp = readdir(dirp) ; entryp ; entryp = readdir(dirp)){
		char pathname[MAXPATHLEN], pathtarget[MAXPATHLEN];
		sprintf(pathname, "%s/%s", source, entryp->d_name);
		sprintf(pathtarget, "%s/%s", dest, entryp->d_name);
		if (stat(pathname, &statb) == -1){
			fprintf(stderr, "%s: Cannot access %s: %s\n", cmd,
				pathname, strerror(errno));
			continue;
		}
		if ((statb.st_mode & S_IFMT) == S_IFDIR){
			if (!mkmerge(tree, pathname, pathtarget, statb.st_mode)){
				closedir(dirp);
				return FAIL;
			}
			continue;
		}
		if ((statb.st_mode & S_IFMT) != S_IFREG){
			fprintf(stderr, "%s: Not a regular file: %s\n", cmd,
				pathname);
			error++;
			continue;
		}
		if (show) {
			fprintf(stdout, "%s\n", pathname);
			continue;
		}
#ifdef NO_OVER_WRITE
		if ((access(pathtarget, 0)) == 0) {
			if (verbose)
				fprintf(stderr, "%s: file %s exists\n", cmd, pathtarget);
			continue;
		}
#else
		if (unlink(pathtarget) == -1 && errno != ENOENT){
			fprintf(stderr, "%s: Cannot remove existing %s: %s\n",
				cmd, pathtarget, strerror(errno));
				error++;
			continue;
		}
#endif	/* NO_OVER_WRITE */
#ifdef SMS_VERSION
		/*
		 * .smsmark file need special processing
		 */
		if (strcmp(entryp->d_name, ".smsmark") == 0){
			if (!smsmark(tree, pathname, pathtarget)){
				closedir(dirp);
				return FAIL;
			}
			if (verbose)
				fprintf(stdout, "f %s\n", pathtarget);
			continue;
		}
#endif

		if (use_symbolic !=0 ){
			if ( makefile_copy != 0 && ismakefile(pathname)!=0 )
					copy_mode = HARD_COPY;
			else
					copy_mode = SYM_LINK;;
		}else if (use_link !=0 )
			copy_mode = HARD_LINK;;

		switch ( copy_mode ){
		case SYM_LINK:
#ifdef SVR4
				if (symlink(pathname, pathtarget) == -1){
#else
			{
#endif
				/*
				 * symlink() is not in the C library.
				 * csymlink() does not seem to work.
				 * However, "ln -s" does work in the ucb universe.
				 * Well, well, well.
				 */
				char buf[MAXPATHLEN * 2];
				sprintf(buf, "ucb ln -s %s %s", pathname, pathtarget);
				if (system(buf) != 0){
					fprintf(stderr,
						"%s: Cannot create symbolic link from %s to %s: %s\n",
						cmd, pathname, pathtarget,
						strerror(errno));
					closedir(dirp);
					return FAIL;
				}
			}
			break;
		case HARD_LINK:
			if (link(pathname, pathtarget) == -1){
				perror(cmd);
				closedir(dirp);
				return FAIL;
			}
			break;
		default:
			if (!copy_file(pathname, pathtarget,
				statb.st_mode)){
				closedir(dirp);
				fprintf(stderr,
					"%s: Cannot link %s to %s: %s\n",
					cmd, pathname, pathtarget,
					strerror(errno));
				closedir(dirp);
				return FAIL;
			}
			break;
		}

		fprintf(dotf, "%s\t%.5u\n", pathname, csum(pathname));
		/*fprintf(dotf, "%s\n", pathname);*/

		if (verbose)
			fprintf(stdout, "%c %s\n", copy_mode == SYM_LINK? 'l' : 'f',
				pathtarget);
	}

	closedir(dirp);
	return SUCCESS;
}
		

copy_file(source, dest, mode)
char *source, *dest;
mode_t mode;
{
	char buf[BUFSIZ];
	int src, dst, n;

	if ((src = open(source, O_RDONLY)) == -1){
		fprintf(stderr, "%s: Cannot open %s: %s\n", cmd, source,
			strerror(errno));
		return FAIL;
	}

	if ((dst = open(dest, O_WRONLY|O_CREAT|O_CREAT, mode)) == -1){
		fprintf(stderr, "%s: Cannot create %s: %s\n", cmd, dest,
			strerror(errno));
		close(src);
		return FAIL;
	}

	while ((n = read(src, buf, sizeof buf)) > 0){
		if (write(dst, buf, n) != n){
			fprintf(stderr, "%s: Write error in %s: %s\n", cmd,
				dest, strerror(errno));
			close(src);
			close(dst);
			return FAIL;
		}
	}
	if (n == -1){
		fprintf(stderr, "%s: Read error in %s: %s\n", cmd, source,
			strerror(errno));
		close(src);
		close(dst);
		return FAIL;
	}
	close(src);
	close(dst);
	return SUCCESS;
}

#ifdef SMS_VERSION
/*
 * smsmark: copy a .smsmark, and update the pathname. This is a kludge
 * to make smsmake work in the linked environment.
 */
smsmark(tree, src, dst)
char *tree, *src, *dst;
{
	FILE *fd_src, *fd_dst;
	char buf[MAXPATHLEN];
	int i, c;

	if ((fd_src = fopen(src, "r")) == NULL){
		error++;
		return SUCCESS;
	}

	if ((fd_dst = fopen(dst, "w")) == NULL){
		fclose(fd_src);
		return FAIL;
	}

	/*
	 * Copy up to third colon.
	 */
	for (i = 0 ; i < 3 ; i++){
		while ((c = getc(fd_src)) != ':'){
			if (c == EOF)
				goto error;
			if (putc(c, fd_dst) == EOF)
				goto error;
		}
		if (putc(':', fd_dst) == EOF)
			goto error;
	}
	/*
	 * Get pathname and replace with more appropriate one
	 */
	i = 0;
	while((c = getc(fd_src))  != ':'){
		if (c == EOF)
			goto error;
		buf[i++] = c;
	}
	buf[i] = '\0';
	if (!chgpath(buf, tree, fd_dst))
		goto error;
	
	if (putc(':', fd_dst) == EOF)
		goto error;

	/*
	 * write the end of the file
	 */
	while ((c = getc(fd_src)) != EOF)
		if (putc(c, fd_dst) == EOF)
			goto error;

	fclose(fd_src);
	fclose(fd_dst);
	return SUCCESS;

error:
	fprintf(stderr, "%s: Cannot update %s: %s\n", cmd, src,
		strerror(errno));
	fclose(fd_src);
	fclose(fd_dst);
	return FAIL;
}

/*
 * chgpath: update a path in .smsmark to reflect the new structure
 */
chgpath(path, tree, fd_dst)
char *path, *tree;
FILE *fd_dst;
{
	char buf[MAXPATHLEN];
	char *ptr;
	int len = strlen(tree);

	strcpy(buf, path);

	/*
	 * Locate tree component in name
	 */
	for(ptr = buf ; *ptr ; ptr++){
		if (strncmp(ptr, tree, len) == 0)
			break;
		if (putc(*ptr, fd_dst) == EOF)
			return FAIL;
	}
	if (!*ptr)
		return FAIL;

	/*
	 * Put targetdir instead of tree
	 */
	if (fputs(targetdir, fd_dst) == EOF)
		return FAIL;

	/*
	 * Put end of path
	 */
	if (fputs(ptr + len, fd_dst) == EOF)
		return FAIL;

	return SUCCESS;
}
#endif /* SMS_VERSION */

unsigned int
csum(f)
char *f;
{
	register unsigned int sum = 0;
	register int c;
	FILE *stream;

	if ((stream = fopen(f, "r")) == NULL) {
		perror(f);
		exit (1);
	}

	while((c = getc(stream)) != EOF) {
		if(sum & 01)
			sum = (sum >> 1) + 0x8000;
		else
			sum >>= 1;
		sum += c;
		sum &= 0xFFFF;
	}

	fclose(stream);

	return (sum);
}

cleanup()
{
	FILE *fp;
	int i, o;
	unsigned int sum;
	char finame[MAXPATHLEN];
	char unpath[MAXPATHLEN];

	for (i = 0; trees[i]; ++i) {

		strcpy(finame, targetdir);
		strcat(finame, "/.");
		strcat(finame, trees[i]);

		if ((fp = fopen(finame, "r")) == NULL) {
			perror(finame);
			exit (0);
		}

		while ((fscanf(fp, "%s %u", finame, &sum)) != EOF) {
			o = sfnd(finame, trees[i]);
			o += (strlen(trees[i]) + 1);
			strcpy(unpath, targetdir);
			strcat(unpath, "/");
			strcat(unpath, &finame[o]);
			if (verbose)
				fprintf(stdout, "unlink %s\n", unpath);
			if ((unlink(unpath)) == -1) {
				perror(unpath);
			}
		}

		fclose(fp);
		strcpy(unpath, targetdir);
		strcat(unpath, "/.");
		strcat(unpath, trees[i]);
		if (verbose)
			fprintf(stdout, "unlink %s\n", unpath);
		if ((unlink(unpath)) == -1) {
			perror(unpath);
		}
	}

	exit (0);
}

#ifndef SVR4
char *
strerror()
{
	extern char *sys_errlist[];

	return (sys_errlist[errno]);
}
#endif	/* SVR4 */

up_date()
{
	exit (0);
}

sfnd(as1,as2)
char *as1,*as2;
{
	register char *s1,*s2;
	register char c;
	int offset;

	s1 = as1;
	s2 = as2;
	c = *s2;

	while (*s1)
		if (*s1++ == c) {
			offset = s1 - as1 - 1;
			s2++;
			while ((c = *s2++) == *s1++ && c) ;
			if (c == 0)
				return(offset);
			s1 = offset + as1 + 1;
			s2 = as2;
			c = *s2;
		}
	 return(-1);
}

int ismakefile( mk_pathname)
   char	*mk_pathname;
{
   char	*mk_expr;
   int t=0;

   mk_expr = regcmp(".*uts/.+\\.mk$",(char *)0);
   if ((t = (int)regex(mk_expr,mk_pathname)) == 0){
	free(mk_expr);
	mk_expr = regcmp(".*uts/.*[Mm]akefile$",(char *)0);
	t=(int)regex(mk_expr,mk_pathname);
   }

   free(mk_expr);
   return t;
}
