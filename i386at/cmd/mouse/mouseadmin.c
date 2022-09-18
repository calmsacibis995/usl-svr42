/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mouse:mouseadmin.c	1.3.2.9"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <curses.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/stream.h>
#include <sys/sysmacros.h>
#include <sys/mouse.h>
#include <sys/wait.h>
#include <sys/mse.h>

#define CFG_NAME	"/dev/mousecfg"
#define MOUSETAB	"/usr/lib/mousetab"
#define MSEERR		1
#define MSENOERR	0


#define MAX_DEV		100
#define MAXDEVNAME	64
int	row, col;
char ans[256];
int	c;
char errstr[80];
char sterminal[256], smouse[256];
char	fname[MAXDEVNAME];
char	msebusy[MAX_MSE_UNIT+1];
int	listing=0, deleting=0, adding=0, testing=0;
int	no_download=0, bus_okay=0;
int	cfg_fd;
int	suserflg=0;
int	setpgrpflg=0;

struct mousemap	map[MAX_DEV];
struct {
	char	disp[MAXDEVNAME];
	char	mouse[MAXDEVNAME];
} table[MAX_DEV];

int	n_dev;

int	cursing;

int	(*print)() = printf;

void load_table(), download_table(), show_table();
void interact();
int delete_entry(), add_entry(), test_entry();
int ckconfg(), config_mod();

void
fatal_error(fname)
char	*fname;
{
	if (cursing) {
		int	save_err;

		save_err = errno;
		endwin();
		errno = save_err;
	}

	perror(fname);
	exit(1);
}

void
_fatal_error(msg)
char	*msg;
{
	if (cursing)
		endwin();
	fprintf(stderr, "\n%s.\n\n", msg);
	exit(1);
}

void
enter_prompt()
{
	char	ch;

	row += 2;
	while (1) {
		mvaddstr(row, col, "Strike the ENTER key to continue.");
		refresh();
		ch = getchar();
		if (ch == '\n' || ch == '\r')
			break;
		else
			beep();
	}
	row++;
}

void
warn_err(msg)
char	*msg;
{

	row+=2;
	beep();
	mvaddstr(row,col,msg);
	enter_prompt();
	return;
}

get_info(strp, retp)
char *strp, *retp;
{

	row++;
	mvaddstr(row, col, strp);
	refresh();
	attron(A_BOLD);
	getstr(retp);
	attroff(A_BOLD);
}

void
main(argc, argv)
	char	*argv[];
{
	int	c, usage = 0, retval, irq = 0, interrupt = 0;
	extern int	optind;
	extern char	*optarg;
	char *device;

	device = (char *) NULL;

	while ((c = getopt(argc, argv, "h:ld:a:ntbi:")) != EOF) {
		switch (c) {

		case 'h':
			if (strcmp(optarg,"idden") == 0)
				setpgrpflg++;
			break;
		case 'l':
			listing++;
			break;
		case 'd':
			deleting++;
			device=(char *)(malloc(strlen(optarg)));
			strcpy(device, optarg);
			break;
		case 'a':
			adding++;
			device=(char *)(malloc(strlen(optarg)));
			strcpy(device, optarg);
			break;
		case 'n':
			no_download++;
			break;
		case 'b':
			++bus_okay;
			break;
		case 't':
			++testing;
			break;
		case 'i':
		{
			char *cmd;
			cmd=(char *)(malloc(1024));
			if (cmd == (char *) NULL)
			interrupt++;
			irq=atoi(optarg);
			if ( irq == 2 )
				irq = 9;
			sprintf(cmd,"FOO=`/etc/conf/bin/idcheck -r -v %d`\n[ \"$FOO\" = \"bmse\" ] && exit 0\n[ \"$FOO\" = \"\" ] && exit 0\nexit 1",irq);
			if ( system(cmd) != 0 ) {
				fprintf(stderr,"Interrupt %d is already in use.\n",irq);
				free(cmd);
				exit(1);
			}
			free(cmd);
			break;
		}
		default:
			usage++;
		}
	}
	switch (testing + deleting + adding + listing) {
	case 0:
		if (argc - optind != 0)
			usage++;
		break;
	case 1:
		if (deleting && argc - optind != 0)
			usage++;
		else if (adding && argc - optind != 1)
			usage++;
		else if (listing && argc - optind != 0)
			usage++;
		else if (testing && argc - optind != 0)
			usage++;
		break;
	default:
		usage++;
		break;
	}
	if (usage) {
		fprintf(stderr,
"Usage:  mouseadmin { -n | -b | -l | -i irq | -d terminal | -a terminal mouse }\n");
		exit(1);
	}

	load_table();
	if(!no_download && !testing)
		get_mse_opened();

	if ((device!=(char *)NULL) && (strcmp(device, "console") == 0))
		strcpy(device, "vt00");

	if((optind<argc) && (strcmp(argv[optind], "320") == 0))
		strcpy(argv[optind], "m320");

	if((optind<argc) && (strcmp(argv[optind], "PS2") == 0))
		strcpy(argv[optind], "m320");


	if((optind<argc) && (strcmp(argv[optind], "BUS") == 0 ||strcmp(argv[optind],"Bus")== 0 || strcmp(argv[optind], "bus")== 0))
		strcpy(argv[optind], "bmse");

	if ((optind<argc)&& (strcmp(argv[optind], "bmse") && interrupt)) {
		fprintf(stderr,"-i option only valid with Bus mouse\n");
		fprintf(stderr,
"Usage:  mouseadmin { -n | -b | -l |-i irq | -d terminal | -a terminal mouse }\n");
		exit(1);
	}

	if (listing)
		show_table();
	else if (testing) {
		exit (test_entry());
	}
	else if (deleting) {
		if ((retval = delete_entry(device)) < 0) {
			if(retval == -1)
				fprintf(stderr,
"\nThere is no mouse assigned for %s.\n", device);
			else if(retval == -2)
				fprintf(stderr,"\nThe mouse on %s is currently busy.\n",device);
			else if(retval == -3)
				fprintf(stderr, "\n%s is not a valid display terminal.\n", device);
			exit(1);
		}
		download_table();
	} else if (adding) {
		char *module;
		int rc = 0;
		if (strcmp(argv[optind],"bmse") && strcmp(argv[optind],"m320"))
			module="smse";
		else
			module=argv[optind];
		rc=ckconfg(module);
		if (rc == 2) {
			fprintf(stderr,"%s mouse module not configured.\n",module);
			exit(1);
		}
		if ( rc == 1 || !strcmp("bmse", module) )
			config_mod(module, irq);
		if (add_entry(device, argv[optind], 0)){
			exit(1);
		}
		download_table();
	} else
		interact();

	exit(0);
}


