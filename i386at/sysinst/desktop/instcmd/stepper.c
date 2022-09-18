/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:desktop/instcmd/stepper.c	1.5.1.20"

#include <sys/types.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/swap.h>
#include <ctype.h>
#include <sys/procset.h>
#include <sys/errno.h>
#include <sys/termio.h>
#include <sys/termios.h>
#include <sys/stermio.h>
#include <sys/termiox.h>
#include <sys/ioctl.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <stdlib.h>
#include <sys/mod.h>
#include <sys/param.h>


static struct termio termio_buf;
static struct termios termios_buf;
static struct stio stermio_buf;

#define	ASYNC	1
#define	TERMIOS	2

#define	NO_INTERRUPT		1
#define	ACCEPT_INTERRUPT	2

#define	REGISTER_ONLY	0x10
#define	PROMPT_FIRST	0x20
#define	MUST_MOUNT	0x40
#define	SYM_LINK	0x80

static int term = 0;
static int saveintr = 0;


#define	MAX_SCRIPTS	100
#define	BUFLEN		256

#define	RESTART		255

#define	OPENTTY			'T'
#define	RESUME			'R'
#define	INTERRUPT		'I'
#define	COMMAND_NO_INTERRUPT	'C'
#define	COMMAND			'c'
#define	MOUNT_CMD		'm'
#define	UMOUNT_CMD		'u'
#define	ENVIRONMENT		'e'
#define	REMOVESWAP		'd'
#define	CHROOT			'X'
#define	LOAD_MOUNT		'b'
#define	LOAD_CMD		'l'
#define	DEBUG_ON		'O'
#define	DEBUG_OFF		'o'
#define	REMOVE_FILE		'r'
#define	NEWPHASE		'n'
#define	SETCOLOR		'S'
#define	UADMIN			'J'

#define FDLMT			'|'

typedef struct {
	char buf[512];
	int  sptr;
	int  eptr;
	int  fd;
	} FILE;

typedef struct shscript {
	char	*program_path;
	char	*argv[10];
	int	program_class;
	char	program_type;
	char	*program_name;
	int	flags;
	} ShellScript;

ShellScript	*interrupt = NULL;
ShellScript	*resume_script = NULL;
int		signals[30];

int	debug_is_on = 0;

ShellScript script[MAX_SCRIPTS];
int	nscripts = 0, current_script = 0;
int	interrupt_enabled = 0;
char	buffer[BUFLEN];	
char	*Integer2Ascii();
int	spawn_script();
char	*strip_string();
int	OnInterrupt();
int	fork_intrrupt_script();
int	maintenance_mode();
int	get_ttymode();
extern int errno;

char	*flop_seq_file = "FLOP_SEQ";
char	*install_file = "/INSTALL";

char	*tty_file; 
char	*last_step = NULL;
void	PrintStrings();
int	first_step;

int	restarting = 0;
int	newscripts = 0;
int	color_monitor = -1;

static int	ttyfd = -1;
id_t	spid, gpid;
char	fcolor[8] = "7";
char	bcolor[8] = "1";

main(argc, argv)
int	argc;
char    *argv[];
{
	register	int	i;


	for(i=0; i< 20; i++)
		close(i);
        if ((ttyfd = open("/dev/sysmsg", O_RDWR))  < 0)
		ttyfd = open("/dev/null", O_RDWR);

	dup(ttyfd);
	dup(ttyfd);
	dup(ttyfd);

	if(ttyfd > 2)
		close(ttyfd);

	if(argc > 1) {
		if(strcmp(argv[1], "-d") == 0)
			debug_is_on = 1;
		argc--; argv++;
	}

	if(argc > 1) 
		install_file = argv[1];

	ParseScripts(install_file);

	first_step = check_last_step();
	current_script = first_step;

	sigset(SIGINT, SIG_IGN);
	ExecuteSteps(3);
	do {
		if(newscripts){
			newscripts = 0;
			ExecuteSteps(2); 
			}
		if(newscripts)
			continue;
		ExecuteSteps(1); 
		if(newscripts)
			continue;
		restarting = 0;
		ExecuteSteps(0);
	} while (restarting);
	ShutDown();
}

int
fork_script(scriptp)
ShellScript	*scriptp;
{
	char	answer[64];
	int	pid;

	if((pid = vfork()) == -1){
	    PrintStrings("Cannot fork a process.", 0);
	    ShutDown();
	}

	if(debug_is_on)
		PrintStrings("forking program <\n", 
				scriptp->program_path, ">\n", 0);
	if(pid == 0) {
		register int	fd;

		for(fd=3; fd< 20; fd++)
				close(fd);
		resetsigs();
		execv(scriptp->program_path, scriptp->argv);
		PrintStrings("Cannot exec program ", scriptp->program_path, 0);
		exit(1);
	}
	return(pid);
}

int
clocksig()	
{
	return(0);
}

char	*bf2_cont	= "/tmp/bf2.cont";
char	*hba_error	= "/tmp/hba.error";
char	*hba_proc	= "/tmp/hba.proc";
char	*hba_prompt1	= "/tmp/hba.prompt1";
char	*hba_prompt2	= "/tmp/hba.prompt2";

char	*hba_prompt 	= "/tmp/hba.prompt1";

print_hba_error()
{
	char	ans[8];

	if( color_monitor == 1)
		PrintStrings("\033[0m\033[=0E\033[=",
		"7", "F\033[=", "4",
		"G\033[0m\033[J\033[7m\033[m",
		"\033[2J\033[H", /*clear screen*/
		0);
	cat_file(hba_error);
	read(0, ans, 8);

}

