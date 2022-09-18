/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)face:src/filecab/fileb/listserv.c	1.8.4.4"
#ident  "$Header: listserv.c 1.6 91/11/18 $"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include "wish.h"

#define BUFF  20

typedef struct {
	char *serv_name;
	char *serv_lininfo;
	char *serv_action;
} SERVSTRUCT;


main(argc,argv)
int argc;
char *argv[];
{
	SERVSTRUCT serv_vec[BUFF];
	FILE *fp;
	char *home, *getenv(), *label, *name, *penv, *fname, *n_name;
	char tpath[PATHSIZ], hpath[PATHSIZ], buf[BUFSIZ], path[PATHSIZ], *opt;
	int flag=0, cond=0, dos=0, all_flag=0;
	int its_open;
	int i=0, j=0;
	int nam_len=0;
	char *getmsg();
	int compare_it();
	int app_type();

	setlocale(LC_ALL,"");
	penv=argv[argc-1];
	while(--argc > 0 && (*++argv)[0] == '-')
		for(opt=argv[0]+1; *opt != '\0'; opt++)
		switch(*opt) {
			case 'a':
				all_flag=1;
				break;
			case 'd':
				flag=1;
				break;
			case 'l': /* used to create the rmenu */
				flag=2;
				break;
			case 'm':
				flag=3;
				break;
			case 'p':
				dos=1;
				break;
			default:
				break;
		}
	home=getenv(penv);

	if (strcmp(penv,"HOME") == 0) {
		sprintf(hpath, "%s/pref/services",home);
		sprintf(tpath,"$VMSYS/OBJECTS/%s",dos?"dos":"programs");
	}
	else {
		sprintf(hpath, "%s/lib/services",home);
		sprintf(tpath,"$OBJ_DIR");
	}

	if ((fp=fopen(hpath,"r")) == NULL) {
		printf(gettxt("uxface:55", "init=`message No Programs Installed`false\n"));
		exit(FAIL);
	}

	while(fp && (fgets(buf,BUFSIZ,fp) != NULL)) {
		if (*buf == '\n' || *buf == '#' )
			continue;

		label = strtok(buf,"=");

		if (! strcmp(label,"name")) {
			name=strtok(NULL,"\n");
			sprintf(path,"%s/bin/%s.ins",home,name);
		} else if (! strcmp(label,"`echo 'name")) {
			name=strtok(NULL,"'");
			fname=strtok(NULL,"=");
			fname=strtok(NULL,"$");
			if (! strncmp(fname,"OPEN",4)) {
				if (all_flag == 0) continue;
				its_open = 1;
				sprintf(path,"%s",fname);
				fname = strrchr(path,'\'');
				if (fname) *fname=0;
			} else {
			  its_open = 0;
			  fname=strtok(NULL,"`");
			  sprintf(path,"%s%s",home,&fname[strlen(penv)]);
			}
		} else
			continue;

		n_name = getmsg(name);
#ifdef DEBUG
fprintf(stderr,"Return value of getmsg: %s\n",n_name );
#endif
		  nam_len = strlen(n_name);
		  serv_vec[i].serv_lininfo = serv_vec[i].serv_action = NULL;
		  if (its_open || (access(path,00)==0 && app_type(path,dos)) ) {
			cond=1;
			if (flag == 2)  {
				serv_vec[i].serv_name = (char *) malloc( nam_len + 1 );
				strcpy(serv_vec[i].serv_name, n_name);
				i++;
				continue;
			}
			serv_vec[i].serv_name = (char *) malloc( strlen("name=") + nam_len +1 );
			sprintf(serv_vec[i].serv_name,"name=%s",n_name);
#ifdef DEBUG
fprintf( stderr,"serv_vec[%d].serv_name: %s\n", i, serv_vec[i].serv_name );
#endif
			if (!its_open) { 
                            serv_vec[i].serv_lininfo = (char *) malloc( strlen(path) + strlen("lininfo=\"\"") + 1 );
                            sprintf(serv_vec[i].serv_lininfo,"lininfo=\"%s\"",path);
			}
			if (flag == 1 ) {

                            serv_vec[i].serv_action = (char *) malloc( strlen("action=OPEN TEXT /Text.conf  \"$LININFO\" \"\" `getfrm`") + strlen(tpath) + strlen(n_name) + strlen(penv) + 1 );

				sprintf(serv_vec[i].serv_action,"action=OPEN TEXT %s/Text.conf %s \"$LININFO\" \"%s\" `getfrm`",tpath,n_name,penv);
			}
			else if (flag == 3 ) {

                            serv_vec[i].serv_action = (char *) malloc( strlen("action=OPEN FORM /Form.mod  \"$LININFO\" \"\" `getfrm`") + strlen(tpath) + strlen(n_name) + strlen(penv) + 1 );

				sprintf(serv_vec[i].serv_action,"action=OPEN FORM %s/Form.mod %s \"$LININFO\" \"%s\" `getfrm`",tpath,n_name,penv);
			}
			else if (its_open) {

                                serv_vec[i].serv_action = (char *) malloc( strlen(path) + strlen("action=") + 1 );
				sprintf(serv_vec[i].serv_action,"action=%s",path);
			}
			else {
                                serv_vec[i].serv_action = (char *) malloc( strlen(path) + 20 );
				sprintf(serv_vec[i].serv_action,"action=`run %s%s`nop",dos?"-n ":"",path);
#ifdef DEBUG
fprintf(stderr,"bis hier\n");
#endif
			}
		}
                i++;
#ifdef DEBUG
fprintf(stderr,"i=%d\n",i);
#endif
	}
	if (!cond) {
		if ( dos )
			printf(gettxt("uxface:54", "init=`message No MS-DOS Programs Installed`false\n"));
		else
			printf(gettxt("uxface:55", "init=`message No Programs Installed`false\n"));
		exit(FAIL);
	}
	qsort(serv_vec,i,sizeof(SERVSTRUCT),compare_it);
        for(j=0; j < i; j++) {
		printf("%s\n",serv_vec[j].serv_name);
		if( serv_vec[j].serv_lininfo != NULL)
			printf("%s\n",serv_vec[j].serv_lininfo);
		if (serv_vec[j].serv_action != NULL)
		printf("%s\n",serv_vec[j].serv_action);
	}

	exit(SUCCESS);
}