int
get_dev(name, dev_p)
char	*name;
dev_t	*dev_p;
{
	struct stat	statb;

	if (strncmp(name, "/dev/", 5) == 0) {
		strcpy(fname, name);
		strcpy(name, name + 5);
	} else {
		strcpy(fname, "/dev/");
		strcat(fname, name);
	}

	if (stat(fname, &statb) == -1)
		return -1;
	if ((statb.st_mode & S_IFMT) != S_IFCHR)
		return -2;

	*dev_p = statb.st_rdev;
	return 0;
}


void
load_table()
{
	FILE	*tabf;
	char	dname[MAXDEVNAME], mname[MAXDEVNAME];
	struct stat	statb;

	if ((tabf = fopen(MOUSETAB, "r")) == NULL)
		return;

	/* Format is:
	 *	disp_name  mouse_name
	 */

	n_dev = 0;
	while (fscanf(tabf, "%s %s", dname, mname) > 0) {
		if (get_dev(dname, &map[n_dev].disp_dev) < 0){
			continue;
		}
#ifdef DEBUG
fprintf(stderr,"load_table: mouse = %s\n",mname);
#endif
		if (strncmp(mname, "m320", 4) == 0){
			map[n_dev].type = M320;
		}else 
		if (strncmp(mname, "bmse", 4) == 0){
			map[n_dev].type = MBUS;
		}else  {
			map[n_dev].type = MSERIAL;
		}
		if (get_dev(mname, &map[n_dev].mse_dev) < 0){
			continue;
		}
		strcat(table[n_dev].disp, dname);
		strcat(table[n_dev++].mouse, mname);
	}

	fclose(tabf);
}


void
write_table()
{
	FILE	*tabf;
	int	i;

	if ((tabf = fopen(MOUSETAB, "w")) == NULL)
		fatal_error(MOUSETAB);
	chmod(MOUSETAB, 0644);

	for (i = 0; i < n_dev; i++)
		fprintf(tabf, "%s\t\t%s\n", table[i].disp, table[i].mouse);

	fclose(tabf);
}

get_mse_opened()
{
	int i;

	if(getuid() != 0)
		suserflg = 1;
	if ((cfg_fd = open(CFG_NAME, O_WRONLY)) < 0)
		fatal_error(CFG_NAME);
	if (ioctl(cfg_fd, MOUSEISOPEN, msebusy) < 0) 
		fatal_error(CFG_NAME);
	close(cfg_fd);
}

void
download_table()
{
	struct mse_cfg	mse_cfg;

	if (!no_download) {
		/* Tell the driver about the change */
		if(suserflg)
			fatal_error(CFG_NAME);
		if ((cfg_fd = open(CFG_NAME, O_WRONLY)) < 0)
			fatal_error(CFG_NAME);

		mse_cfg.mapping = map;
		mse_cfg.count = n_dev;
		if (ioctl(cfg_fd, MOUSEIOCCONFIG, &mse_cfg) < 0) {
			if (errno == EBUSY) {
				_fatal_error(
"One or more mice are in use.\nTry again later");
			}
			fatal_error(CFG_NAME);
		}

		close(cfg_fd);
	}

	/* Write the new table out to the mapping file */
	write_table();
}