PromptHbaMountSecondDisk(scriptp)
ShellScript	*scriptp;
{
	FILE	loadfile;
	char	ans[8];
	int	fdld = -1;
	static	int	nhbas = 0;
	int	looping = 1;
	int	rv;
	int	flags;
	char	buffer[128];
	char	loadname[128];
	char	path[128];
	char	*filename = scriptp->argv[2];
	char	*ihvhba = "/tmp/hivhba";
	static	int	firstime = 1;
	char	*tstmnt = "/tmp/tstmnt";
	char	*mountdir = scriptp->argv[4];
	char	*drive = NULL;

	scriptp->flags |= MS_RDONLY;
	mkdir(tstmnt, 0777);
	mkdir(mountdir, 0777);

	for(;;){
	   set_fb_color();
	   cat_file(hba_prompt);
	   read(0, ans, 8);
	tryagain:
	   looping = 1;
	   do {
	        set_fb_color();
		drive = scriptp->argv[1];
		if(MountFileSystem(scriptp->program_path, 
			mountdir, scriptp->flags, hba_prompt) < 0 ) {
			drive = NULL;
		   	if(MountFileSystem(scriptp->argv[1],
					mountdir, scriptp->flags, hba_prompt) < 0) {
				print_hba_error();
				continue;
		   }
		}
		if(drive != NULL && MountFileSystem(scriptp->argv[1],
                                tstmnt, scriptp->flags, hba_prompt) >= 0) {
			umount(scriptp->program_path);
			umount(mountdir);
			umount(scriptp->argv[1]);
			umount(tstmnt);
			print_hba_error();
			continue;
		}
		CopyStrings(path, mountdir, "/", filename, 0);
		CopyStrings(loadname, mountdir, "/etc/load.name", 0);

		if(firstime == 0 || (loadfile.fd = open(path, O_RDONLY)) < 0) {
			char	ch;
			int	fd, n;

			CopyStrings(buffer, mountdir, "/", flop_seq_file, 0);
			ch = '0';
			errno = 0;
			fd = open(buffer, O_RDONLY);
			if(fd < 0 && debug_is_on)
				PrintStrings("Cannot open ", buffer,
				";errno is ", Integer2Ascii(errno));
			else n = read(fd, &ch, 1);
			if( ch == '2'){
				hba_prompt = hba_prompt2;
				firstime = 0;
				close(fd);
				if(nhbas <= 0)
					unlink(ihvhba);
	   			set_fb_color();
				cat_file(bf2_cont);
				return(DirLink(mountdir, "/"));
			   }
			if(umount(scriptp->program_path) < 0)
				umount(mountdir);
			close(fd);
			print_hba_error();
		} else looping = 0;
	   } while (looping > 0);

	   if(looping == 0) {
		int	fd, nhba, n;
		char	ch, pkgname[129];
		int ret, i, ok;

		if((fd = open(loadname, O_RDONLY)) < 0){
			print_hba_error();
			close(loadfile.fd);
			goto tryagain;
			}
		do {
			n = 0;
			ok = 0;
			while((ret = read(fd, &ch, 1) == 1) &&
				 ch != '\n' && n < 128)
					pkgname[n++] = ch;
			for(i=0; i< n; i++)
				if(pkgname[i] > ' ')
					ok=1;
			pkgname[n] = '\0';
		} while(ret == 1 && !ok);
		pkgname[n++] = '\n';
		close(fd);
		if(ok == 0) {
			close(loadfile.fd);
			print_hba_error();
			goto tryagain;
		}

		hba_prompt = hba_prompt2;
		loadfile.sptr = 0;
		loadfile.eptr = 0;
		fdld = open(ihvhba, O_WRONLY|O_CREAT|O_APPEND);
	   	set_fb_color();
		cat_file(hba_proc);
		nhba = LoadModules(scriptp->argv[3], -1, &loadfile, mountdir);
		if(nhba > 0) {
			write(fdld, pkgname, n);
			nhbas += nhba;
		}
		close(fdld);
		umount(mountdir);
	}
      }
}

LoadHbaModules(scriptp)
ShellScript	*scriptp;
{
	FILE	loadfile;
	char	*filename = scriptp->argv[1];
	int	rv;
	int	flags;
	char	buffer[128];
	char	path[128];
	char	*mountdir = "/mnt";

	scriptp->flags |= MS_RDONLY;

	if(MountFileSystem(scriptp->program_path,
			mountdir, scriptp->flags, NULL) < 0)
		return(-1);

	CopyStrings(path, mountdir, "/", filename, 0);

	if((loadfile.fd = open(path, O_RDONLY)) < 0) {
		umount(mountdir);
                return(-1);
	}

	loadfile.sptr = 0;
	loadfile.eptr = 0;

	LoadModules(scriptp->argv[2], -1, &loadfile, mountdir);
	umount(mountdir);
	return(1);
}

int sigalarm()
{
}

