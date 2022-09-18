/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/interface/ie_build.c	1.3.13.3"
#ident	"$Header: ie_build.c 2.1 91/08/19 $"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <mac.h>
#include <unistd.h>
#include "intf.h"
#include "print_err.h"
#include "errors.h"
#include "userdefs.h"
#include "sysadm.h"
#include "menu_io.h"
#include "menutrace.h"
#include "../intf_reloc/oldmenu.h"

extern char	*getenv(),
		*menutok();
extern int	ismenu(),
		match_cnt(),
		find_expr(),
		mk_mt_node();
extern void	exit(),
		print_err(),
		lineage(),
		clr_marks();

#define PL_LEN (sizeof(PHOLDER)-1)
#define TMP_NM "/tmp/tmp.menu"

int task_id = 0;
TASKDESC *root = (TASKDESC *)0;
TASKDESC *thread_strt = (TASKDESC *)0;
TASKDESC **thread_end = &thread_strt;

FILE *iefp;			/* internal express DB file pointer */

#ifdef DEBUG
	FILE *debug;
#endif


static void xseed_proc();
static void old_trace();
static void iexpr_write();
static int menu_build();
static int menu_trace(), r3_desc();
static char *pkginst;

main(argc, argv)
int	argc;
char	*argv[];
{
	char fn1[PATH_MAX], fn2[PATH_MAX];
	char *oambase;
	level_t expr_lid = 0;

#ifdef DEBUG
	if((debug = fopen("/tmp/debug", "w")) == (FILE *)0) {
		fprintf(stderr, "Could not open file /tmp/debug\n");
		exit(1);
	}
#endif

	if((oambase = getenv(OAMBASE)) == NULL)
		oambase = "/usr/sadm/sysadm";

	/*
	 * Set pkginst
	 */
	pkginst = NULL;
	pkginst = getenv("PKGINST");
#ifdef DEBUG
	fprintf(stderr, "pkginst: %s\n", pkginst);
#endif

	(void) sprintf(fn1, "%s/%s%s", oambase, MAIN_PATH, I_EXPR_PTH);
	if (menu_build("/usr/sadm/sysadm/menu") != 0)
		exit(1);

	thread_strt = (TASKDESC *)0;
	(void) find_expr(root, OLD_SYS, 0);

	if(thread_strt != (TASKDESC *)0) {
		thread_strt->action = "O";
		old_trace(thread_strt, "/usr/admin/menu");
	}

	/* re-create internal express mode file "i_expr" */
	if((iefp = fopen(fn1, "w")) == (FILE *)0) {
		print_err(NOT_OPEN, "ie_build", fn1);
		exit(1);
	}

	/* write out menu hierarchy */
	iexpr_write(root->child, 0);
	(void) fflush(iefp);
	/* read and process express mode seed file */
	(void) sprintf(fn2, "%s/%s%s", oambase, MAIN_PATH, EXPR_PTH);
	xseed_proc(fn2);
	(void) fclose(iefp);

	lvlfile("/usr/sadm/sysadm/menu", MAC_GET, &expr_lid);
	lvlfile(fn1, MAC_SET, &expr_lid);
	if((chmod(fn1, 0464)) < 0) {
		print_err("could not chmod i_expr file", "chmod", fn1);
		exit(-1);
	}


	/* 
	 * Now that we've translated all .menu files into Menu. files,
	 * do final installf to indicate that installation if final.
	 * This is done only if this package is not a pre-SVR4 package.
	 */
	if ((pkginst != NULL) && (strcmp(pkginst, "_PRE4.0") != 0)
		&& (strcmp(pkginst, "_ONLINE") != 0)) { 
			if(execl("/usr/sbin/installf", "installf", "-f",  pkginst, (char *)0) <0) {
				print_err("could not exec final installf -f", "installf");
				exit(-1);
		}
	}

	exit(0);
	/*NOTREACHED*/
}