void
show_table()
{
	int	i;

	if (n_dev == 0) {
		(*print)("\nThere are no mice assigned.\n\n");
		return;
	}

	(*print)("\nThe following terminals have mice assigned:\n\n");
	(*print)("Display terminal      Mouse device\n");
	(*print)("----------------      ------------\n");

	for (i = 0; i < n_dev; i++) {
		if(strcmp(table[i].disp, "vt00") == 0)
			(*print)("%-22s", "console");
		else
			(*print)("%-22s", table[i].disp);
		if(strncmp(table[i].mouse,"bmse", 4) == 0)
			(*print)("Bus mouse\n");
		else
		if(strncmp(table[i].mouse,"m320", 4) == 0)
			(*print)("PS2 mouse\n");
		else
			(*print)("Serial mouse on %s\n", table[i].mouse);
	}

	(*print)("\n");
}


int
lookup_disp(disp)
char	*disp;
{
	int	slot;

	for (slot = 0; slot < n_dev; slot++) {
		if (strcmp(disp, table[slot].disp) == 0)
			return slot;
	}
	return -1;
}

int childpid=-1;
int childpid2=-1;

OnSigTerm()
{
 	mvprintw(0,0,""); 
	clear();
	refresh();
	/* only endwin() if not invoked via 'T' option to mouseadmin menu */
	if (testing)
		endwin();
	exit(5);
}

int
test_entry()
{
	int cnt;
	int xscale = 10;
	int yscale = 10;
	int disp;
	int waitflag, waitflag2;
	int mouse_is_on = 0;

	int mousefd, x, y, sx, sy, old_sx, old_sy, sleep_time;
	struct mouseinfo m;
	int buttoncnt=0; /* keep track of how many button state changes we
			    have seen in test -- we need to see 2 to both
			    see successful button change and not leave
			    button state in state such that retry appears
			    to see button change even though the user
			    didn't see button change.
			  */
	
	for (cnt=0;cnt < 10; cnt++)
		switch (childpid=fork()) {

		  case -1: { /* retry up to 10 times */
			continue;
			break;
		  }
		  default: { /* parent or child */
			cnt=15; /* break out of for */
			break;
		  }
		}	
	
	if (cnt==10) return (2); /* cnt will be 15 if forked OK */
	if (childpid) { /* parent */
		sigignore(SIGINT);
		sigignore(SIGHUP);
		for (cnt=0;cnt < 10; cnt++)
			switch (childpid2=fork()) {

		  	  case -1: { /* retry up to 10 times */
				continue;
				break;
		  	  }
		  	  default: { /* parent or child */
				cnt=15; /* break out of for */
				break;
		  	  }
		        }
		if (cnt==10) {
			kill (childpid,SIGKILL); /* make sure 1st kid dies */
			return (2); /* cnt will be 15 if forked OK */
		}
		if (!childpid2) { /* child */
			int rv;
			sleep(20);
			rv = kill(childpid,SIGTERM);

			exit(0); /* whole purpose in life is to kill other
				  * child
				  */
		}
		waitpid(childpid, &waitflag, 0);
		sigrelse(SIGINT);
		sigrelse(SIGHUP);
		kill(childpid2,SIGKILL);
		wait(&waitflag2);
		/* return childpid's exit value */
		if (WIFEXITED(waitflag)) 
			return (WEXITSTATUS(waitflag)); /* return child exit */
		return (1); /* returned because of signal */

	}
	/* Child will run simple app to test mouse input
	 * Return 0 as soon as mouse input detected
	 */

	if (setpgrpflg) {
		int i, fd;

		setpgrp(); /* become group leader */

		for (i=0; i<20; i++)
			close(i);

		fd=open("/dev/console",O_RDWR); /* stdin */
		if (fd==-1)
			exit (4);
		dup(fd);		        /* stdout */
		dup(fd);		        /* stderr */
	}
	signal(SIGTERM, OnSigTerm);
	sleep_time = 0;
	mousefd = open ("/dev/mouse", O_RDONLY);
	/*
	 * We *could* fail right here, but the mouse test is
	 * more consistent at installation time if it always
	 * "hangs" even on cases when we can detect right away
	 * that the mouse ain't there...
	 */
	mouse_is_on = 1;
	mvaddstr(LINES - 1, 0, "Mouse tracking test program");
 	if (testing) {
		initscr (); 
	} else {
		mvaddstr(0,0,"");
		erase();
	}
	refresh();

	m.xmotion = 0;
	m.ymotion = 0;
	old_sx = sx = old_sy = sy = 0;
	x = COLS / 2 * xscale;
	y = LINES / 2 * yscale;
	while (1) {

		if ((mousefd >= 0) && (ioctl (mousefd, MOUSEIOCREAD, &m) == -1)) {
			if (testing) {
				mvprintw(0,0,"");
				erase();
				refresh();
				endwin();
			}
			exit (3);
		}

		if ((mousefd >= 0) && (m.status & BUTCHNGMASK)) {
			if (buttoncnt == 0){ 
			   /* button chg found. Wait for next button
			    * change -- otherwise next retry of test will
			    * see it and exit test prematurely
			    */
				 buttoncnt++;
				 continue;
			}
			if (testing) {
				mvprintw(0,0,"");
				erase();
				refresh();
				endwin();
			}
			exit(0);
		}
		x += m.xmotion;
		y += m.ymotion;
		mvaddch (old_sy, old_sx, (int) ' ');
		if ((sx = x / xscale) < 0)
			x = sx = 0;
		else if (sx >= COLS)
			x = (sx = COLS - 1) * xscale;
		if ((sy = y / yscale) < 2)
			y = sy = 2;
		else if (sy >= LINES - 1)
			y = (sy = LINES - 2) * yscale;
		mvaddch (sy, sx, (int) 'X');
		old_sy = sy;
		old_sx = sx;
		mvprintw (0, 0, "Press a mouse button to stop test.\n", m.status);
		printw ("Test will be canceled automatically in 15 seconds.\n", m.status);
		refresh ();
	}
}