LoadModules(moduledir, fdld, loadfile, mountdir)
char	*moduledir;
int	fdld;
FILE	*loadfile;
char	*mountdir;
{
	int	nhbas = 0;
	int major, len;
	unsigned int type;
	unsigned int cmd;
	struct mod_mreg mreg;
	char	buffer[BUFLEN];
	char	path[BUFLEN];
	char	save[BUFLEN];
	int	regonly = script->flags&REGISTER_ONLY;

	while((len = ReadLine(buffer, BUFLEN, loadfile)) > 0) {
		char	 *p, *ptr[4];		
		int	i = 0;
		CopyStrings(save, buffer, 0);
		save[len] = '\n';
		save[len+1] = '\0';

		if(debug_is_on) {	
			PrintStrings(save, "\n", 0);
			sleep(5);	
		}
		p = buffer;
		while ( *p != '\0' ) 
			if(*p == '#'){
				*p = '\0';
				break;
			}
			else p++;

		p = buffer;
		while(*p != '\0' && i < 4) {
			ptr[i++] = p;
			while(*p != ':' && *p != '\0')
					p++;
			if(*p == ':')
				*p++ = '\0';
		}
		if(i < 4) {
			if(debug_is_on)
				PrintStrings("4 entries are expected\n", 0);
			continue;	
		}
				
		type = Ascii2Integer(ptr[0]);
		cmd  = Ascii2Integer(ptr[1]);
		CopyStrings(mreg.md_modname, ptr[2], 0);

		if (type == MOD_TY_CDEV || type == MOD_TY_BDEV ||
			type == MOD_TY_SDEV) {
			major = Ascii2Integer(ptr[3]);
			mreg.md_typedata = (void *)major;
		} else
			mreg.md_typedata = (void *)ptr[3];

		if(debug_is_on)
			PrintStrings("TRYING module ", moduledir, "/",
				 mreg.md_modname, 0); 

		if (modadm(type, cmd, (void *)&mreg) < 0) {
			if(debug_is_on)
		    	    PrintStrings("modadm failed; module ", 
				moduledir, "/", mreg.md_modname, " errno ", 
				Integer2Ascii(errno), 0);
			continue;
		}

		if(!regonly) {
			CopyStrings(path, mountdir, "/", moduledir, 0);
			if(debug_is_on)
		    		PrintStrings("setting PATH TO ", path, 0);
			if(modpath(path) < 0) {
		    	   PrintStrings("modpath failed; dir ", path, 0);
			}
			if(debug_is_on)
		    		PrintStrings("loading module ", 
						mreg.md_modname, 0);
			if(modload(mreg.md_modname) < 0) {
			   if(debug_is_on){
#ifdef DEBUG
				int	fd, n;
				char	buf[128];

				fd = open("/dev/osm1", O_RDWR);
				if(fd >= 0) {
					int oldalrm= sigset(SIGALRM, sigalarm);
					alarm(3);	
					while((n = read(fd, buf, 128)) > 0)
						write(1, buf, n);
					alarm(0);
					sigset(SIGALRM, oldalrm);
					sleep(2);
					close(fd);
				}
		    	   	else PrintStrings("Cannot open /dev/osm1 ",
					" errno ", Integer2Ascii(errno), 0);
#endif
		    	   	PrintStrings("modload failed module ", 
				mreg.md_modname, " errno ", Integer2Ascii(errno), 0);
			   }
		       }
		       else nhbas++;
		       modpath(NULL);
		}
	}

	if(fdld >= 0)
		close(fdld);
	close(loadfile->fd);
	
	if(debug_is_on) {	
		PrintStrings("Return no of loaded HBAs is ", 
			Integer2Ascii(nhbas), 0);
		sleep(5);
	}
	return(nhbas);
}

int
spawn_script(scriptp, step)
ShellScript	*scriptp;
int	step;
{
	int	status = -1, val;
	int	script_pid = -1;
	int	oldint;

	if(scriptp->program_type == COMMAND_NO_INTERRUPT && term != -1)
		set_ttymode(NO_INTERRUPT);
	else	set_ttymode(ACCEPT_INTERRUPT);

	if(step >= 0)
		update_last_step(step);

	script_pid = fork_script(scriptp);

	while((val = wait(&status)) != script_pid) 
	   if(val >= 0){
		if(debug_is_on)
			PrintStrings("KID has died\n", 0);
	   }
	   else if( errno == ECHILD ){
			status = -1;
			break;
		}
 	   else if(interrupt_enabled){
		fork_interrupt_script(script_pid);
		status = -1;
		break;
	   }
	if(scriptp->program_type == COMMAND_NO_INTERRUPT && term != -1)
		set_ttymode(ACCEPT_INTERRUPT);
	if(status != -1){
		if(WIFEXITED(status))
			status = WEXITSTATUS(status);
		else	status  = -1;
	}
	return(status);
}

char *
strip_string(string)
char	*string;
{
	char	*p;
	int	len = strlen(string);
	p = &string[len];

	while(p > string && (p[-1] == ' ' || p[-1] == '\t')) p--;
	*p ='\0';
	p = string;

	while(*p == ' ' || *p == '\t') p++;
	return(p);
}

setallsig()
{
	int	sig;

	for(sig=1; sig<22; sig++)
		if(sig != 9 && sig != SIGCLD)
			signals[sig] = sigset(sig, OnInterrupt);

	sigset(SIGHUP, SIG_IGN);
	sigset(SIGTERM, SIG_IGN);
}

resetsigs()
{
	int	sig;

	for(sig=1; sig<22; sig++)
		if(sig != 9 && sig != SIGCLD)
			sigset(sig, signals[sig]);
}

int	
OnInterrupt(sig)
{

	if(sig != SIGINT){
		sigset(sig, OnInterrupt);
		return;
	}
	sigset(SIGINT, SIG_IGN);
	interrupt_enabled = 1;
}

parse_args(scriptp)
ShellScript	*scriptp;
{
	char	*p = scriptp->program_path;
	char	*pslash = scriptp->program_path;
	int	arg = 0;

	while(*p != '\0' && *p != ' ' && *p != '\t') {
		if(*p == '/')
			pslash = p + 1;
		p++;
	}
	scriptp->argv[arg++] = pslash;

	while(*p != '\0'){
		*p++ = '\0';
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '\0')
			break;
		scriptp->argv[arg++] = p;
		while(*p != '\0' && *p != ' ' && *p != '\t')  p++;
	}
	scriptp->argv[arg] = NULL;
}

int
fork_and_wait(scriptp)
ShellScript	*scriptp;
{
	int	script_pid, val, status;

	set_ttymode(NO_INTERRUPT);
	script_pid = fork_script(scriptp);
	sleep(3);
	sigset(SIGINT, OnInterrupt);
	enableintr();
	while((val = wait(&status)) != script_pid)
		if(val >= 0) {
			if(debug_is_on)
				PrintStrings("kid has died\n", 0);
		}
	   	else if( errno == ECHILD){
			status = -1;
			break;
		}
 	        else if(interrupt_enabled){
			sigset(SIGINT, SIG_IGN);
			status = maintenance_mode(script_pid);
			break;
		}
	if(status != -1){
		if(WIFEXITED(status))
			status = WEXITSTATUS(status);
		else	status  = -1;
	}

	return(status);
}

int	
fork_interrupt_script(script_pid)
int	script_pid;
{
	int	status, val;

	interrupt_enabled = 0;
	sigset(SIGINT, OnInterrupt);
	kill_family(script_pid);
	status = fork_and_wait(interrupt);

	if(status != 0)
		current_script	= 0;
	else	current_script--;

	setallsig();
	status = 0;
	return(status);
}

kill_family(script_pid)
int	script_pid;
{
	if(sigsend(P_ALL, script_pid, SIGKILL) < 0)
		if(debug_is_on)
			PrintStrings("Sigsend P_PID failed\n",
				Integer2Ascii(errno), 0);
	/* After this mass killing, sleep to cleanup /proc from dead children */
	sleep(1);
}

