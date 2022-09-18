#ident	"@(#)debugger:gui.d/common/util.C	1.7"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "Vector.h"
#include "Buffer.h"
#include "str.h"
#include "Msgtab.h"
#include "UIutil.h"
#include "Dispatcher.h"
#include "Proclist.h"
#include "Windows.h"
#include "Window_sh.h"
#include "Sender.h"
#include "UI.h"
#include "gui_msg.h"

Vector	vscratch1;
Vector	vscratch2;
Buffer	buffer1;
int	Nsignals;
int	Nsyscalls;

static char **mutate(char **, char**);

int
alpha_comp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

const char **
get_signals(Order order)
{
	static char	**alpha_names;
	static char	**numeric_names;
	static char	**mutated_names;

	if (!alpha_names)
	{
		Message	*msg;
		int	i = 0;

		alpha_names = new char *[NSIG];
		numeric_names = new char *[NSIG];
		mutated_names = new char *[2*NSIG];
		alpha_names[NSIG-1] = numeric_names[NSIG-1] = 0;

		for (int j = 0; j < 2*NSIG - 1; j++)
			mutated_names[j] = new char [30];

		dispatcher.query(0, 0, "help signames\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			char	*buf;
			Word	sig;

			Msg_id mid = msg->get_msg_id();
			if (mid == MSG_signame)
			{
				msg->unbundle(sig, buf);
				numeric_names[i] = new char[10];
				(void) sprintf(numeric_names[i], "%-6s %-2d ",
					buf, sig);
				alpha_names[i] = numeric_names[i];
				i++;
			}
			else if (mid == MSG_help_hdr_sigs
				|| mid == MSG_newline) // no action needed
				continue;
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		}

		qsort(alpha_names, NSIG-1, sizeof(char *), alpha_comp);
		Nsignals = NSIG-1;
	}

	if (order == Alpha)
		return mutate(alpha_names, mutated_names);
	else
		return mutate(numeric_names, mutated_names);
}

char **
mutate(char **input, char **output)
{
	int i = 0;
	int j = 0;
	static char buf[BUFSIZ];
	while(input[i] != NULL){
		char *p;
		strcpy(buf, input[i]); //strtok on copy to avoid trashing input
		p = strtok(buf, " ");
		while(p != NULL){
			strcpy(output[j++], p);
			p = strtok(NULL, " ");
		}
		i++;
	}
	return output;
}

const char **
get_syslist(Order order)
{
	static char	**alpha_names;
	static char	**numeric_names;
	static char	**mutated_names;
	static int		i = 0;

	if (!alpha_names)
	{
		Message	*msg;

		alpha_names = new char *[NSYS];
		numeric_names = new char *[NSYS];
		mutated_names = new char *[2*NSYS];

		for(int j = 0; j < 2*NSYS; j++)
			mutated_names[j] = new char[18];

		dispatcher.query(0, 0, "help sysnames\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			char	*buf;
			Word	sys;

			Msg_id mid = msg->get_msg_id();
			if (mid == MSG_sys_name)
			{
				msg->unbundle(sys, buf);
				numeric_names[i] = new char[18];
				(void) sprintf(numeric_names[i], "%-13s %-3d ",
					buf, sys);
				alpha_names[i] = numeric_names[i];
				i++;
			}
			else if (mid == MSG_help_hdr_sys
				|| mid == MSG_newline)	// not displayed
				continue;
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			if (i >= NSYS)
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				break;
			}
		}
		alpha_names[i] = numeric_names[i] = 0;

		qsort(alpha_names, i, sizeof(char *), alpha_comp);
		Nsyscalls = i;
	}

	if (order == Alpha)
		return mutate(alpha_names, mutated_names);
	else
		return mutate(numeric_names, mutated_names);
}

void
Command_sender::de_message(Message *)
{
}

void
Command_sender::cmd_complete()
{
}