int
delete_entry(terminal)
char	*terminal;
{
	int	slot;
	dev_t	dummy;


	if (get_dev(terminal, &dummy) < 0 || strcmp(terminal,"vt00") != 0	&& !(strncmp(terminal,"s",1)==0 && strchr(terminal,'v')!=NULL))
		return -3;
	if ((slot = lookup_disp(terminal)) == -1)
		return -1;
	if(msebusy[slot])
		return -2;
	if ( strcmp(table[slot].mouse, "bmse") == 0 )
		unconfig_mod("bmse");
	if ( strcmp(table[slot].mouse, "m320") == 0 )
		unconfig_mod("m320");
	--n_dev;
	while (slot < n_dev) {
		table[slot] = table[slot + 1];
		map[slot] = map[slot + 1];
		slot++;
	}

	return 0;
}


int
add_entry(terminal, mouse, mesg)
char	*terminal, *mouse;
unchar mesg;
{
	int	slot,i;
	int newflag = 0;
	dev_t	disp_dev, mse_dev;


	if ((slot = lookup_disp(terminal)) == -1) {
		newflag = 1;
		if ((slot = n_dev++) >= MAX_DEV)
		{
			if(mesg)
				warn_err("Too many mice configured, one must be removed before another is added.");
			else
				fprintf(stderr, "\nToo many mice configured, one must be removed before another is added.\n\n");
			return(1);
		}
	
		if (get_dev(terminal, &disp_dev) < 0 || strcmp(terminal,"vt00") != 0	&& !(strncmp(terminal,"s",1)==0 && strchr(terminal,'v')!=NULL)){
			--n_dev;
			if (mesg) {
				sprintf(errstr, "Requested display terminal is not valid.");
				warn_err(errstr);
			} else
				fprintf(stderr, "\nRequested display terminal is not valid.\n\n");
			return(1);
		}
	}
	if ((strcmp(terminal, "vt00") ==0 && (strcmp(mouse,"bmse") != 0 && strcmp(mouse,"m320") != 0)) && (strcmp(terminal,"vt00")==0 && strncmp(mouse,"tty",3)!=0)){
		--n_dev;
		if (mesg) {
			sprintf(errstr, "Requested display/mouse pair is not valid.");
			warn_err(errstr);
		} else
			fprintf(stderr, "\nRequested display/mouse pair is not valid.\n\n");
		return(1);
	}
	if ((strncmp(terminal,"s",1)==0 && strchr(terminal,'v')!=NULL)&& (strncmp(mouse,"s",1)!=0 || strchr(mouse,'t')==NULL)){
		--n_dev;
		if (mesg) {
			sprintf(errstr, "Requested display/mouse pair is not valid.");
			warn_err(errstr);
		} else
			fprintf(stderr, "\nRequested display/mouse pair is not valid.\n\n");
		return(1);
	}
   if ((strncmp(mouse, "bmse", 4) == 0 || strncmp(mouse,"m320",4) == 0) && bus_okay) 
	goto passck;


	if (get_dev(mouse, &mse_dev) < 0)
	{
		if (mesg) {
			sprintf(errstr, "%s is not a valid mouse device.", mouse);
			warn_err(errstr);
		} else
			fprintf(stderr, "\n%s is not a valid mouse device.\n\n", mouse);
		if(newflag)
			--n_dev;
		return(1);
	}
passck:
	if((strlen(mouse) > 5 && strcmp("vt00",mouse+5)==0) || strcmp("vt00",mouse) == 0)
	{
		if (mesg) {
			sprintf(errstr, "%s is not a valid mouse device.", mouse);
			warn_err(errstr);
		} else
			fprintf(stderr, "\n%s is not a valid mouse device.\n\n", mouse);
		if(newflag)
			--n_dev;
		return(1);
	}


	if(strcmp(terminal,mouse) == 0)
	{
		if (mesg)
			warn_err("The mouse and display terminal can not be connected to the same port.");
		else
			fprintf(stderr, "\nThe mouse and display terminal can not be connected to the same port.\n\n");
		if(newflag)
			--n_dev;
		return(1);
	}
	for(i=0;i<n_dev;i++){
		if(strcmp(mouse,table[i].mouse) == 0){
			if(mesg){
				sprintf(errstr,"Device %s is already assigned to a Display terminal.",mouse);
				warn_err(errstr);
			} else
				fprintf(stderr,"\nDevice %s is already assigned to a Display terminal.\n\n",mouse);
			if(newflag)
				--n_dev;
			return(1);
		}
	}
	if(msebusy[slot]){
		if(mesg){
			sprintf(errstr,"Mouse device %s is currently in use.",mouse);
			warn_err(errstr,"The configuration was not changed.");
		}
		else
			fprintf(stderr,"\nMouse device %s is currently in use.\n\n",mouse);
		return(1);
	}

	if(!newflag)
		disp_dev = map[slot].disp_dev;
	strcpy(table[slot].disp, terminal);
	map[slot].disp_dev = disp_dev;
	strcpy(table[slot].mouse, mouse);
	map[slot].mse_dev = mse_dev;
	return(0);
}