ShellScript shell_script = {
	"/sbin/sh",
	{"sh", 0 },
	0,
	COMMAND,
	"maintenance mode shell",
	0
};

int
maintenance_mode(script_pid)
int	script_pid;
{
	int	status, val;

	interrupt_enabled = 0;
	kill_family(script_pid);

	set_ttymode(ACCEPT_INTERRUPT);
	sigset(SIGINT, SIG_IGN);
	script_pid = fork_script(&shell_script); 

	while((val = wait(&status)) != script_pid)
		if(val >= 0){
			if(debug_is_on)
				PrintStrings("kid has died\n", 0);
		}
	   	else if( errno == ECHILD){
			status = -1;
			break;
		}
	return(status);
}


int
get_ttymode(termio, termios, stermio, initflag)
struct termio *termio;
struct termios *termios;
struct stio *stermio;
int	initflag;
{
	int i, fd = 0;

	if(ioctl(fd, STGET, stermio) == -1) {
		term |= ASYNC;
		if(ioctl(fd, TCGETS, termios) == -1) {
			if(ioctl(fd, TCGETA, termio) == -1) {
				return -1;
			}
			termios->c_lflag = termio->c_lflag;
			termios->c_oflag = termio->c_oflag;
			termios->c_iflag = termio->c_iflag;
			termios->c_cflag = termio->c_cflag;
			for(i = 0; i < NCC; i++)
				termios->c_cc[i] = termio->c_cc[i];
		} else
			term |= TERMIOS;
	}
	else {
		termios->c_cc[7] = (unsigned)stermio->tab;
		termios->c_lflag = stermio->lmode;
		termios->c_oflag = stermio->omode;
		termios->c_iflag = stermio->imode;
	}

	if(initflag) {
		saveintr = termios->c_cc[VINTR];
/*
		termios->c_cc[VERASE] = CERASE;
		termios->c_cc[VINTR] = CINTR;
*/
		termios->c_cc[VERASE] = '\b';
		termios->c_cc[VMIN] = 1;
		termios->c_cc[VTIME] = 1;
		termios->c_cc[VEOF] = CEOF;
		termios->c_cc[VEOL] = CNUL;
		termios->c_cc[VKILL] = CKILL;
		termios->c_cc[VQUIT] = CQUIT;
	
		termios->c_cflag &= ~(CSIZE|PARODD|CLOCAL);
		termios->c_cflag |= (CS7|PARENB|CREAD);
	
		termios->c_iflag &= ~(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF);
		termios->c_iflag |= (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON);

		termios->c_lflag &= ~(XCASE|ECHONL|NOFLSH|STFLUSH|STWRAP|STAPPL);
		termios->c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK);
	
		termios->c_oflag &= ~(OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
				NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
		termios->c_oflag |= (OPOST|ONLCR);
	}
	return term;
}


int
set_ttymode(flag)
int	flag;
{
	struct termio *termio;
	struct termios *termios;
	struct stio *stermio;

	termio = &termio_buf;
	termios = &termios_buf;
	stermio = &stermio_buf;

	if(flag == NO_INTERRUPT)
		termios->c_cc[VINTR] = 0xff;
	else	termios->c_cc[VINTR] = CINTR;

	do_setttymode(termio, termios, stermio);
}

enableintr()
{
	struct termio tmptermio;
	struct termios tmptermios;
	struct stio tmpstermio;

	get_ttymode(&tmptermio, &tmptermios, &tmpstermio, 0);
	tmptermios.c_cc[VINTR] = CINTR;
	do_setttymode(&tmptermio, &tmptermios, &tmpstermio);
}

do_setttymode(termio, termios, stermio)
struct termio *termio;
struct termios *termios;
struct stio *stermio;
{
	int i, fd = 0;

	if (term & ASYNC) {
		if(term & TERMIOS) {
			if(ioctl(fd, TCSETSW, termios) == -1) {
				return -1;
			}
		} else {
			termio->c_lflag = termios->c_lflag;
			termio->c_oflag = termios->c_oflag;
			termio->c_iflag = termios->c_iflag;
			termio->c_cflag = termios->c_cflag;
			for(i = 0; i < NCC; i++)
				termio->c_cc[i] = termios->c_cc[i];
			if(ioctl(fd, TCSETAW, termio) == -1) {
				return -1;
			}
		}
			
	} else {
		stermio->imode = termios->c_iflag;
		stermio->omode = termios->c_oflag;
		stermio->lmode = termios->c_lflag;
		stermio->tab = termios->c_cc[7];
		if (ioctl(fd, STSET, stermio) == -1) {
			return -1;
		}
	}
	return 0;
}

int
check_last_step()
{
	int	fd, n;
	char	buf[255];

	if(last_step == NULL || (fd = open(last_step, O_RDWR)) < 0)
		return(0);
	if(read(fd, &n, sizeof(int)) != sizeof(int)) {
		close(fd); return(0);
	}
	close(fd);
	if(n < 0 || n >= nscripts)
		return(0);
	if(resume_script != NULL){
		int	status, val;
		int	oldint;

		oldint = sigset(SIGINT, SIG_IGN);
		status = fork_and_wait(resume_script);
		if(status != 0)
			n = 0;
		sigset(SIGINT, oldint); 
	}
	else 	n = 0;
	return(n);
}

