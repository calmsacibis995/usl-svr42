/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fs.cmds:i386/cmd/fs.d/df.c	1.34.14.9"
#ident  "$Header: df.c 1.4 91/09/30 $"

/***************************************************************************
 * Command: df
 * Inheritable Privileges: P_COMPAT,P_MACREAD,P_DACREAD,P_DEV
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfstab.h>
#include <sys/mnttab.h>
#include <sys/param.h>		/* for MAXNAMELEN - used in devnm() */
#include <dirent.h> 	
#include <fcntl.h>
#include <sys/wait.h>
/* new includes added for security */
#include	<mac.h>
#include <errno.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>

#define MNTTAB	"/etc/mnttab"
#define VFSTAB	"/etc/vfstab"
#define MAX_OPTIONS	20	/* max command line options */

char *argp[MAX_OPTIONS];	/* command line for specific module*/
int argpc;
int status;
char path[BUFSIZ];
int k_header = 0;
int header = 0;
int eb_flg = 0;

#define DEVLEN 1024	/* for devnm() */
struct stat S;
struct stat Sbuf;
char *devnm();
char *basename();
int is_remote();

/*
 * Procedure:     main
 *
 * Restrictions:
                 getopt: none
                 setlocale: none
                 stat(2): none
                 pfmt: none
                 strerror: none
                 printf: none
                 fopen: P_MACREAD for MNTTAB and VFSTAB
                 getmntent: P_MACREAD for MNTTAB
                 statvfs(2): none
                 fclose: none
                 getmntany: P_MACREAD for MNTTAB
                 getvfsspec: P_MACREAD for VFSTAB
 * Generic DF 
 *
 * This is the generic part of the df code. It is used as a
 * switchout mechanism and in turn executes the file system
 * type specific code located at /usr/lib/fs/FSType. 
 *
 */

static int	mac_install;	/* defines whether security MAC is installed */
static level_t	level;		/* new MAC security level ceiling for mounted file system */
static const char badopen[] = ":4:Cannot open %s: %s\n";
static const char badstatvfs[] = ":250:statvfs() on %s failed: %s\n";
static const char badstat[] = ":5:Cannot access %s: %s\n";
static const char otherfs[] = ":251:%s mounted as a %s file system\n";

static const char mnttab[] = MNTTAB;
static const char vfstab[] = VFSTAB;

main (argc, argv)
int argc;
char *argv[];