void
debug(const char *fmt ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
interface_error(const char *func, int line, int quit)
{
	char	catid[sizeof(GUICATALOG) + sizeof(":1") + 1];

	strcpy(catid, GUICATALOG);
	if (quit)
	{
		strcat(catid, ":1");
		fprintf(stderr, gettxt(catid,
			"Fatal error: Internal error in %s at line %d\n"),
			func, line);
		ui_exit(1);
	}
	else
	{
		strcat(catid, ":2");
		fprintf(stderr, gettxt(catid,
			"Internal error in %s at line %d, displayed information may be suspect\n"),
			func, line);
	}
}

void
ui_exit(int exit_code)
{
	exit(exit_code);
}

Component::~Component()
{
	// interesting work is done in derived classes, as needed
}

Base_window *
Component::get_base()
{
	Component	*component = this;

	while (component && component->get_type() != WINDOW_SHELL)
		component = component->get_parent();

	if (!component)
		return 0;
	return (Base_window *)component->get_creator();
}

char *
do_vsprintf(const char *format, va_list ap)
{
	static char		*buf;
	static size_t		bufsiz;

	va_list			ap2 = ap;
	register const char	*ptr;
	size_t			len = 1; // allow for '\0' at end

	for (ptr = format; *ptr; ptr++)
	{
		if (*ptr == '%')
		{
			switch(*++ptr)
			{
			case '%':
				len++;
				break;

			case 's':
				len += strlen(va_arg(ap, char *));
				break;

			case 'c':
			case 'd':
			case 'x':
			case 'X':
			case 'o':
			case 'i':
			case 'u':
			case 'p':
				len += 10;	//  enough for maximum digits?
				(void) va_arg(ap, int);
				break;

			default:	// floating point
				len += 20;
				(void) va_arg(ap, double);
				break;
			}
		}
	}

	if (len > bufsiz)
	{
		delete buf;
		bufsiz = len + 200;	// room to expand
		buf = new char[bufsiz];
	}

	vsprintf(buf, format, ap2);
	return buf;
}

const char *
gm_format(Gui_msg_id msg)
{
	char	buf[sizeof(GUICATALOG) + 10];	// enough space for catalog:number

	if ((msg <= GM_none) || (msg >= GM_last) || !gmtable[msg].string)
		interface_error("gm_format", __LINE__);

	// return the cached string, if possible
	register GM_tab	*mptr = &gmtable[msg];
	if (mptr->istring)
		return mptr->istring;

	// read the string from the message database and cache it
	sprintf(buf, "%s:%d", GUICATALOG, mptr->catindex);
	mptr->istring = gettxt(buf, mptr->string);
	return mptr->istring;
}
#include <sys/types.h>
#include <sys/procfs.h>
#include <fcntl.h>
#include <dirent.h>
#include <memory.h>
int
do_ps(char ***res)
{
	//
	//This routine gets the 'ps' information, i.e.
	//pid and command string
	//by reading /proc. It returns an array suitable for
	//input to the scrolling list constructors, that is
	//two line pairs, the first element is the
	//pid, the second the command string itself

	//the /proc directory is read, entry by entry
	//each file is opened, the appropriate ioctl is performed
	//and the uid's matched. If they match, that process is added to the
	//return array. 

	//this routine uses the vector class as a variable-sized
	//array handler.
	//the vector `vscratch1' is used to hold the pairs of strings
	//	pid
	//	command
	//returned by the /proc ioctl. In turn, the vector 'vscratch2'
	//is built using the addresses of the strings in vscratch1, and
	//that's what's returned. If debug is ever multi-threaded, uses
	//of things like these vscratch buffers needs to be made thread-safe

	DIR *dirp;
	int procp;
	struct dirent *direntp;
	prpsinfo_t ps_data; 
	static struct buf {
		char pid[25];
		char args[PRARGSZ + 1];
	} my_buf;
	uid_t my_id = getuid();
	char name[25];

	pid_t	my_pid = getpid();	// gui's pid
	pid_t	parent_pid = getppid();	// debug's pid

	if ((dirp = opendir("/proc")) == NULL)
		abort();
	vscratch1.clear();
	vscratch2.clear();
	while((direntp = readdir(dirp)) != NULL)
	{
		sprintf(name, "/proc/%s", direntp->d_name);
		if ((procp = open(name, O_RDONLY)) != -1)
		{
			if(ioctl(procp, PIOCPSINFO, &ps_data) == 0 &&
				ps_data.pr_uid == my_id)
			{
				// don't let user try to grab gui, debug,
				// follow process, or process already grabbed
				if (ps_data.pr_pid == my_pid
					|| ps_data.pr_pid == parent_pid
					|| (ps_data.pr_ppid == parent_pid &&
						strncmp(ps_data.pr_psargs, "follow ", sizeof("follow ")-1) == 0)
					|| proclist.find_process(ps_data.pr_pid))
					continue;

				memset((char *)my_buf.pid, 0, sizeof(my_buf.pid));
				memset((char *)my_buf.args, 0, sizeof(my_buf.args));
				sprintf(my_buf.pid, "%d", ps_data.pr_pid);
				strcpy(my_buf.args, ps_data.pr_psargs);
				vscratch1.add(&my_buf, sizeof(struct buf));
			}
			close(procp);
		}
	}
	closedir(dirp);
	char *p = (char *)vscratch1.ptr();
	for (int i = 0; i < vscratch1.size() / sizeof(struct buf); i++){
		vscratch2.add(&p, sizeof(char *));
		p += sizeof(my_buf.pid);
		vscratch2.add(&p, sizeof(char *));
		p += sizeof(my_buf.args);
	}
	*res = (char **)vscratch2.ptr();
	return vscratch1.size() / sizeof(struct buf);
}

// gui-only messages are used to update state information and
// should never be displayed directly to the user
Boolean
gui_only_message(Message *m)
{
	switch(m->get_msg_id())
	{
	case MSG_proc_start:
	case MSG_set_language:
	case MSG_set_frame:
	case MSG_event_disabled:
	case MSG_event_enabled:
	case MSG_event_deleted:
	case MSG_event_changed:
	case MSG_bkpt_set:
	case MSG_bkpt_set_addr:
	case MSG_source_file:
	case MSG_new_context:
	case MSG_new_pty:
	case MSG_rename:
	case MSG_cmd_complete:
	case MSG_jump:
	case MSG_script_on:
	case MSG_script_off:
	case MSG_assoc_cmd:
	case MSG_proc_stop_fcall:
		return TRUE;

	default:
		return FALSE;
	}
}
