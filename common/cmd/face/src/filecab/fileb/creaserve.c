/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)face:src/filecab/fileb/creaserve.c	1.9.5.4"
#ident "$Header: creaserve.c 1.16 91/11/22 $"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <locale.h>
/* you can make a non-es-version defininf -DNO_ES_VERSION -tf- */
#ifndef NO_ES_VERSION
#include <priv.h>
#include <mac.h>
#include <errno.h>
#endif
#include "wish.h"

main(argc,argv)
int argc;
char *argv[];
{
	FILE *fp, *tp;
	char *home, *getenv();
	char *prompt, *term, *mname, *appath, *dir, *penv, *npath;
	/*** I18n sni-ar 181191 ***/
        char *new_string;
        char *old_string;
	char path[PATHSIZ], fpath[PATHSIZ], temp_name[PATHSIZ];
	char io_buf[BUFSIZ], new_line[BUFSIZ];
	extern char *tempnam();
	extern pid_t getpid();
	extern uid_t getuid();
	extern gid_t getgid();
	int exist, comp_len, i, written_yet, dos=0;
	uid_t uid;
	gid_t gid;
	struct passwd *pw;
#ifndef NO_ES_VERSION
	level_t set_val; /* For setting the level for global progs */
	int	mac_absent=0;
	level_t	test_lid ;
#endif
	setlocale(LC_ALL,"");
	if (argc < 7) {
		fprintf(stderr,gettxt("uxface:4", "Arguments invalid\n"));
		exit(FAIL);
	}

	/* Initialize arguments needed to create installation script */
	term=argv[1];
	mname=argv[2];
	appath=argv[3];
	dir=argv[4];
	prompt=argv[5];
	penv=argv[6];
	if ( argc == 8 )
		dos = 1;

	home=getenv(penv);
	sprintf(fpath, "%s/bin", home);
	if ( (npath = tempnam(fpath,"ins.")) == NULL ) {
		fprintf(stderr,gettxt("uxface:13", "Cannot create install file\n"));
		exit(FAIL);
	}

	if ((fp=fopen(npath,"w")) == NULL) {
		fprintf(stderr,gettxt("uxface:16", "Cannot open install file\n"));
		exit(FAIL);
	}

	if (strcmp(penv, "HOME") == 0) {
		uid = getuid();
		gid = getgid();
	} else {
		pw = getpwnam("vmsys");
		uid = pw->pw_uid;
		gid = pw->pw_gid;
	}

	/* Create the Shell script the application is going to use */
	fprintf(fp,"#%s\n",dos?"dos":gettxt("uxface:60", "unix"));
	fprintf(fp,"#TERM=%s;export TERM\n",term);
	fprintf(fp,"cd %s\n",dir);
	if (strcmp(prompt, gettxt("uxface:61", "yes")) == 0) {
		fprintf(fp,gettxt("uxface:52", "echo \"Enter the arguments for %s: \\c\";"),appath);
		fprintf(fp,"read FACE_ARGS\n");
		fprintf(fp,"eval %s%s $FACE_ARGS\n",dos?"dos -c ":"",appath);
	} else {
		fprintf(fp,"%s%s\n",dos?"dos -c ":"",appath);
	}
	fclose(fp);
	chmod(npath, 0755);
	chown(npath, uid, gid);
#ifndef NO_ES_VERSION
	if ((lvlproc(MAC_GET, &test_lid) !=0) && (errno == ENOPKG))
		mac_absent = 1; /* MAC not installed */
	if ((strcmp(penv, "HOME") != 0) && (mac_absent == 0))/* Global prog   */
        {
		set_val=1; /* SYS_PUBLIC =1 */
		lvlfile(npath, MAC_SET, &set_val);
	}
#endif


	/* Update the User's service file */
	if (strcmp(penv, "HOME") == 0)
		sprintf(path, "%s/pref/services",home);
	else
		sprintf(path, "%s/lib/services",home);

	exist = access(path, 00) ? 0 : 1;

	sprintf(temp_name, "/tmp/ins.%ld", getpid());

	if ((tp=fopen(temp_name,"w+")) == NULL) {
		fprintf(stderr, gettxt("uxface:18", "Cannot open temporary file\n"));
		exit(FAIL);
	}

	if ((fp=fopen(path,exist ? "r+" : "w+")) == NULL) {
		fprintf(stderr, gettxt("uxface:17", "Cannot open services file\n"));
		fclose(tp);
		unlink(temp_name);
		exit(FAIL);
	}
/*
 *  copy current services file to a temp file if it exists
 */
	if ( exist )
		while (fgets(io_buf, sizeof(io_buf), fp) != NULL)
			fputs(io_buf, tp);
	else
		fprintf(tp,"#3B2-4I1\n");
	rewind(fp);
	rewind(tp);

	sprintf(new_line,"`echo 'name=\"%s\"';echo 'action=`run %s$%s%s`nop'`\n",mname,dos?"-n ":"",penv,&npath[strlen(home)]);
	comp_len = strcspn(new_line,";");
	written_yet = 0;
	old_string=malloc(comp_len+1);
	new_string=malloc(comp_len+1);
	strncpy(new_string,new_line,comp_len);
	new_string[comp_len] = '\0';

	while (fgets(io_buf, sizeof(io_buf), tp) != NULL) {
                
                strncpy(old_string,io_buf,comp_len);
		old_string[comp_len] = '\0';
		i=strcoll(new_string,old_string);
		if (i==0)
		   i = strcmp(new_string,old_string);

		if ( ! written_yet && i <= 0 ) {
			written_yet++;
			fputs(new_line, fp);
			fputs(io_buf, fp);
		} else
			fputs(io_buf, fp);
	}

	if ( ! written_yet )
		fputs(new_line, fp);

	fclose(fp);
	fclose(tp);
	chmod(path, 0644);
	chown(path, uid, gid);

#ifndef NO_ES_VERSION
	if ((strcmp(penv, "HOME") != 0) && (mac_absent == 0)) {  /* Global prog */
		set_val=1; /* SYS_PUBLIC =1 */
		lvlfile(path, MAC_SET, &set_val);
	}
#endif
	unlink(temp_name);
	exit(SUCCESS);
}
