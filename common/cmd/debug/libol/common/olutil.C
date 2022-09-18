#ident	"@(#)debugger:libol/common/olutil.C	1.9"

// GUI headers
#include "Alert_sh.h"
#include "UI.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "Message.h"
#include "Msgtab.h"
#include "Transport.h"

// Debug headers
#include "str.h"

#include "Buffer.h"
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <Dt/Desktop.h>

Widget                  base_widget;
extern Transport        transport;
char			*Help_path;
OlDefine		gui_mode;

static void
get_msg(XtPointer, int *, XtInputId)
{
	dispatcher.process_msg();
	while(!transport.inqempty())
		dispatcher.process_msg();
}

void
toolkit_main_loop()
{

	// using infinite loop here instead of XtMainLoop to avoid getting
	// backlog of messages in the queue
        for (;;)
	{
		XEvent  event;

		XtNextEvent(&event);
		XtDispatchEvent(&event);

		// clear queue
		while (!transport.inqempty())
			dispatcher.process_msg();
	}
}

Widget
init_gui(const char *name, const char *wclass, int *argc, char **argv)
{
	void parse_args(int *, char ***);

	parse_args(argc, &argv);

	argv[0] = (char *)wclass;
	OlToolkitInitialize(argc, argv, NULL);
	base_widget = XtInitialize(name, wclass, NULL, 0, argc, argv);
	DtInitialize(base_widget);
	gui_mode = OlGetGui();
	XtAddInput(fileno(stdin), (XtPointer) XtInputReadMask,
		(XtInputCallbackProc) get_msg, NULL);

	XtVaSetValues(base_widget, XtNhelpDirectory, "/help/debug", 0);
	char *locale = setlocale(LC_MESSAGES, NULL);
	char *path = getenv("XFILESEARCHPATH");
	if (!path || (path[0] == '/' && path[1] == '\0'))
		path = "/usr/X/lib/locale";
	Help_path = makesf("%s/%s/help/debug", path, locale);

	return base_widget;
}

static char *parse_str;

static void
parse_args(int *argc, char ***argv)
{
	int argc_old;
	char **argv_old;
	char *arg1;
	char *parse_one();
	void argv_init(char *, int);
	void argv_put(char *);
	void argv_get(int *, char ***);

	argc_old = *argc;
	if(argc_old <= 1)
		return;
	argv_old = *argv;
	argv_init(argv_old[0], argc_old*2);
	while(--argc_old > 0){
		parse_str = *++argv_old;
		while(arg1 = parse_one()){
			argv_put(arg1);
		}
	}

	argv_get(argc, argv);
}

static char *
parse_one()
{
	char *s, *rets;

	if(!parse_str)
		return NULL;
	// assume that on entry, parse_str points to the first char
	// of a token
	s = rets = parse_str;
	// find end of token
	if(*s == '"'){
		++rets;
		// skip to enclosing '"'
		for(++s; *s && *s != '"'; ++s)
			if(*s == '\\')
				++s;
	}
	else{
		// skip to first non whitespace
		for(; *s &&  *s != ' ' && *s != '\t'; ++s)
			if(*s == '\\')
				++s;
	}
	if(*s){
		// turn first char past the end of token to NULL
		*s++ = '\0';
		// skip white spaces
		for(; *s == ' ' || *s == '\t'; ++s)
			;
	}
	if(!*s)
		// end of line
		parse_str = NULL;
	else
		// advance parse_str to next token
		parse_str = s;
	return rets;
			
}

static int argv_max;
static int argc_new;
static char **argv_new;

static void
argv_init(char *argv0, int initSz)
{
	argv_max = initSz;
	argv_new = new char *[argv_max];
	argv_new[0] = argv0;
	argc_new = 1;
}

static void
argv_put(char *arg)
{
	if(argc_new >= argv_max) {
		/* expand: double in size */
		char **new_argv = new char *[argv_max+argv_max];
		memcpy(new_argv, argv_new, argv_max*sizeof(char *));
		delete argv_new;
		argv_new = new_argv;
		argv_max += argv_max;
	}
	argv_new[argc_new++] = arg;
}

static void
argv_get(int *argc, char ***argv)
{
	*argc = argc_new;
	*argv = argv_new;
}


void
display_msg(Message *m)
{
	new Alert_shell(m->format(), "OK");
}

void
display_msg(Severity, Gui_msg_id id ...)
{
	va_list	ap;
	char	*msg;

	va_start(ap, id);
	msg = do_vsprintf(gm_format(id), ap);
	va_end(ap);

	new Alert_shell(msg, "OK");
}

void
display_msg(Callback_ptr handler, void *object, const char *action,
	const char *no_action, Gui_msg_id id ...)
{
	va_list	ap;
	char	*msg;

	va_start(ap, id);
	msg = do_vsprintf(gm_format(id), ap);
	va_end(ap);

	new Alert_shell(msg, action, no_action, handler, object);
}

void
beep()
{
	XBell(XtDisplay(base_widget), -1);
}

void
register_help(Widget widget, const char *title, Help_id help)
{
        if(!help)
                return;

        Help_info *help_entry = &Help_files[help];
        OlDtHelpInfo *help_data = (OlDtHelpInfo *)help_entry->data;

        if(!help_data)
        {
                help_entry->data = help_data = new OlDtHelpInfo;

                help_data->title = (String)title;
                help_data->path = (String)Help_path;
                help_data->filename = (String)help_entry->filename;
                help_data->section = (String)help_entry->section;
                help_data->app_title = NULL;
        }

        OlRegisterHelp(OL_WIDGET_HELP, widget, NULL,
                OL_DESKTOP_SOURCE, (XtPointer)help_data);
}

void
display_help(Widget w, Help_mode mode, Help_id help)
{
	DtRequest request;
	Display *dpy = XtDisplay(w);

	if(!help)
		return;
	// extract help entry data
	Help_info *help_entry = &Help_files[help];
	if(!help_entry->data)
		// not registered?
		return;
	// issue request to DTM 
	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	request.display_help.source_type = 
		mode == HM_section ? DT_SECTION_HELP : DT_TOC_HELP;
	request.display_help.app_name = (String)"debug";
	request.display_help.help_dir = (String)Help_path;
	request.display_help.file_name = (String)help_entry->filename;
	request.display_help.sect_tag = (String)help_entry->section;
	DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(dpy),
		_DT_QUEUE(dpy), XtWindow(w), &request);
}