ckconfg(mouse)
char *mouse;
{
	FILE *fopen(), *fp;
	char    *pathname;
	int notfound = 1;

	char	buffer[250];

        char name[10], f2[10], f3[10], f4[10], f5[10], f6[10],
		f7[10], f8[10], f9[10], f10[10], f11[10];

	/** -b flag given **/
	if(bus_okay)
	{
		return(0);
	}
	pathname=(char *)(malloc(strlen(mouse)+23));
	sprintf(pathname, "/etc/conf/sdevice.d/%s",mouse);
	if((fp = fopen(pathname,"r")) == NULL)
	{
		return(2);
	}

	while ( fgets(buffer,250, fp) && notfound ) {
		if(sscanf(buffer,"%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s   %10s\n",name,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11) != 11)	{
			continue;
		}
		if ( strcmp(name, mouse) == 0 )
			notfound=0;
	}
	if (notfound)
		fatal_error("could not find entry in sdevice file");

	free(pathname);
	/* always have ckconfig indicate that the bus mouse needs
	 * to be configured, since the user may wish to chg IVNs.
	 */
	if (strcmp(mouse,"bmse") == 0) return (1);
	if (strcmp(f2, "Y") == 0) /* it's configured. */
		return(0);
	else { /* needs to be configured */
		return(1);
	}
}

unconfig_mod(mouse)
char *mouse;

{
	FILE *fopen(), *fp, *fp2;
        char name[10], f2[10], f3[10], f4[10], f5[10], f6[10],
		f7[10], f8[10], f9[10], f10[10], f11[10];
	char *pathname,*tmpfile;
	char buffer[250];
	struct stat sbuf;
	int notfound=1;

	pathname=(char *)(malloc(strlen(mouse)+128));
	tmpfile=(char *)(malloc(strlen(mouse)+128));
	sprintf(pathname,"/etc/conf/sdevice.d/%s",mouse);
	sprintf(tmpfile,"/etc/conf/sdevice.d/T%s",mouse);
	if((fp = fopen(pathname,"r")) == NULL)
	{
		fatal_error("can't open sdevice.d file");
	}
	while ( fgets(buffer,250, fp) && notfound) {
		if(sscanf(buffer,"%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s  %10s\n",name,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11) != 11)
		{
			continue;
		}
		if (strcmp(name, mouse) == 0) {
			notfound=0;
			break;
		}
	}
	if ( notfound )
		fatal_error("can't find entry in sdevice file");
	fclose(fp);
	if((fp = fopen(pathname,"r")) == NULL)
	{
		fatal_error("can't open sdevice.d file");
	}
	if((fp2 = fopen(tmpfile,"w")) == NULL)
	{
		fatal_error("can't open tmp file name");
	}
	while( fgets(buffer,250, fp) ) {
		if(sscanf(buffer,"%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s  %10s\n",name,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11) != 11)
		{
			if (fputs(buffer, fp2) == EOF) {
				fatal_error("can't fputs tmp sdevice.d file");
			}
			continue;
 ;
		}
		if (strcmp(name, mouse) == 0) {
			fprintf(fp2,"%s  N    %s    %s    %s    %s    %s    %s    %s   %s    %s\n",name,f3,f4,f5,f6,f7,f8,f9,f10,f11);
		}
		else {
			if (fputs(buffer, fp2) == EOF) {
				fatal_error("can't fputs tmp sdevice.d file");
			}
		}
	}

	
	fclose(fp);
	fclose(fp2);

	/* Get the original file attributes */
	(void)stat(pathname, &sbuf);

	unlink(pathname);
	rename(tmpfile, pathname);

	/* Change back to original file attributes */
	(void)chmod(pathname, sbuf.st_mode);
	(void)chown(pathname, sbuf.st_uid, sbuf.st_gid);

	/* now make sure module being unconfigured isn't loaded */
	sprintf(pathname,"modadmin -U %s 1>/dev/null 2>&1",mouse);
	(void) system(pathname);

	/* now remove entry from sdevice; note that since no kernel
	 * rebuild is done, we have to edit sdevice ourselves so that
	 * idcheck can work successfully
	 */
	sprintf(pathname,"ed /etc/conf/cf.d/sdevice 1>/dev/null 2>&1 <<EOF\n/^%s\ns/Y/N/\nw\nw\nq\nEOF\n",mouse);
	(void) system(pathname);

	/* remove entry from mod_register file (so it doesn't get loaded
	 * on next reboot)
	 */
	sprintf(pathname,"ed /etc/mod_register 1>/dev/null 2>&1 <<EOF\n/%s\nd\nw\nw\nq\nEOF\n",mouse);
	(void) system(pathname);

	/* now remove loadable copy of driver in /etc/conf/mod.a */
	sprintf(pathname,"/etc/conf/mod.d/%s",mouse);
	unlink(pathname);
	free(pathname);
	free(tmpfile);
	return(0);
}


