#ident	"@(#)debugger:gui.d/common/main.C	1.3"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#include "Msgtypes.h"
#include "Message.h"
#include "Msgtab.h"
#include "Transport.h"
#include "Severity.h"
#include "UIutil.h"

#include "Context.h"
#include "UI.h"
#include "Dispatcher.h"
#include "Windows.h"

extern Transport	transport;
const char		*prompt = "debug> ";
const char		*msg_internal_error;

typedef void (*PFVV)();
extern PFVV	set_new_handler(PFVV);

void
internal_error(int sig)
{
	dispatcher.send_msg(0, 0, "quit\n");
	write(2, msg_internal_error, strlen(msg_internal_error));
	exit(sig);
}

void
new_handler()
{
	interface_error("new", __LINE__, 1);
}

void
main(int argc, char **argv)
{
	(void) setlocale(LC_MESSAGES, "");
	(void) set_new_handler(new_handler);

	msg_internal_error = Mtable.format(ERR_cannot_recover);
	signal(SIGQUIT, internal_error);
	signal(SIGILL, internal_error);
	signal(SIGTRAP, internal_error);
	signal(SIGIOT, internal_error);
	signal(SIGEMT, internal_error);
	signal(SIGFPE, internal_error);
	signal(SIGBUS, internal_error);
	signal(SIGSEGV, internal_error);
	signal(SIGSYS, internal_error);
	signal(SIGXCPU, internal_error);
	signal(SIGXFSZ, internal_error);

	(void) init_gui("debug", "debug", &argc, argv);

	// let the debugger know the gui is alive and well and ready to start
	write(fileno(stdout), "1", 1);

	(void) new Window_set();

	// Main loop - never returns
	toolkit_main_loop();
}