static void
xseed_proc(fn)
char	*fn;
{
	FILE *xseedp;			/* express seed file pointer */
	char *eof, inbfr[200];		/* input processing */
	char *xname, *cmdstr, *link;	/* express file fields */
	int new_cnt, old_cnt;
	TASKDESC *p;
	char path_lineage[PATH_MAX];	/* path to an action */

	if((xseedp = fopen(fn, "r")) == (FILE *)0) {
		print_err(NO_EXPR, "ie_build");
	}

	/* for each record */
	do {
		if((eof = fgets(inbfr, 200, xseedp)) == NULL) continue;
		if(inbfr[0] == '#') continue;	/* comment */
		for (eof = inbfr; *eof != '\0'; ++eof) if(*eof == '\n') { *eof = '\0'; }
		if((xname = menutok(inbfr)) == NULL) continue;
		if((cmdstr = menutok(NULL)) == NULL) continue;
		link = menutok(NULL); /*optional*/;

		switch (*cmdstr) {

		case 'E':	/* exec string */
			(void) fprintf(iefp, "e\t0\t%s\t%s\n", xname, link);
			break;

		case 'P':	/* placeholder */
			(void) fprintf(iefp, "p\t0\t%s\n", xname);
			break;

		case 'L':
			/* process link information */
			clr_marks(root);
			thread_strt = (TASKDESC *)0;
			thread_end = &thread_strt;
			if(find_expr(root, strdup(link), 1) == 0) {
				print_err(EXPR_SYNTAX, "ie_build", xname, link);
				break;
			}
			(void) match_cnt(&new_cnt, &old_cnt);
			if(new_cnt != 1) {
				print_err(EXPR_SYNTAX, "ie_build", xname, link);
				(void) fprintf(stderr, " not unique menu path\n");
				for (p = thread_strt; p != (TASKDESC *)0; p = p->thread) {
					if(*(p->action) == 'O') continue;
					path_lineage[0] = '\0';
					lineage(p, path_lineage);
					(void) fprintf(stderr, "\t%s\n", path_lineage);
				}
			} else {
				/* find  non-old_sysadm match and write */
				for (p = thread_strt; (p != (TASKDESC *)0) &&
					(*p->action != 'O'); p = p->thread);
				(void) fprintf(iefp, "r\t%d\t%s\n", thread_strt->ident, xname);
			}
			break;

		default:
			/* syntax error */
			print_err(EXPR_SYNTAX, "ie_build", xname, link);
			break;

		}/*endswitch*/
	} while(eof != NULL);
	(void) fclose(xseedp);
}


static void
iexpr_write(p, indx)
TASKDESC *p;
int	indx;	/* index of parent node */
{
	TASKDESC *y;

	/* write out menu hierarchy */
	for (y = p; y != (TASKDESC *)0; y = y -> next) {
		if(*y->action == 'O') {
		  (void) fprintf(iefp, "%d\t%d\t%s\tO\n", y->ident, indx, y->tname);
		} else {
		  (void) fprintf(iefp, "%d\t%d\t%s\n", y->ident, indx, y->tname);
		}
		if(y->child != (TASKDESC *)0)
			iexpr_write(y->child, y->ident);
	}

}


extern int
menu_build(base)
char *base;		/* base directory */
{
	/* set up first/root node in hierarchy */
	root = (TASKDESC *)0;
	(void) mk_mt_node(&root, (TASKDESC *)0, "main");
	root->action = "main.menu";
	root->ident = 0;
	if(menu_trace(root, base) != 0) return(-1);
	return (0);
}