int
compare_it(SERVSTRUCT *struct1, SERVSTRUCT *struct2)

{
   int x;
   x=strcoll(struct1->serv_name,struct2->serv_name);
   if(x < 0)
      return(-1);
   else if(x == 0)
      return( strcmp(struct1->serv_name,struct2->serv_name) );
   else
      return(1);
}

char *
getmsg(char *doll_name)
{
   static char msgcat[128];
   char *msgid, *defmsg, *doll_name1;

   doll_name1 = doll_name;

   if (*doll_name1 == '"' )
	doll_name1++;

   if (*doll_name1 != '$' || *(doll_name1 + 1) != '$' )
	return(doll_name);

   doll_name1 += 2;
   msgid = strtok(doll_name1, ":");
   if (!msgid) return(doll_name);
   strcpy(msgcat, msgid);
   msgid = strtok(NULL, ":");
   if (!msgid) return(doll_name);
   strcat(msgcat,":");
   strcat(msgcat, msgid);
   defmsg = strtok(NULL,"\"");
   if (defmsg == NULL) defmsg = "";
#ifdef DEBUG
fprintf(stderr,"%s %s\n",msgcat, defmsg);
#endif
   msgid = gettxt(msgcat, defmsg);
   msgcat[0] = '"';
   strcpy(msgcat+1, msgid);
   strcat(msgcat+1, "\"");
#ifdef DEBUG
fprintf(stderr,"%s\n", msgcat);
#endif
   return(msgcat);
}

int
app_type(path,dos)
char *path;
int dos;
{
	FILE *fp;
	char buf[BUFSIZ];
	int retval;

	retval = dos?FALSE:TRUE;

	if ((fp=fopen(path,"r")) == NULL)
		return(retval);

	while(fp && (fgets(buf,BUFSIZ,fp) != NULL)) {
		if ( *buf != '#' )
			continue;

		if (! strcmp(buf,"#dos\n")) {
			retval = dos?TRUE:FALSE;
			break;
		}

		if (! strcmp(buf,"#unix\n")) {
			retval = dos?FALSE:TRUE;
			break;
		}
	}

	(void)fclose(fp);

	return(retval);
}