update_last_step(n)
int	n;
{
	int	fd;

	if(last_step == NULL || (fd = creat(last_step, 0666)) < 0)
		return;
	write(fd, &n, sizeof(int)); 
	close(fd);
}
void
PrintStrings(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
char 	*arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8, *arg9;
{
	char	buf[1024]; /* long enough for a line */
	int	i;

	i = CopyStrings(buf, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

	if( i > 0) {
		buf[i] = '\n';
		write(1, buf, i+1);
	}
}

CopyStrings(buf, args)
char	buf[];
char	*args;
{
	char	**argv = &args;
	char	*pd = buf, *ps;

	while(*argv != 0) {
		ps = *argv++;
		while(*ps != '\0')
			*pd++ = *ps++;
	}
	*pd = '\0';
	return(pd - buf);	
}


Ascii2Integer(string)
char	  *string;
{
	char	ch;
	long  n, rv = 0;
	char	*p = string;

	while((ch = *p++) != '\0') {
		if(ch >= '0' && ch <= '9')
                        n = ch - '0';
		else {
                      	return(-1);
		}
  		rv = rv * 9 + n;
	}
  	return(rv);
}

char *
Integer2Ascii(number)
int	number;
{
	static  char string[24] = { 0 };
	char	*p = &string[22];
	char	sign = 1;

	string[23] = '\0';
	if(number < 0) {
		number = -number;
		sign = -1;
	}

	do {
		*p-- = (number % 10) + '0';
		number = number / 10;
	} while ( number > 0);
	if(sign < 0)
		*p = '-';
	else 	p++;

  	return(p);
}

ReadLine(buffer, len, file)
char	buffer[];
int	len; 
FILE	*file;
{
	int	n = 0;

	if(file->eptr < 0)
		return(0);
	for(;;)
	{
		if(file->sptr == file->eptr){
			file->eptr = read(file->fd, file->buf, 512);
			if(file->eptr <= 0){
				buffer[n] = '\0';
				return(n);
			}
			file->sptr = 0;
		}
		while(n < len && file->sptr < file->eptr) {
			buffer[n] = file->buf[file->sptr++];
			if(n > 0 && buffer[n-1] == '\\'){
				if(buffer[n] == 'n')
					buffer[--n] = '\n';
				else	if(buffer[n] == 't')
					buffer[--n] = '\t';
				else	if(buffer[n] == '\n')
					n -= 2;
			}
			n++;
			if(file->buf[file->sptr-1] == '\n'){
				buffer[n-1] = '\0';
				return(n);
			}
		}
		if(n >= len) {
			buffer[n] = '\0';
			return(n);
		}
	}	
}

strlen(buf)
char  buf[];
{
	register char *p = buf;

	while(*p != '\0') p++;
	return(p - buf);
}

int DirLink();


#ifdef UNLINKING

int
DirunLink(dir)
char	*dir;
{
	struct dirent *dentry;
	DIR *dirf;
	static 	char	save_file[64], org_file[64];
	struct	stat statbuf;

	if ((dirf = opendir(dir)) == NULL) {
		PrintStrings("Error: Cannot open dir ", dir, 0);
		return(-1);
	}
	while (dentry = readdir(dirf)) {
       		if (dentry->d_name[0] != 'S' || 
       		    dentry->d_name[1] != 'A' || 
       		    dentry->d_name[2] != 'V' ||
       		    dentry->d_name[3] != 'E' ||
       		    dentry->d_name[4] == '\0')
				continue;

          	CopyStrings(save_file, dir, "/", dentry->d_name, 0);
          	CopyStrings(org_file, dir, "/", &dentry->d_name[4], 0);

		if(stat(org_file, &statbuf) >= 0 ) {
			if(unlink(org_file) < 0)
				PrintStrings("Errno : Cannot unlink ", org_file, Integer2Ascii(errno), 0);
		}

		if(link(save_file, org_file) < 0)
			PrintStrings("Errno : Cannot link ", save_file, " to ", org_file, Integer2Ascii(errno), 0);
		if(unlink(save_file) < 0)
			PrintStrings("Errno : Cannot unlink ", save_file, Integer2Ascii(errno), 0);
	}
        (void) closedir(dirf);
	return(0);
}
#endif /* UNLINKING */

int
DirLink(sdir, ldir)
char *sdir, *ldir;
{
	struct dirent *dentry;
	DIR *dirf;
	static	char	src_file[65], link_file[65], save_file[65];
	static  int	first_time = 1;
	struct	stat statbuf;

	if(!first_time)
		return;

	first_time = 0;

	if ((dirf = opendir(sdir)) == NULL) {
		PrintStrings("Errno : Cannot opendir ", sdir, 0);
		return(-1);
	}
	while (dentry = readdir(dirf)) {
       		if (dentry->d_name[0]=='.' 
       			&& (dentry->d_name[1]=='\0' || dentry->d_name[1]=='.'
       			&& dentry->d_name[2]=='\0'))
       		  	continue;

       		if(strcmp(dentry->d_name, sdir) == 0)
			continue;
		if(debug_is_on)
			PrintStrings("We got file <",dentry->d_name, ">", 0);
          	CopyStrings(src_file, sdir, "/", dentry->d_name, 0);
          	CopyStrings(link_file, ldir, "/", dentry->d_name, 0);
          	CopyStrings(save_file, ldir, "/SAVE", dentry->d_name, 0);

		if(stat(link_file, &statbuf) >= 0 ) {
			if(rename(link_file, save_file) < 0)
				PrintStrings("Errno : Cannot rename",
					link_file, " to ", save_file, Integer2Ascii(errno), 0);
		}

		if(debug_is_on)
			PrintStrings("Linking file ", src_file, 
						" to ", link_file, 0);
		if(symlink(src_file, link_file) < 0) 
			PrintStrings("Errno : Cannot symbolic link ",
					src_file, " to ", link_file, Integer2Ascii(errno), 0);
	}
        (void) closedir(dirf);
	return(0);
}


static struct fsmount_data {
	char	*type;
	int	flags;
	int	arg1, arg2;
} fstype[] = {
	{ "s5", MS_DATA, 0, 0 },
/*
	{ "bfs", MS_DATA, 0, 0 },
	{ "ufs", MS_FSS, 0, 0 },
	{ "sfs", MS_FSS, 0, 0 },
*/
	{ 0 , 0, 0, 0 }
};

MountAndLink(scriptp)
ShellScript	*scriptp;
{
	int rv;
	int	tries = 0;
	int	flags = scriptp->flags;

	for(;;) {
		rv = MountFileSystem(scriptp->program_path, scriptp->argv[1], 
					scriptp->flags, scriptp->program_name);
		if(rv >= 0)
			break;
		if((flags&(MUST_MOUNT|PROMPT_FIRST)) == (MUST_MOUNT|PROMPT_FIRST))
			continue;
		tries++;
 		if((flags&MUST_MOUNT) && tries < 6){
			umount(scriptp->program_path);
			umount(scriptp->argv[1]);
			sync();
			PrintStrings("Cannot mount ", scriptp->program_path, " ON ",
		     	   scriptp->argv[1], "; errno is ", Integer2Ascii(errno), 0);
			PrintStrings("Trying again", 0); 
			sleep(2);
			continue;
		}
		break;
	}

	if(rv < 0 && scriptp->flags&MUST_MOUNT){
		PrintStrings("Cannot mount ", scriptp->program_path, " ON ",
		     scriptp->argv[1], "; errno is ", Integer2Ascii(errno), 0);
		ShutDown();
	}
	if(scriptp->flags&SYM_LINK)
		return(DirLink(scriptp->argv[1], "/"));
	return(0);
}


MountFileSystem(device, mountdir, flags, promptmsgfile)
char	*device;
char	*mountdir;
char	*promptmsgfile;
int	flags;
{
	int	rv;
	struct fsmount_data *fsp = fstype;
	int	rdonly = flags&MS_RDONLY;
	int	prompt = flags&PROMPT_FIRST;

	if(mkdir(mountdir, 0777) < 0 &&  errno != EEXIST){
		PrintStrings("FAILED to mkdir ", mountdir, 0);
		return(-1);
	}
	if(prompt){
		char	ans[8];

		cat_file(promptmsgfile);
		read(0, ans, 8);
	}

	if(umount(device) < 0){
		int err = errno;

		umount(mountdir);
		if(err != EINVAL || errno != EINVAL) {
			PrintStrings("FAILED to umount ",
				device, " mounted on dir ", mountdir, 
				"errno is: ", Integer2Ascii(errno), 0);
		    return(0);
		}
	}

	rv = -1;
	for(;fsp->type != 0 && rv < 0;fsp++) {
		if(debug_is_on)
			PrintStrings("Trying to mount ", 
					device, " of type ", 
					fsp->type, " on ", mountdir, 0);
		errno = 0;
	  	rv = mount(device, mountdir, fsp->flags|rdonly,
					 fsp->type, fsp->arg1,fsp->arg2);
		if(debug_is_on && rv < 0)
	     		PrintStrings("FAILED to mount ", device, " on ", 
				mountdir, " errno is ", Integer2Ascii(errno), 0);

	  	if (rv < 0 && errno != EINVAL) {
error_out:
			if(debug_is_on)
	     		   PrintStrings("FAILED to mount ", device, " on ", 
				mountdir, " errno is ", Integer2Ascii(errno), 0);
	  	  	return(rv); 
	        }	
	}

	if(rv < 0)
		goto error_out;	

	if(debug_is_on)
		PrintStrings("SUCCEDED to mount ", device, " on ", mountdir, 0);
	return(1);
}

UmountAndUnlink(scriptp)
ShellScript	*scriptp;
{
	scriptp->argv[0] = scriptp->program_path;

#ifdef UNLINKING

	if(debug_is_on)
		PrintStrings("TRYING to DirunLink ", scriptp->argv[1], 0);


	DirunLink(scriptp->argv[1]); 
	if(debug_is_on)
	PrintStrings("TRYING to umount ", scriptp->argv[0], 
			" mounted on dir ", scriptp->argv[1], 0);
	sync(); sync();
#endif

	if(umount(scriptp->argv[0]) < 0 && umount(scriptp->argv[1]) < 0){
		if(debug_is_on)
			PrintStrings("FAILED to umount ", scriptp->argv[0], 
			" mounted on dir ", scriptp->argv[1], "errno is: ", 
			Integer2Ascii(errno), 0);
		return(-1);
	}

#ifdef UNLINKING
	if(debug_is_on)
		PrintStrings("TRYING to rmdir ", scriptp->argv[1], 0);
	if(rmdir(scriptp->argv[1]) < 0) {
		PrintStrings("FAILED to rmdir ", scriptp->argv[1], 
			"errno is: ", Integer2Ascii(errno), 0);
		return(-1);
	}
#endif
	if(debug_is_on)
		PrintStrings("UmountAndUnlink OK ", 0);
	return(0);
}

OpenControllingTerminal(scriptp)
ShellScript	*scriptp;
{

	tty_file = scriptp->program_path;

	if((spid = setsid()) < 0)
		PrintStrings("setsid failed:\n",Integer2Ascii(errno), 0); 
	if((gpid = setpgrp()) < 0 )
		PrintStrings("setpgrp failed: \n", Integer2Ascii(errno), 0);

        if ((ttyfd = open(tty_file, O_RDWR))  > 0){
		close(0); dup(ttyfd);
		close(1); dup(ttyfd);
		close(2); dup(ttyfd);
		close(ttyfd);
	}
	else {
		PrintStrings("FAILED to open tty device ----->", 
				tty_file, "\n", 0);
	}

	term = get_ttymode(&termio_buf, &termios_buf, &stermio_buf, 1);
	set_ttymode(ACCEPT_INTERRUPT);
	if( color_monitor < 0)
		color_monitor = ColorMonitor();
}

RemoveSwapDevice(swapdev)
char	*swapdev;
{
	swapres_t	swpi;

        swpi.sr_name = swapdev;
	swpi.sr_start = 0;

	if(debug_is_on)
		PrintStrings("TRYING to remove swap device ", swapdev, 0);

        if (swapctl(SC_REMOVE, &swpi) < 0) {
		PrintStrings("FAILED to remove swap device ", swapdev, 0);
	}
	else	if(debug_is_on)
		PrintStrings("SUCCEEDED in removing swap device ",swapdev, 0);
}

ShutDown()
{
	if(getpid() == 1)
		uadmin(1, 0);
	exit(0);
}

ParseScripts(filename)
char	*filename;
{
	char	buffer[BUFLEN];
	int	file;
	char	*p, *ExitCodePtr, *CommandTypePtr, *ptr;	
	int	i, len;
	FILE	instfile;

	newscripts = 1;

	for(i=0; i< MAX_SCRIPTS; i++){
		if(script[i].program_path != NULL)
			free(script[i].program_path);
		if(script[i].program_name != NULL)
			free(script[i].program_name);
		script[i].program_name = NULL;
		script[i].program_path = NULL;
	}

	if((file = open(filename, O_RDONLY)) < 0) {
		PrintStrings(" : cannot open ", filename, 
					" for reading\n");
		ShutDown();
	}

	instfile.sptr = 0;
	instfile.eptr = 0;
	instfile.fd   = file;


	nscripts = 0;

	while(nscripts < MAX_SCRIPTS && (len = ReadLine(buffer, BUFLEN,&instfile)) > 0) 
	{
		buffer[len] = '\0';
		p = buffer;
		while ( *p != '\0' ) 
			if(*p == '#'){
				*p = '\0';
				break;
			}
			else p++;
		len = strlen(buffer);
		if(buffer[len - 1] == '\n' || buffer[len - 1] == FDLMT){
			buffer[len - 1] = '\0';
			len--;
			}
		if(len < 6) {
			if(debug_is_on)
				PrintStrings("Error 1: in ", buffer, 0);
			continue;
		}
		p = buffer;
		while(*p != '\0' && *p != FDLMT) p++;
		if(*p == '\0') {
			if(debug_is_on)
				PrintStrings("Error 2: in ", buffer, 0);
			continue;
		}
		*p++ = '\0';

		ExitCodePtr = p;	
		while(*p != '\0' && *p != FDLMT) p++;
		if(*p == '\0') {
			if(debug_is_on)
				PrintStrings("Error 3: in ", buffer, 0);
			continue;
			}
		*p++ = '\0';

		CommandTypePtr = p;	
		while(*p != '\0' && *p != FDLMT) p++;
		if(*p == '\0') {
			if(debug_is_on)
				PrintStrings("Error 4: in ", buffer, 0);
			continue;
			}
		*p++ = '\0';

		ptr = strip_string(buffer);
		len = strlen(ptr) + 1;
		if(len < 2)
			continue;
		script[nscripts].program_path = (char *) malloc(len);
		CopyStrings(script[nscripts].program_path, ptr, NULL); 
		parse_args(&script[nscripts]);

		ptr = strip_string(ExitCodePtr);
		script[nscripts].program_class = Ascii2Integer(ptr);

		ptr = strip_string(CommandTypePtr);
		switch(*ptr) {
		case  't':
		case  'T':
			script[nscripts].program_type = OPENTTY;
			break;
		case  'n':
			script[nscripts].program_type = NEWPHASE;
			break;
		case  'S':
			script[nscripts].program_type = SETCOLOR;
			break;
		case  'J':
		case  'j':
			script[nscripts].program_type = UADMIN;
			break;
		case  'x':
		case  'X':
			script[nscripts].program_type = CHROOT;
			break;
		case  'm':
		case  'M':
			script[nscripts].flags = 0;
			script[nscripts].program_type = *ptr++;
			while(*ptr){
				if(*ptr == 'r' || *ptr == 'R')
					script[nscripts].flags |= MS_RDONLY;
				else if(*ptr == 'p' || *ptr == 'P')
					script[nscripts].flags |= PROMPT_FIRST;
				else if(*ptr == 'c' || *ptr == 'C')
					script[nscripts].flags |= MUST_MOUNT;
				else if(*ptr == 'l' || *ptr == 'L')
					script[nscripts].flags |= SYM_LINK;
				ptr++;
			}
			break;

		case  'd':
		case  'D':
			script[nscripts].program_type = REMOVESWAP;
			break;

		case  'U':
		case  'u':
			script[nscripts].program_type = UMOUNT_CMD;
			break;

		case  'e':
		case  'E':
			script[nscripts].program_type = ENVIRONMENT;
			break;

		case  'o':
			script[nscripts].program_type = DEBUG_OFF;
			break;

		case  'O':
			script[nscripts].program_type = DEBUG_ON;
			break;

		case  'c':
		case  'C':
			script[nscripts].program_type = *ptr;
			break;

		case  'i':
		case  'I':
			script[nscripts].program_type = INTERRUPT;
			interrupt = &script[nscripts];
			break;

		case  'r':
			script[nscripts].program_type = REMOVE_FILE;
			break;
		case  'R':
			script[nscripts].program_type = RESUME;
			resume_script = &script[nscripts];
			break;

		case  'b':
		case  'B':
			script[nscripts].program_type = LOAD_MOUNT;
			break;
		case  'l':
			script[nscripts].flags = 0;
			script[nscripts].program_type = *ptr++;
			while(*ptr){
				if(*ptr == 'r' || *ptr == 'R')
					script[nscripts].flags |= REGISTER_ONLY;
				else if(*ptr == 'p' || *ptr == 'P')
					script[nscripts].flags |= PROMPT_FIRST;
				else if(*ptr == 'c' || *ptr == 'C')
					script[nscripts].flags |= MUST_MOUNT;
				ptr++;
			}
			break;

		default :
			if(debug_is_on)
				PrintStrings("Error 5 : unknown cmd type ",
					 buffer, 0);
			free(script[nscripts].program_path);
			continue;
		}

		len = strlen(p) + 1;
		if(len > 1) {
			script[nscripts].program_name = (char *) malloc(len);
			CopyStrings(script[nscripts].program_name, p, NULL); 
			}
		else	script[nscripts].program_name = NULL;
		nscripts++;	
	}

	close(file);
}

ExecuteSteps(class)
int	class;
{
	char	 buffer[BUFLEN];
	current_script = 0;
	
	while(current_script < nscripts) 
	{

		if(script[current_script].program_class != class) {
			current_script++;
			continue;
		}
		CopyStrings(buffer, "SCRIPTNO=", Integer2Ascii(current_script), 0); 
		putenv(buffer);

		switch(script[current_script].program_type) 
		{
			case	REMOVE_FILE:
				if(debug_is_on)
					PrintStrings("removing ",
                                        	script[current_script].program_path, 0);
				if(unlink(script[current_script].program_path) < 0)
					PrintStrings("cannot remove ",
                                        	script[current_script].program_path, 
						" errno is ",
						Integer2Ascii(errno), 0);
				current_script++;
				break;
			case	OPENTTY:
				OpenControllingTerminal(&script[current_script]);
				current_script++;
				break;
			case	DEBUG_ON:
				debug_is_on = 1;
				current_script++;
				break;
			case	DEBUG_OFF:
				debug_is_on = 0;
				current_script++;
				break;
			case	NEWPHASE:
				ParseScripts(script[current_script].program_path);
				restarting = 1;
				return;

			case	SETCOLOR:
				{
					int	n;

					CopyStrings(fcolor, 
						script[current_script].program_path, 0);
					CopyStrings(bcolor, 
						script[current_script].argv[1], 0);

					current_script++;

					PrintStrings("Color is set to ",
							 Integer2Ascii(color_monitor), 0);
					if(color_monitor){
						set_fb_color();
					}
				}
				break;
			case UADMIN:
			{
				int	cmd, func;
			
				cmd = Ascii2Integer(script[current_script].program_path);
				func = Ascii2Integer(script[current_script].argv[1]);
				uadmin(cmd, func);

				current_script++;
			}
			break;
			case CHROOT: 
			{
				char *newroot =strip_string(
					script[current_script].program_path);

				current_script++;
				if(debug_is_on)
					PrintStrings("TRYING chroot to ", newroot, 0);

     				if(chroot(newroot) < 0) {
					PrintStrings("chroot to ", newroot, 
						" FAILED: errno ", 
							Integer2Ascii(errno), 0);
					ShutDown(); 
				}
				if(debug_is_on)
					PrintStrings("TRYING chdir to / ", newroot, 0);
		        	if(chdir("/") < 0) {
					PrintStrings("chroot: chdir(/) FAILED:errno ",
					 	Integer2Ascii(errno), 0);
					ShutDown();
				}
			}
			break;
			case RESUME:
				resume_script = &script[current_script];
				current_script++;
			break;

			case INTERRUPT:
				interrupt = &script[current_script];
				current_script++;
				setallsig();
			break;

			case REMOVESWAP: 
			{
				char *dev = strip_string(
					script[current_script].program_path);
				RemoveSwapDevice(dev);
				current_script++;
			}
			break;

			case ENVIRONMENT: 
			{
				char *env = strip_string(
					script[current_script].program_path);
				putenv(env);
				current_script++;
			}
			break;

			case	LOAD_CMD:
				LoadHbaModules(&script[current_script]); 
				current_script++;
			break;
			case	LOAD_MOUNT:
				PromptHbaMountSecondDisk(&script[current_script]); 
				current_script++;
			break;

			case MOUNT_CMD: 
			MountAndLink(&script[current_script]);
			current_script++;
			break;

			case  UMOUNT_CMD:
				if(debug_is_on)
					PrintStrings("Calling UmountAndUnlink ", 0);
				UmountAndUnlink(&script[current_script]);
				current_script++;
				if(debug_is_on)
					PrintStrings("Called UmountAndUnlink ", 0);
			break;

			case  COMMAND_NO_INTERRUPT:
			case  COMMAND:
			{
				int	i = 0, status;
				char	*p;

				if(debug_is_on)
				   while (script[current_script].argv[i] != 0) {
					PrintStrings("argv[", Integer2Ascii(i),
						 "] is ",
						script[current_script].argv[i], 0);
					i++;
				   }

				i = current_script++;
	
				status = spawn_script(&script[i], i);
				p = script[current_script].argv[1];
				if(debug_is_on)
					PrintStrings("status for command ", 
						script[current_script].argv[0],
						p == NULL? p : " ", " is ",
						Integer2Ascii(status), 0);

				if(status == RESTART){
					restarting = 1;
					current_script = 0;
					return;
				}
			}
			break;
			default:
			{
				char	buf[2];

				buf[0] = script[current_script++].program_type;
				buf[1] = '\0';
				PrintStrings("UNKOWN type ", buf, 0);
				break;
			}
		}
	}
}

ColorMonitor()
{
	int fd;
	struct kd_disparam kd_d;
	struct kd_vdctype kd_v;

	errno = 0;
	if ((fd=open("/dev/video", O_RDWR)) < 0 ) {
		if(debug_is_on)
			PrintStrings("Cannot open /dev/video errno ", 
						Integer2Ascii(errno), 0);
		return(0); /* non-integral console */
	}

	if (ioctl(fd,KDDISPTYPE, &kd_d)  < 0 ){
		if(debug_is_on)
			PrintStrings("ioctl KDDISPTYPE errno ", Integer2Ascii(errno), 0);
		close(fd);
		return(0);
		}

	if (kd_d.type == KD_VGA) {
		if (ioctl(fd,KDVDCTYPE, &kd_v)  == -1 ) {
			if(debug_is_on)
				PrintStrings("ioctl KDVDCTYPE errno ", 
							Integer2Ascii(errno), 0);
			close(fd);
			return(0);
		}
		close(fd);
		switch(kd_v.dsply ) {
		case KD_MULTI_C:
		case KD_STAND_C:
			return(1);
		default:
			if(debug_is_on)
				PrintStrings("Monochrome monitor", 0);
			return(0);
		}
	}
	if(debug_is_on)
		PrintStrings("Not a VGA monitor", 0);
	close(fd);
	return(0);
}

cat_file(filename)
char	*filename;
{
	int	fd, n;
	char	*clear_screen = "\033[2J\033[H";
	char	buf[512];

	if(filename == NULL)
		return;

	write(1, clear_screen, strlen(clear_screen));

	if((fd = open(filename, O_RDONLY)) < 0) {
		if(debug_is_on)
			PrintStrings("Cannot open message file ", filename, 0);
			return;
		}

	while((n = read(fd, buf, 512)) > 0)
		write(1, buf, n);

	close(fd);
}

set_fb_color()
{
	if(color_monitor == 1)
		PrintStrings("\033[0m\033[=0E\033[=",
		fcolor, "F\033[=", bcolor,
		"G\033[0m\033[J\033[7m\033[m", 
		"\033[2J\033[H", /*clear screen*/
		0);
}