static int
menu_trace(tsk_nodp, dir)
TASKDESC *tsk_nodp;
char	*dir;
{
	char *tasknm, *n_action;	/* .menu field pointers */
	char *menu_dir;			/* path to add to */
	char newdir[PATH_MAX];
	struct menu_line *ptr_menu;	/* pointer to first line of menu item */
	static struct menu_line m_line;	/* first line of menu item */
	TASKDESC *tp;
	char *x;
	char *str;
	char *menu_name;
	char *path;
	char *ptr;
	char *target;
	char *basename;
	int i;
	int fd;
	int noexist;
	level_t lid = 0;
	struct passwd *pswd; 
	pid_t status;
	uid_t root_uid;
	uid_t sys_gid;
	char str_lid[128];
#ifdef DEBUG
	char line[PATH_MAX];
#endif

	extern char *read_item();

	ptr_menu = &m_line;		/* init pointer */

	/* ignore ifplaceholder: no access through main.menu */
	if(strncmp(tsk_nodp->action, PHOLDER, PL_LEN) == 0) return(0);

	/* if"*.menu" file, then process sub-menu */
	if(ismenu(tsk_nodp->action)) {
		FILE *mfilep;
		char *instr;

		/* open .menu file */
		(void) sprintf(newdir, "%s/%s", dir, tsk_nodp->action);
#ifdef DEBUG
		fprintf(debug, "%s\n", newdir);
#endif

		/*
		 * Get the level of the .menu file to
		 * be used to set the Menu. file that
		 * will be generated.
		 */
		(void) lvlfile(newdir, MAC_GET, &lid);
		sprintf(str_lid, "%ld", lid);
			

		/*
		 * Build "Menu." file name where output
		 * of object_gen is to be placed.
		 */
		target = strdup(newdir);
		basename = strrchr(target, '/');
		*(basename++) = '\0';
		path = strdup(target);
		menu_name = strdup(basename);
		str = strdup(strtok(basename, "."));
		strcat(target, "/Menu.");
		strcat(target, str);

		/*
		 *  Now exec object_gen to translate
		 * ".menu" file to "Menu." FMLI form.
		 */
#ifdef DEBUG
		/**********************************************
		sprintf(line, "rm -f %s", target);
		fprintf(debug, "%s\n", line);
		system(line);
		***********************************************/
#endif

		/* Check if target Menu.* file exists */
		if((access(target, F_OK)) == -1) {
			if (errno == EACCES) {
				print_err("access error", "ie_build", target);
				exit(1);
			}
			noexist=1;	/* Menu.* file does not exist */
		}
		else
			noexist=0;	/* Menu.* file already exists */


		/*
		 * Whether the 'Menu.' file associated with the current '.menu' file we are
		 * dealing with existed prior to this invocation of ie_build or not, we exec
		 * object_gen to perform the translation.  This is done for the case where the
		 * associated 'menu.' file was modified to include new tasks associated with
		 * the pkginst being installed.  This will ensure that the 'Menu.' file contains
		 * the same modifications.
		 *
		 * A child process runs object_gen redirecting standard output to the current
		 * 'Menu.' file.  It also changes the 'Menu.' file's owner and group to "root"
		 * and "sys", respectively.
		 */

		if (fork() != 0) {	/* Parent - wait for object_gen to complete */
			if(wait(&status) == 1) {
				print_err("failed waiting for object_gen to complete", "wait");
				exit(-1);
			}
		}
		else {		/* Child - exec object_gen */
			umask(0113);

			if((fd = open(target, O_WRONLY|O_TRUNC|O_CREAT, 0664)) < 0)
				print_err(NOT_OPEN, "ie_build", target);
			/* dup stdout to 'Menu.*' file */
			close(1);		/* Close stdout */
			if(dup(fd) < 0) {	/* target is stdout */
				print_err(NODUP, "ie_build", target);
				exit(1);
			}
			close(fd);		/* close uneeded fd */
	

#ifdef DEBUG
			fprintf(stderr, "\n*** object_gen %s %s > %s\n", path, menu_name, target);
#endif

			if(execl("/usr/sadm/sysadm/bin/object_gen", "object_gen", path, menu_name, (char *)0) <0) {
				print_err("could not exec object_gen", "object_gen", menu_name);
				exit(-1);
			}
			 
		}

		/* 
		 * If the 'Menu.' file did not exist, that is we just introduced a brand
		 * new '.menu' into the system associated with the current pkginst, then
		 * we should run installf on it to register it in the contents file.  This
		 * is required so that it is removed when the package referred to by pkginst
		 * is removed.
		 *
		 * fork again to perform installf on new Menu. file.
		 */

		if (noexist) {
			if (fork() != 0) {
				if(wait(&status) == 1) {
					print_err("failed waiting for object_gen to complete", "wait");
					exit(-1);
				}
			}
			else {
				/*
				 * Now installf the Menu.* file so that it is placed in the
				 * contents file.  This will ensure that the Menu.* file will
				 * be removed during a pkgrm of the package instance that it
				 * is associated with.  After all *.menu files are translated
				 * translated to Menu.* files, exec installf -f to commit to
				 * the changes.
				 */
				 if ((pkginst != NULL) && (strcmp(pkginst, "_PRE4.0") != 0)
				    && (strcmp(pkginst, "_ONLINE") != 0 )) {
				 	if(execl("/usr/sbin/installf", "installf", "-c", "OAMadmin", pkginst, target, "v", "0644", "root", "sys", str_lid, "NULL", "NULL", (char *)0) <0) {
						print_err("could not exec installf", "installf", menu_name);
						exit(-1);
					}
			 	}
			}

			if((pswd = getpwnam("root")) == NULL) {
				print_err("could not get uid for root", "getpwnam", "root");
				exit(-1);
			}
			root_uid = pswd->pw_uid;
			if((pswd = getpwnam("sys")) == NULL) {
				print_err("could not get gid for sys", "getpwnam", "sys");
				exit(-1);
			}
			if((chmod(target, 0464)) < 0) {
				print_err("could not chmod Menu file", "chmod", target);
				exit(-1);
			}
			sys_gid = pswd->pw_gid;
			if (chown(target, -1, sys_gid) == -1) {
				print_err("could not change group to sys", "chown", target); 
				exit(-1);
			}
			if (chown(target, root_uid, -1) == -1) {
				print_err("could not change owner to root", "chown", target); 
				exit(-1);
			}
			noexist=0;
		}

		
		if((mfilep = fopen(newdir, "r")) == (FILE *)0) {
			print_err(NOT_FOUND, "ie_build", newdir);
			return(-1);
		}
		menu_dir = strrchr(newdir, '/');
		++menu_dir;
		*menu_dir = '\0';
	
		/* read past header info to first menu item */
		while((instr = read_item(ptr_menu, mfilep, FULL_LINE)) != NULL) {
			if((*instr == '\n') || (*instr == '#')) continue;
		
			x = instr + strlen(instr) - 1;
			if(*x == '\n')  *x = '\0';
			/* get task, discard description, and get action field */
			if((tasknm  = menutok(instr)) == NULL) continue;
			if(menutok(NULL) == NULL) continue;
			if((n_action  = menutok(NULL)) == NULL) continue;
	
			/* create a child to this node in the hierarchy */
			if(mk_mt_node(&(tsk_nodp->child), tsk_nodp, tasknm) != 0) return(-1);
			tsk_nodp->child->action = strdup(n_action);
	
			/* handle OLD SYSADM case */
			if(strncmp(n_action, OLD_SYSADM, sizeof(OLD_SYSADM) - 1) == 0) {
				/* indicate OLD_SYSADM branch */
				tsk_nodp->child->action = "O";
			}
		}
	
		(void) fclose(mfilep);
	
		/* trace all children */
		for (tp = tsk_nodp->child; tp != (TASKDESC *)0; tp = tp -> next) {
			/* look at child menu/action */
			if(menu_trace(tp, newdir) != 0) {
				return (-1);
			}
		}
		return (0);
	}
}