config_mod(mouse, irq)
char *mouse;
int irq;

{
	char	buffer[250];
	FILE *fopen(), *fp, *fp2;

        char name[10], f2[10], f3[10], f4[10], f5[10], f6[10],
		f7[10], f8[10], f9[10], f10[10], f11[10];
	char *pathname, *tmpfile, *command;
	struct stat sbuf;
	int notfound=1,oirq=0;

	pathname=(char *)(malloc(strlen(mouse)+128));
	tmpfile=(char *)(malloc(strlen(mouse)+128));
	command=(char *)(malloc(strlen(mouse)+128));
	sprintf(pathname,"/etc/conf/sdevice.d/%s",mouse);
	sprintf(tmpfile,"/etc/conf/sdevice.d/T%s",mouse);
	sprintf(command, "/etc/conf/bin/idbuild -M %s 1>&2",mouse);

	if((fp = fopen(pathname,"r")) == NULL)
	{
		fatal_error("can't open sdevice.d file");
	}
	while ( fgets(buffer,250, fp) && notfound) {
		if(sscanf(buffer,"%10s	%10s	%10s	%10s	%10s	%d	%10s	%10s	%10s	%10s  %10s\n",name,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11) != 11)
		{
			continue;
		}
		if (strcmp(name, mouse) == 0) {
			notfound=0;
			oirq=atoi(f6);
			break;
		}
	}
	if ( notfound )
		fatal_error("can't find entry in sdevice file");
	fclose(fp);
	if (strcmp(f2, "Y") == 0) { /* it's configured. */
	   if ( (irq == 0) || (irq==oirq) ) {
		close(fp);
		free(pathname);
		free(tmpfile);
		free(command);
		return(0);
	   }
	}

	if((fp2 = fopen(tmpfile,"w")) == NULL)
	{
		fatal_error("can't create temporary sdevice file");
	}
	if((fp = fopen(pathname,"r")) == NULL)
	{
		fatal_error("can't re-open sdevice.d file");
	}
	while( fgets(buffer,250, fp) ) {
		if(sscanf(buffer,"%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s	%10s  %10s\n",name,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11) != 11)
		{
			if (fputs(buffer, fp2) == EOF) {
				fatal_error("can't fputs tmp sdevice.d file");
			}
			continue;
 ;
		}
		if (strcmp(name, mouse) == 0) {
		   if ( irq != 0 )
			fprintf(fp2,"%s  Y    %s    %s    %s    %d    %s    %s    %s   %s    %s\n",name,f3,f4,f5,irq,f7,f8,f9,f10,f11);
		   else
			fprintf(fp2,"%s  Y    %s    %s    %s    %s    %s    %s    %s  %s  %s\n",name,f3,f4,f5,f6,f7,f8,f9,f10,f11);
		}
		else {
			if (fputs(buffer, fp2) == EOF) {
				fatal_error("can't fputs tmp sdevice.d file");
			}
		}
	}

	
	fclose(fp);
	fclose(fp2);

	/* Get the original file attributes */
	(void)stat(pathname, &sbuf);

	unlink(pathname);
	rename(tmpfile, pathname);

	/* Change back to original file attributes */
	(void)chmod(pathname, sbuf.st_mode);
	(void)chown(pathname, sbuf.st_uid, sbuf.st_gid);

	/* run idbuild to configure module */
	sprintf(command, "modadmin -U %s 1>/tmp/mse.log 2>&1",mouse);
	(void) system(command);
	sprintf(command, "/etc/conf/bin/idbuild -M %s 1>/tmp/mse.log 2>&1",mouse);
	if (system(command) != 0) {
		fatal_error("idbuild -M failed\n");
	}
	sprintf(command, "modadmin -l %s 2>/tmp/mse.log 1>&2",mouse);
	if (system(command) != 0) {
		fatal_error("modadmin -l failed\n");
	}
	free(pathname);
	free(tmpfile);
	free(command);

}