{
	int arg;			/* argument from getopt() */
	int usgflg, b_flg, e_flg, V_flg, o_flg, k_flg;
	int t_flg, g_flg, n_flg, l_flg, f_flg, F_flg, v_flg;
/* Enhanced Application Compatibility Support */
	int i_flg = 0;
/* End Enhanced Application Compatibility Support */

	int i, j;
	int status;

	mode_t mode;

	extern char *optarg;		/* getopt(3c) specific */
	extern int optind;
	extern int opterr;
	char	 *res_name, *s;
	int	 errcnt = 0;
	int	 exitcode = 0;
	int	 notfound=1;
	int	 res_found=0;

	char *FSType = NULL;		/* FSType */
	char *oargs;			/* FSType specific argument */
	char *cmdname; 			/* command name or path */
	char options[MAX_OPTIONS];	/* options for specific module */

	char *usage = ":1122:Usage:\ndf [-F FSType] [-begiklntVv] [current_options] [-o specific_options] [directory | special | resource...]\n";

	FILE *fp, *fp2, *fdopen();
	struct mnttab mountb;
	struct mnttab mm;
	struct statvfs statbuf;
	struct vfstab	vfsbuf;
	struct vfstab ss;
	struct stat stbuf;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");


	usgflg = b_flg = e_flg = V_flg = k_flg = eb_flg = 0;
	t_flg = g_flg = n_flg = l_flg = f_flg = v_flg = 0; 


	F_flg = o_flg = 0;
	cmdname = argv[0];	
	strcpy(options, "-");

	/* see if the command called is devnm */

	s = basename(argv[0]);
	if(!strncmp(s,"devnm",5)) {
		(void)setlabel("UX:devnm");

		while(--argc) {
			if(stat(*++argv, &S) == -1) {
				pfmt(stderr, MM_ERROR, badstat, *argv,
					strerror(errno));
				errcnt++;
				continue;
			}
			res_name = devnm();
			if(res_name[0] != '\0')
				printf("%s %s\n", res_name, *argv);
			else {
				pfmt(stderr, MM_ERROR, ":253:%s not found\n",
					*argv);
				errcnt++;
			}
		}
		exit(errcnt);
	}
	(void)setlabel("UX:df");

	/* establish upfront if the enhanced security package is installed */
	if ((lvlproc(MAC_GET, &level) == -1) && (errno ==ENOPKG))
		mac_install = 0;	
	else
		mac_install = 1;

	/* if init is running df, then we need MACREAD, otherwise we don't want it*/

	if (mac_install && level)
		procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);

	/* open mnttab and vfstab */

	if (( fp = fopen(MNTTAB, "r")) == NULL){
		pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
		exit(2);
	}

	if (( fp2 = fopen(VFSTAB, "r")) == NULL){
		pfmt(stderr, MM_ERROR, badopen, vfstab, strerror(errno));
		exit(2);
	}

	if (mac_install && level)
		procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
	/* 
 	* If there are no arguments to df then the generic 
 	* determines the file systems mounted from /etc/mnttab
 	* and does a statvfs on them and reports on the generic
 	* superblock information
 	*/

	if (argc == 1) {		/* no arguments or options */

		while (( i = getmntent(fp, &mountb)) == 0 ) {
			if ((j = statvfs(mountb.mnt_mountp, &statbuf)) != 0){
				pfmt(stderr, MM_ERROR, badstatvfs,
					mountb.mnt_mountp, strerror(errno));
				exitcode=1;
				continue;
			}
			print_statvfs(&mountb, &statbuf, 'x');
 		}
		if (i > 0 ) {
			mnterror(i);
			exit(1);
		}
		exit(exitcode);
	}

	/* One or more options or arguments */
	/* Process the Options */ 

	while ((arg = getopt(argc,argv,"F:o:?beikVtgnlfv")) != -1) {

		switch(arg) {
		case 'v':       /* print verbose output */
			 v_flg = 1;
			 break;
		case 'b':	/* print kilobytes free */
			b_flg = 1;
			strcat(options, "b");
			break;
		case 'e':	/* print file entries free */
			e_flg = 1;
			strcat(options, "e");
			break;
		case 'V':	/* echo complete command line */
			V_flg = 1;
			break;
		case 't':	/* full listing with totals */
			t_flg = 1;
			strcat(options, "t");
			break;
		case 'g':	/* print entire statvfs structure */
			g_flg = 1;
			strcat(options, "g");
			break;
		case 'n':	/* print FSType name */
			n_flg = 1;
			strcat(options, "n");
			break;
		case 'l':	/* report on local File systems only */
			l_flg = 1;
			strcat(options, "l");
			break;
		case 'F':	/* FSType specified */
			if (F_flg) {
				pfmt(stderr, MM_ERROR, ":254:More than one FSType specified\n");
				pfmt(stderr, MM_ACTION, usage);
				exit(2);
			}
			F_flg = 1;
			FSType = optarg;
			if (( i = strlen(FSType)) > 8 ) {
				pfmt(stderr, MM_ERROR, ":255:FSType %s exceeds 8 characters\n", FSType);
				exit(2);
			}
			break;
		case 'o':	/* FSType specific arguments */
			o_flg = 1;
			oargs = optarg;
			break;
		case 'f':	/* perform actual count on free list */
			f_flg = 1;
			strcat(options, "f");
			break;
		case 'k':	/* new format */
			k_flg = 1;
			strcat(options, "k");
			break;

		/* Enhanced Application Compatibility Support */
		case 'i':	/* print inode info */
			i_flg = 1;
			strcat(options, "i");
			break;
		/* End Enhanced Application Compatibility Support */

		case '?':	/* print usage message */
			usgflg = 1;
			strcat(options, "?");
		}
	}

	/* i_flg related code below is part of the 
	   Enhanced Application Compatibility Support */
	if (v_flg && !i_flg)
		pfmt(stdout, MM_NOSTD,
		    ":1123:Mount Dir  Filesystem         blocks      used      avail  %%used\n");
	else if (v_flg && i_flg)
		pfmt(stdout, MM_NOSTD,
		    ":1124:Mount Dir  Filesystem        blocks     used     avail %%used iused  ifree %%iused\n");
	else if (!v_flg && i_flg)
		pfmt(stdout, MM_NOSTD,
			":1078:Mount Dir  Filesystem            iused     ifree    itotal %%iused\n");
	/* End Enhanced Application Compatibility Support */

	if ((!F_flg) && (usgflg)) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, usage);
		exit(2);
	}
	if (b_flg && e_flg) {
		eb_flg++;
	}

	/* if no arguments (only options): process mounted file systems only */
	if (optind == argc) {
		if (V_flg) {
			while ((i = getmntent(fp, &mountb)) == 0 ) {
				if ((F_flg) && (strcmp(FSType, mountb.mnt_fstype) != 0))
					continue;
				if ((l_flg) && (is_remote(mountb.mnt_fstype)))
					continue;
				mk_cmdline(mountb.mnt_fstype,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
		  		echo_cmdline(argp, argpc, mountb.mnt_fstype);
		 	}
			if (i > 0 ) {
				mnterror(i);
				exit(1);
			}
		 	exit(0);
		}
		if (f_flg || o_flg) {	/* specific df */
			while ((i = getmntent(fp, &mountb)) == 0 ) {
				if ((F_flg) && ( strcmp(FSType, mountb.mnt_fstype) != 0))
					continue;
				mk_cmdline(mountb.mnt_fstype,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
				build_path(mountb.mnt_fstype, path);
				exec_specific(mountb.mnt_fstype); 
			}
			if (i > 0 ) {
				mnterror(i);
				exit(1);
			}
			exit(0);
		} /* end specific df */

		if ( F_flg && usgflg ) {
			mk_cmdline(FSType,
					options,
					o_flg,
					oargs,
					NULL);
			build_path(FSType, path);
			exec_specific(FSType);
			exit(0); 	/* only one usage mesg required */
		}
		/* f or o flags are not set */
		while (( i = getmntent(fp, &mountb)) == 0 ) {
			if ((F_flg) && (strcmp(FSType, mountb.mnt_fstype) != 0))
				continue;
	    		if ((l_flg) && (is_remote(mountb.mnt_fstype))) 
				continue;
			j = statvfs(mountb.mnt_mountp, &statbuf);
			if (j != 0){
				pfmt(stderr, MM_ERROR, badstatvfs,
					mountb.mnt_mountp, strerror(errno)); 
				exitcode=1;
				continue;
			}
			if (g_flg) {
				print_statvfs(&mountb, &statbuf, 'g');
				continue;
			}
			if (k_flg) {
				print_statvfs(&mountb, &statbuf, 'k');
				continue;
			}

			/* i_flg related code below is part of the 
		       Enhanced Application Compatibility Support */
			if (v_flg && !i_flg) {
				print_statvfs(&mountb, &statbuf, 'v');
				continue;
			}
			else if (v_flg && i_flg) {
				print_statvfs(&mountb, &statbuf, 'vi');
				continue;
			}
			else if (!v_flg && i_flg) {
				print_statvfs(&mountb, &statbuf, 'i');
				continue;
			}
			/* End Enhanced Application Compatibility Support */

			if (t_flg) {
				print_statvfs(&mountb, &statbuf, 't');
				continue;
			}
			if (b_flg){
				print_statvfs(&mountb, &statbuf, 'b');
			}
			if (e_flg){
				print_statvfs(&mountb, &statbuf, 'e');
			}
			if (n_flg) {
				print_statvfs(&mountb, &statbuf, 'n');
			}
			if (b_flg || n_flg || e_flg)  
				continue;
			if (F_flg){
				print_statvfs(&mountb, &statbuf, 'x');
				continue;
			}
			print_statvfs(&mountb, &statbuf, 'x');
 		}
		if (i > 0 ) {
			mnterror(i);
			exit(1);
		}
		exit(exitcode);

	}  /* end case of no arguments */

	/* arguments could be mounted/unmounted file systems */
	for (; optind < argc; optind++) {

		fclose(fp);
		fclose(fp2);
		if (( fp = fopen(MNTTAB, "r")) == NULL){
			pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
			exit(2);
		}
		if (( fp2 = fopen(VFSTAB, "r")) == NULL){
			pfmt(stderr, MM_ERROR, badopen, vfstab, strerror(errno));
			exit(2);
		}

		mntnull(&mm);
		mm.mnt_special = argv[optind];

		/* case argument is special from mount table */
		i = getmntany(fp, &mountb, &mm);
		if (i == 0){

			if ((l_flg) && (is_remote(mountb.mnt_fstype)))
				continue;
			if ((F_flg)&&(strcmp(mountb.mnt_fstype, FSType) != 0)){
				pfmt(stderr, MM_WARNING, otherfs,
					mm.mnt_special, mountb.mnt_fstype);
				exitcode=1;
				continue;
			}
			if (V_flg) {
				if (!F_flg) 
					FSType = mountb.mnt_fstype;
				mk_cmdline(FSType,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
				echo_cmdline(argp, argpc, FSType);
				continue;
			}
			if (f_flg || o_flg) {
				if (!F_flg) 
					FSType = mountb.mnt_fstype;
				mk_cmdline(FSType,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
				build_path(FSType, path);
				exec_specific(FSType);
				continue;
			}
			j = statvfs(mountb.mnt_mountp, &statbuf);
			if (j != 0) {
				pfmt(stderr, MM_ERROR, badstatvfs,
					mountb.mnt_mountp, strerror(errno)); 
				exitcode=1;
				continue;
			}
			if (F_flg && usgflg) {
				mk_cmdline(FSType,
					options,
					o_flg,
					oargs,
					mountb.mnt_special);
				exec_specific(FSType);
				exit(0);
			}
			if (g_flg) {
				print_statvfs(&mountb, &statbuf, 'g');
				continue;
			}
			if (k_flg) {
				print_statvfs(&mountb, &statbuf, 'k');
				continue;
			}
			if (t_flg) {
				print_statvfs(&mountb, &statbuf, 't');
				continue;
			}
			if (n_flg) {
				print_statvfs(&mountb, &statbuf,'n');
			}
			if (b_flg) {
				print_statvfs(&mountb, &statbuf, 'b');
			}
			if (e_flg) {
				print_statvfs(&mountb, &statbuf, 'e');
			}
			if ( b_flg || e_flg || n_flg )
				continue;
			if (F_flg) {
				print_statvfs(&mountb, &statbuf, 'x');
				continue;
			}
			print_statvfs(&mountb, &statbuf, 'x');
			continue;
		}

		/* perform a stat(2) to determine file type */

		/* stat fails */
		i = stat(argv[optind], &stbuf);
		if (i == -1) {
			pfmt(stderr, MM_ERROR, badstat, argv[optind],
				strerror(errno));
			exitcode=1;
		 	continue;
		}
		if ((( stbuf.st_mode & S_IFMT) == S_IFREG) ||
			(( stbuf.st_mode & S_IFMT) == S_IFIFO )) {
			pfmt(stderr, MM_ERROR, ":256:(%-10.32s) not a file system, directory or mounted resource\n",
				argv[optind]);
			exitcode=1;
			continue;
		}

		/* if block or character device */

		if ((( mode = ( stbuf.st_mode & S_IFMT )) == S_IFBLK) || 
			( mode == S_IFCHR )) {

			/* check if the device exists in vfstab */
			i = getvfsspec(fp2, &vfsbuf, argv[optind]);
			if (i != 0) {
				if (!F_flg) {
					pfmt(stderr, MM_ERROR, ":257:FSType cannot be identified\n");
					exit(2);	
				}
				mk_cmdline(FSType, 
					options,
					o_flg,
					oargs,
					argv[optind]);
				if (V_flg) {
					echo_cmdline(argp, argpc, FSType);
					continue;
				}
				build_path(FSType, path);
				exec_specific(FSType);
				continue;
			}

			/* device exists in vfstab */
			if (!F_flg) 
				FSType = vfsbuf.vfs_fstype;
			if ( g_flg || n_flg || l_flg ){
				pfmt(stderr, MM_ERROR, ":258:Options g, n or l not supported for unmounted FSTypes\n");
				exit(2);
			}
			mk_cmdline(FSType, 
					options,
					o_flg,
					oargs,
					vfsbuf.vfs_special);
			if (V_flg) {
				echo_cmdline(argp, argpc, FSType);
				continue;
			}
			build_path(FSType, path);
			exec_specific(FSType);
			continue;

		} /* end: block or character device */
		/* argument is a path */

		j = statvfs(argv[optind], &statbuf);
		if (j != 0) {
			pfmt(stderr, MM_ERROR, badstatvfs, mountb.mnt_mountp,
				strerror(errno));
			exitcode=1;
			continue;
		}
		if ((F_flg)&&(strcmp(statbuf.f_basetype, FSType) != 0)){
			pfmt(stderr, MM_WARNING, otherfs, argv[optind],
				statbuf.f_basetype);
			exitcode=1;

			continue;
		}
		if (V_flg) {
			mk_cmdline(statbuf.f_basetype,
					options,
					o_flg,
					oargs,
					argv[optind]);
			echo_cmdline(argp, argpc, statbuf.f_basetype);
			continue;
		}
		if (f_flg || o_flg) {
			mk_cmdline(statbuf.f_basetype,
					options,
					o_flg,
					oargs,	
					argv[optind]);
			build_path(statbuf.f_basetype, path);
			exec_specific(statbuf.f_basetype);
			continue;
		}
		/* rest handled by generic */
		mountb.mnt_mountp = argv[optind];	/* mount pt is file */
		i = stat(argv[optind], &S);
		if (i == -1) {
			pfmt(stderr, MM_ERROR, badstat, argv[optind],
				strerror(errno));
			exitcode=1;
		 	continue;
		}
		res_name = devnm();
		/* Even if the resource name is found here, we may not
		   have the correct mountpoint(in the case where a path
		   was given below a mountpoint, ie, /usr is the mntpt,
		   but the argument given was /usr/include/sys.)
		*/
		   
		if(res_name[0] != '\0') {
			res_found++;
		}
			fclose(fp);
			fp = fopen(MNTTAB, "r");
			if (fp == NULL) {
				pfmt(stderr, MM_ERROR, badopen, mnttab,
					strerror(errno));
				exit(2);
			}
			mntnull(&mm);
			mm.mnt_mountp = argv[optind];
			/* case argument is mountpoint from mount table */
			/* if argument is a path below a mountpoint, then sat
			   each entry in the mttab and check if the device no.
			   matches.  
			*/
			i = getmntany(fp, &mountb, &mm);
			if (i != 0){
				if (i < 0) {
					fclose(fp);
					fp = fopen(MNTTAB, "r");
					if (fp == NULL){
						pfmt(stderr, MM_ERROR, badopen,
							mnttab, strerror(errno));
						exit(2);
					}
					mntnull(&mountb);
					i = stat(argv[optind], &S);
					if (i == -1) {
						pfmt(stderr, MM_ERROR, badstat,
							argv[optind], strerror(errno));
						exitcode=1;
		 				continue;
					}

					while (getmntent(fp,&mountb) == 0) {
						i=stat(mountb.mnt_mountp,&Sbuf);
						if ( i<0 )  {
							pfmt(stderr, MM_ERROR, badstat,
								argv[optind], strerror(errno));
							exitcode=1;
		 					continue;
						
						}
						if (S.st_dev == Sbuf.st_dev) {
							notfound=0;
							break;
						}
					}
					/* argument may be a path with a mounted
					   resource under it. ie, arg=/mnt/var,
					   where /mnt is mounted via rfs and /var
					   is a mounted file system.  In this case
					   stat will not return the same device
					   numbers so a match will never be found.
					   Since we have the info from statvfs print
					   it anyway.
					*/

					if (notfound) {
						mountb.mnt_mountp=argv[optind];
						if (res_found) {
							mountb.mnt_special=res_name; 
							res_found=0;
						}
						else {
							mountb.mnt_special="***************"; 
						}
					}
				}
			}

		mountb.mnt_fstype  = statbuf.f_basetype;
		if ((l_flg) && (is_remote(mountb.mnt_fstype))) 
			continue;
		if (g_flg) {
			print_statvfs(&mountb, &statbuf, 'g');
			continue;
		}
		if (k_flg) {
			print_statvfs(&mountb, &statbuf, 'k');
			continue;
		}

		/* i_flg related code below is part of the 
		   Enhanced Application Compatibility Support */
		if (v_flg && !i_flg) {
			print_statvfs(&mountb, &statbuf, 'v');
			continue;
		}
		else if (v_flg && i_flg) {
			print_statvfs(&mountb, &statbuf, 'vi');
			continue;
		}
		else if (!v_flg && i_flg) {
			print_statvfs(&mountb, &statbuf, 'i');
			continue;
		}
		/* End Enhanced Application Compatibility Support */

		if (t_flg) {
			print_statvfs(&mountb, &statbuf, 't');
			continue;
		}
		if (b_flg) 
			print_statvfs(&mountb, &statbuf, 'b');
		if (e_flg) 
			print_statvfs(&mountb, &statbuf, 'e');
		if (n_flg) 
			print_statvfs(&mountb, &statbuf,'n');
		if (b_flg || n_flg || e_flg) 
			continue;
		print_statvfs(&mountb, &statbuf, 'x');

	}	/* end: for all arguments */
	exit(exitcode);
}	/* end main */




/*
 * Procedure:     echo_cmdline
 *
 * Restrictions:
                 printf: none

*/
echo_cmdline(argp, argpc, fstype)
char *argp[];
int argpc;
char *fstype;
{
	int i;
	printf("%s", argp[0]);
	if (fstype != NULL )	
		printf(" -F %s", fstype);
	for( i= 1; i < argpc; i++) 
	        printf(" %s", argp[i]);
	printf("\n");

}

/*
 * Procedure:     mnterror
 *
 * Restrictions:
                 pfmt: None
*/
mnterror(flag)
	int	flag;
{
	switch (flag) {
	case MNT_TOOLONG:
		pfmt(stderr, MM_ERROR, ":259:Line in mnttab exceeds %d characters\n",
			MNT_LINE_MAX-2);
		break;
	case MNT_TOOFEW:
		pfmt(stderr, MM_ERROR, ":260:Line in mnttab has too few entries\n");
		break;
	case MNT_TOOMANY:
		pfmt(stderr, MM_ERROR, ":261:Line in mnttab has too many entries\n");
		break;
	}
}

/* function to generate command line to be passed to specific */

mk_cmdline(fstype, options, o_flg, oargs, argument)
char *fstype;
char *options;
int  o_flg;
char *oargs;
char *argument;
{


	argpc = 0;
	argp[argpc++] = "df";
	if (strcmp(options, "-") != 0)
		argp[argpc++] = options;
	if (o_flg) {
		argp[argpc++] = "-o";
		argp[argpc++] = oargs;
	}
	argp[argpc++] = argument;
	argp[argpc] = NULL;
}


/*
 * Procedure:     print_statvfs
 *
 * Restrictions:
                 pfmt: none
*/

print_statvfs(mountb, statbuf, flag)
struct mnttab *mountb;
struct statvfs *statbuf;
int flag;
{
	int 	physblks,
		TotalBlocks,
		UsedBlocks,
		AvailBlocks,
		Capacity;

/* Enhanced Application Compatibility Support */
	int TotalInodes=0, FreeInodes=0, UsedInodes=0;
/* End Enhanced Application Compatibility Support */

	physblks=statbuf->f_frsize/512;
        TotalBlocks = statbuf->f_blocks*physblks;
        AvailBlocks = statbuf->f_bavail*physblks;
        UsedBlocks = TotalBlocks - AvailBlocks;
        Capacity = TotalBlocks ? 100 * (double) UsedBlocks / (double) TotalBlocks + 0.5 : 0;


	switch(flag) {

        case 'v':
		
		pfmt(stdout, MM_NOSTD,
			":801:%-10.10s %-17.17s %9ld %9ld %9ld %4d%%\n",
                        mountb->mnt_mountp,
                        mountb->mnt_special,
                        TotalBlocks,
                        UsedBlocks,
                        AvailBlocks,
			Capacity);
                break;

		/* Enhanced Application Compatibility Support */
		case 'vi':
			TotalInodes = statbuf->f_files;
			FreeInodes = statbuf->f_ffree;
			UsedInodes = statbuf->f_files - statbuf->f_ffree;
			pfmt(stdout, MM_NOSTD,
				":1125:%-8.8s %-17.17s %8ld %8ld %8ld %3d%% %6ld %6ld %3d%%\n",
				mountb->mnt_mountp, 
				mountb->mnt_special,
				TotalBlocks,
				UsedBlocks,
				AvailBlocks,
				Capacity,
				UsedInodes,
				FreeInodes,
				TotalInodes ? (100 * UsedInodes / TotalInodes) : 0);

			break;
	
		case 'i':
			TotalInodes = statbuf->f_files;
			FreeInodes = statbuf->f_ffree;
			UsedInodes = statbuf->f_files - statbuf->f_ffree;
	
			pfmt(stdout, MM_NOSTD,
				":801:%-10.10s %-17.17s %9ld %9ld %9ld %4d%%\n",
				mountb->mnt_mountp, 
				mountb->mnt_special,
				UsedInodes,
				FreeInodes,
				TotalInodes,
				TotalInodes ? (100 * UsedInodes / TotalInodes) : 0);
			break;
	/* End Enhanced Application Compatibility Support */

	case 'g':
		pfmt(stdout, MM_NOSTD,
			":262:%-18s(%-15s):  %6u block size  %7u frag size\n", 
			mountb->mnt_mountp, 
			mountb->mnt_special,
			statbuf->f_bsize, 
			statbuf->f_frsize);
		pfmt(stdout, MM_NOSTD,
			":802:%7u total blocks%7u free blocks%7u available   %7d total files\n", 
			statbuf->f_blocks*physblks, 
			statbuf->f_bfree*physblks,
			statbuf->f_bavail < statbuf->f_blocks ? statbuf->f_bavail*physblks : 0,
			statbuf->f_files);
		pfmt(stdout, MM_NOSTD,
			":803:%7d free files  %7u filesys id %32s \n", 
			statbuf->f_ffree,
			statbuf->f_fsid,
			statbuf->f_fstr);
		pfmt(stdout, MM_NOSTD,
			":804:%7s fstype   0x%8.8X flag       %7d filename length\n\n", 
			statbuf->f_basetype,
			statbuf->f_flag,
			statbuf->f_namemax);
		break;

	case 't':
		pfmt(stdout, MM_NOSTD,
			":266:%-19s(%-16s):  %8d blocks%8d files\n", 
			mountb->mnt_mountp, 
			mountb->mnt_special,
			AvailBlocks, 
			statbuf->f_ffree);
		pfmt(stdout, MM_NOSTD,
			":267:                                total:\t%8d blocks%8d files\n",
			TotalBlocks,
			statbuf->f_files);
		break;

	case 'b':
		if (eb_flg) {
			pfmt(stdout, MM_NOSTD,
				":268:%-19s(%-16s):  %8d kilobytes\n", 
				mountb->mnt_mountp, 
				mountb->mnt_special,
				((AvailBlocks*512)/1024));
		}
		else {
			if (!header) {
				pfmt(stdout, MM_NOSTD, ":269:Filesystem             avail\n");
				header++;
			}
			pfmt(stdout, MM_NOSTD, ":270:%-16s       %-8d\n", 
				mountb->mnt_special,
				((AvailBlocks*512)/1024));
		}
		break;

	case 'e':
		if (eb_flg) {
			pfmt(stdout, MM_NOSTD,
				":271:%-19s(%-16s):  %8d files\n", 
				mountb->mnt_mountp, 
				mountb->mnt_special,
				statbuf->f_ffree);
		}
		else {
			if (!header) {
				pfmt(stdout, MM_NOSTD, ":272:Filesystem             ifree\n");
				header++;
			}
			pfmt(stdout, MM_NOSTD,
				":270:%-16s       %-8d\n", 
				mountb->mnt_special, statbuf->f_ffree);
		}
		break;

	case 'n':
		pfmt(stdout, MM_NOSTD,
			":273:%-19s: %-10s\n",
			mountb->mnt_mountp,
			statbuf->f_basetype);
		break;
	case 'k':
		if (!k_header) {
			pfmt(stdout, MM_NOSTD,
				":274:filesystem         kbytes   used     avail    capacity  mounted on\n");
			k_header = 1;
		} 
		pfmt(stdout, MM_NOSTD,
			":1126:%-18s %-8lu %-8lu %-8lu %2lu%%       %-19s\n",
			mountb->mnt_special,
			TotalBlocks*512/1024,
			UsedBlocks*512/1024,
			AvailBlocks*512/1024,
			Capacity,
			mountb->mnt_mountp);
		break;

        default:	
		pfmt(stdout, MM_NOSTD,
			":276:%-19s(%-16s):%8d blocks%8d files\n", 
			mountb->mnt_mountp, 
			mountb->mnt_special,
			AvailBlocks,
			statbuf->f_ffree);
		break;
	}
}

int
build_path(FSType, path)
char *FSType;
char *path;
{

	strcpy(path, "/usr/lib/fs/");
	strcat(path, FSType );
	strcat(path, "/df");
 	return 0;	
}


/*
 * Procedure:     exec_specific
 *
 * Restrictions:
                 pfmt: None
                 strerror: None
                 execvp(2): P_MACREAD
*/

exec_specific(FSType)
char *FSType;
{
int  pid,c_ret;
	switch(pid = fork()) {
	case (pid_t)-1:
		pfmt(stderr, MM_ERROR, ":277:fork() failed: %s\n",
			strerror(errno));
		exit(2);

	case 0:	
		if (mac_install && level)
			procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
		if (execvp(path, argp) == -1) {
			if (errno == EACCES) {
				pfmt(stderr, MM_ERROR, ":278:Cannot execute %s: %s\n",
					path, strerror(errno));
				exit(2);
			}
			pfmt(stderr, MM_ERROR, ":279:Operation not applicable for FSType %s\n", FSType);
			exit(2);
		}
		exit(2);
		
	default:
		if (wait(&status) == pid) {
			if ((c_ret=WHIBYTE(status)) != 0){
				exit(c_ret);
			}
		}
	} 
}

/* code used by devnm */
char *
basename(s)
char *s;
{
	int n = 0;

	while(*s++ != '\0') n++;
	while(n-- > 0)
		if(*(--s) == '/') 
			return(++s);
	return(--s);
}

struct dirent *dbufp;

/*
 * Procedure:     devnm
 *
 * Restrictions:
                 opendir: none
                 chdir(2): none
								 getcwd: none
*/
char *
devnm()
{
	int i,j;
	static dev_t fno;
	static struct devs {
		char *devdir;
		DIR *dfd;
	} devd[] = {		/* in order of desired search */
		"/dev/dsk",0,
		"/dev",0,
		"/dev/rdsk",0
	};
	static char devnam[DEVLEN];
	static char cwd[MAXPATHLEN];

	devnam[0] = '\0';
	if(!devd[1].dfd) {	/* if /dev isn't open, nothing happens */
		for(i = 0; i < 3; i++) {
			devd[i].dfd = opendir(devd[i].devdir);
		}
	}
	fno = S.st_dev;

	(void)getcwd(cwd, MAXPATHLEN - 1);
	for(i = 0; i < 3; i++) {
		   j=chdir(devd[i].devdir);
		   if ((j == 0) && (dsearch(devd[i].dfd,fno))) {
			strcpy(devnam, devd[i].devdir);
			strcat(devnam,"/");
			strncat(devnam,dbufp->d_name,MAXNAMELEN);
			(void)chdir(cwd);
			return(devnam);
		}
	}
	(void)chdir(cwd);
	return(devnam);

}


/*
 * Procedure:     dsearch
 *
 * Restrictions:
                 stat(2): none
                 pfmt: none
                 strerror: none
*/
dsearch(ddir,fno)
DIR *ddir;
dev_t fno;
{
	int i;

	rewinddir(ddir);
	while((dbufp=readdir(ddir)) != (struct dirent *)NULL) {
		if(!dbufp->d_ino) continue;
		i=stat(dbufp->d_name, &S);
		if(i == -1) {
			pfmt(stderr, MM_ERROR, badstat,dbufp->d_name,strerror(errno));
			return(0);
		}
		if((fno != S.st_rdev) 
		|| ((S.st_mode & S_IFMT) != S_IFBLK)
		|| (strcmp(dbufp->d_name,"swap") == 0)
		|| (strcmp(dbufp->d_name,"pipe") == 0)
			) continue;
		return(1);
	}
	return(0);
}
int
is_remote(fstype)
	char	*fstype;
{
	if(strcmp(fstype, "rfs") == 0) 
		return 1;
	if(strcmp(fstype, "nfs") == 0) 
		return 1;
	return 0;
}