static void
old_trace(tsk_nodp, dir)
TASKDESC *tsk_nodp;
char	*dir;
{
	TASKDESC *tp;
	DIR	*dirp;
	struct stat	xstat;
	struct dirent *dep;
	char	b1[PATH_MAX], *endp;

	if(tsk_nodp == (TASKDESC *)0) return;
	if((dirp = opendir(dir)) == (DIR *)0) {
		/* not a directory */
		return;
	}
	(void) strcpy(b1, dir);
	for (endp = b1; *endp != '\0'; ++endp) ;
	*endp++ = '/';  *endp = '\0';
	while ((dep = readdir(dirp)) != (struct dirent *)0) {
		/* ignore ".", ".." and "DESC" (description) entries */
		if(*dep->d_name == '.') continue;
		if(strcmp(dep->d_name, "DESC") == 0) continue;

		(void) strcpy(endp, dep->d_name);
		(void) stat(b1, &xstat);
		/* ifa sub-dir, only process ifdir has a DESC file */
		if(xstat.st_mode & S_IFDIR) {
			(void) strcat(endp, "/DESC");
			if(stat(b1, &xstat) == -1) continue;
		} else {
			/* ifno proper R3 descr., do not process */
			if(r3_desc(b1) == -1) continue; 
		}
		if(mk_mt_node(&(tsk_nodp->child), tsk_nodp, dep->d_name) != 0)
			return;
		tsk_nodp->child->action = "O";
	}
	(void) closedir(dirp);

	/* trace all child dirs */
	for (tp = tsk_nodp->child; tp != (TASKDESC *)0; tp = tp -> next) {
		/* look at child menu/action */
		(void) strcpy(endp, tp->tname);
		old_trace(tp, b1);
	}
	return;
}


static int
r3_desc(fn)
char	*fn;
{
	FILE	*fp;
	char	b[300];

	if((fp = fopen(fn, "r")) == (FILE *)0) {
		return(-1);
	}
	while (fgets(b, 300, fp) != (char *)0) {
		if(strncmp(b, MENUHDR, sizeof(MENUHDR) - 1) == 0) {
			(void) fclose(fp);
			return(0);
		}
	}
	(void) fclose(fp);
	return(-1);
}