int
select_term(terminal, check_table,eflag)
char	*terminal;
int	check_table;
int	eflag;
{
	dev_t	dummy;
	char c;

	if (check_table)
		return(1);
	if (get_dev(terminal, &dummy) < 0) {
		if (eflag) {
			beep();
			row++;
			sprintf(errstr, "Requested display terminal is not valid.");
			warn_err(errstr);
			refresh();
		}
		return(0);
	}
	return(1);
}


int
main_menu()
{
	char	ch, terminal[MAXDEVNAME], mouse[MAXDEVNAME];
	int oldrow, oldcol, retval, slot, irq;
	dev_t	dummy;
	char interrupt[MAXDEVNAME];


	c=show_menu(0);
	move(0,0);
	erase();
	show_table();
	getyx(stdscr, row, col);
	row++;

	if (c == 'E') {
		if (cursing)
			endwin();
		exit(1);
	}
	getyx(stdscr, row, col);
	row++;
	if (suserflg) {
		beep();
		mvaddstr(row,col,"Permission denied, changes will not be accepted.");
		enter_prompt();
		return(1);
	}
	switch (toupper(c)) {
	case 'R':
		mvaddstr(row++,col,"Enter the display terminal from which the mouse will be removed,");
		mvaddstr(row++,col,"or strike the ENTER key to return to the main menu.");
		get_info("Display terminal:  ", terminal);
		row++;
		if(strcmp(terminal, "console") == 0)
			strcpy(terminal, "vt00");
		strcpy(sterminal,terminal);
		if (terminal[0] == '\0')
			break;
		if ((retval = delete_entry(terminal)) < 0) {
			row++;
			if(retval == -1)
				mvaddstr(row,col,"There is no mouse assigned for this terminal.");
			else if(retval == -2)
				mvaddstr(row,col,"Cannot remove mouse while busy.");
			else if(retval == -3)
				mvaddstr(row,col, "Not a valid display terminal.");
			enter_prompt();
			break;
		}
		break;
	case 'T':
		mvaddstr(row++,col,"Please try using your mouse when the next screen appears.");
		get_info("Strike the ENTER key when ready:  ", terminal);
		if ((retval = test_entry()) > 0) {
			row++;
			mvaddstr(row,col,"Unable to detect mouse.");
			enter_prompt();
			mvprintw(0,0,"");
			clear();
			refresh();
			break;
		}
		mvprintw(0,0,"");
		clear();
		refresh();
		row++;
		
		break;
	case 'P': 
	case '3': {
		int rc=0;
		if ((slot = lookup_disp("vt00")) >= 0)
			if(msebusy[slot]){
   				mvaddstr(row++,col,"Mouse currently assigned to console is busy, change will not be accepted. ");
				enter_prompt();
				break;
			}
		rc=ckconfg("m320");
		if (rc == 2) {
			warn_err("PS2 Mouse module not configured.");
			break;
		}
		if ( rc == 1 )
			config_mod("m320", 12);
		add_entry("vt00", "m320", 1);
		break;
	}
	case 'B': {
		int rc, fail=0;
		rc=ckconfg("bmse");
		if (rc == 2) {
			warn_err("Bus mouse driver not installed");
			break;
		}

		if (rc) {
			char *cmd;
			int rc=0;

			cmd=(char *)(malloc(1024));
			if (cmd == (char *)NULL) {
				warn_err("Temporary memory allocation failure. Please retry");
				break;
			}
				
			getyx(stdscr, row, col);
			oldrow=row;
			oldcol=col;
			while (1) {

				mvaddstr(row++,col,"Enter the interrupt to be used for the Bus mouse.");
				mvaddstr(row++,col,"or strike the ENTER key to return to the main menu.");
				clrtobot();
				refresh();
				get_info("Interrupt (i.e. 2, 3, or 5:):  ", interrupt);
				row++;
				irq=atoi(interrupt);
				if ( irq == 2 )
					irq = 9;

			        sprintf(cmd,"FOO=`/etc/conf/bin/idcheck -r -v %d`\n[ \"$FOO\" = \"bmse\" ] && exit 0\n[ \"$FOO\" = \"\" ] && exit 0\nexit 1",irq);
				if ( system(cmd) != 0 ) {
					mvaddstr(row++,col,"Interrupt already in use.");
					for (;;) {
						mvaddstr(row,col,"Do you wish to select another? [y or n] ");
						clrtobot();
						refresh();
						attron(A_BOLD);
						getstr(ans);
						attroff(A_BOLD);
						ch = toupper(ans[0]);
						if (ch == 'Y' ) {
							row=oldrow;
							col=oldcol;
							break;
						} else
							if (ch == 'N') {
								fail++;
								break;
							}
						else
							beep();
					}
				}
				else {
					break;
				}
				if (fail)
					return(1);
			}
			config_mod("bmse", irq);
		}
		if ((slot = lookup_disp("vt00")) >= 0)
			if(msebusy[slot]){
   				mvaddstr(row++,col,"Mouse currently assigned to console is busy, change will not be accepted. ");
				enter_prompt();
				break;
			}
		add_entry("vt00", "bmse", 1);
		break;
	}
	case 'S':
		while (1) {
			getyx(stdscr, row, col);
			oldrow=row;
			oldcol=col;
			clrtobot();
			mvaddstr(row++,col,"Enter the display terminal that will be using the mouse,");
			mvaddstr(row++,col,"or strike the ENTER key to return to the main menu.");
			get_info("Display terminal (i.e. console, s0vt00, etc.):  ", terminal);
			row++;
			if(strcmp(terminal, "console") == 0)
				strcpy(terminal, "vt00");
			strcpy(sterminal,terminal);
			if (terminal[0] == '\0')
				break;
			if (lookup_disp(terminal) >= 0) {
				sprintf(errstr, "Requested display terminal is already configured to use a mouse.");
				mvaddstr(row++,col,errstr);
				for (;;) {
					mvaddstr(row,col,"Do you wish to continue? [y or n] ");
					clrtobot();
					refresh();
					attron(A_BOLD);
					getstr(ans);
					attroff(A_BOLD);
					ch = toupper(ans[0]);
					if (ch == 'Y' || ch == 'N')
						break;
					beep();
				}
				if (ch == 'N')
					break;
				move(row+=2,col);
			}
			if (select_term(terminal, 0, MSEERR)) {
				if(strcmp(terminal,"vt00")==0){
					mvaddstr(row++,col,"Enter the device that the mouse will be attached to,");
					mvaddstr(row++,col,"or strike the ENTER key to return to the main menu.");
					get_info("Mouse device (i.e. tty00, tty01):  ", mouse);
					row++;
					strcpy(smouse, mouse);
					if (mouse[0] == '\0')
						break;
					if(strncmp(mouse,"ttyh",4)==0||strncmp(mouse,"ttys",4)==0||strncmp(mouse,"tty",3)!=0 || (strchr(mouse,'0')==NULL && strchr(mouse,'1')==NULL) ){
						sprintf(errstr, "Requested display/mouse pair is not valid.");
						warn_err(errstr);
						break;
					}
					if (ckconfg("smse"))
						config_mod("smse", 0);
					add_entry(terminal, mouse, 1);
					break;
				} else if (strncmp(terminal,"s",1)==0) {
					if( strchr(terminal,'v')==NULL){
						row++;
						sprintf(errstr, "Requested display terminal is not valid.", terminal);
						warn_err(errstr);
						break;
					}
					mvaddstr(row++,col,"Enter the device that the mouse will be attached to,");
					mvaddstr(row++,col,"or strike the ENTER key to return to the main menu.");
					get_info("Mouse device (i.e. s0tty0, s3tty1):  ", mouse);
					row++;
					if (mouse[0] == '\0')
						break;
					strcpy(smouse,mouse);
					if(strncmp(mouse,"s",1) != 0 || strchr(mouse,'v') != NULL || strchr(mouse,'l') != NULL ){
						sprintf(errstr, "Requested display/mouse pair is not valid.");
						warn_err(errstr);
						break;
					}
					if (ckconfg("smse"))
						config_mod("smse", 0);
					add_entry(terminal, mouse, 1);
					break;
				}
				sprintf(errstr, "Requested display terminal is not valid.");
				warn_err(errstr);
				break;
			} else {
				break;
			}
		}
		break;
	case 'U':
		return(0);
	}
	return 1;
}


void
interact()
{
	initscr();
	cursing = 1;
	print = printw;

	do {
		erase();
		show_table();
	} while (main_menu());

	download_table();
	endwin();
}

show_menu(flag)
int flag;
{
	char	ch;

	getyx(stdscr, row, col);
	row++;

	mvaddstr(row++, col, "Select one of the following:");
	col += 5;
	mvaddstr(row++, col, "B)us mouse add");
	mvaddstr(row++, col, "P)S2 mouse add");
	mvaddstr(row++, col, "S)erial mouse add");
	mvaddstr(row++, col, "T)est your mouse configuration");
	if (n_dev)
		mvaddstr(row++, col, "R)emove a mouse");
	mvaddstr(row++, col, "U)pdate mouse configuration and quit");
	mvaddstr(row++, col, "E)xit (no update)");
	col -= 5;
	if (!flag) {
	   for (;;) {
		mvaddstr(row, col, "Enter Selection:  ");
		clrtobot();
		refresh();
		attron(A_BOLD);
		getstr(ans);
		attroff(A_BOLD);
		ch = toupper(ans[0]);
		if (strchr("BPSUTE", ch) || (n_dev && ch == 'R'))
			break;
		beep();
	   }
	   row++;
	   return(ch);
	}
}
