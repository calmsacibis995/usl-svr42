#ident	"@(#)debugger:libol/common/Window_sh.C	1.10"

#include "UI.h"
#include "Component.h"
#include "Window_sh.h"
#include "Windows.h"
#include "Message.h"
#include "Msgtab.h"
#include <stdio.h>

#include <X11/Shell.h>
#include <Xol/RubberTile.h>
#include <Xol/StaticText.h>
#include <Xol/OlCursors.h>

// icon pixmap data
#include "debug.pmp"

#ifdef __cplusplus
extern "C" {
#endif
	extern Pixmap XCreatePixmapFromData(	// defined in xpm.c
		Display *, 	/* display */
		Drawable,	/* drawable */ 
		Colormap,	/* colormap */
		unsigned int,	/* width */
		unsigned int,	/* height */
		unsigned int,	/* depth */
		unsigned int,	/* ncolors */
		unsigned int,	/* chars_per_pixel */
		char **,	/* colors */
		char **,	/* pixels */
		Pixel		/* bg_pix */
	);
#ifdef __cplusplus
}
#endif

// catch the message from the window manager when the user selects
// quit in the window menu, and exit gracefully
static void
wm_cb(Widget w, XtPointer, XtPointer cdata)
{
	OlWMProtocolVerify	*protocol = (OlWMProtocolVerify *)cdata;

	if (protocol->msgtype == OL_WM_DELETE_WINDOW)
		((Window_set *)windows.first())->ok_to_quit();
	else
		OlWMProtocolAction(w, protocol, OL_DEFAULTACTION);
}

Window_shell::Window_shell(const char *s, Callback_ptr d, Base_window *c,
	Help_id h) : COMPONENT(0, s, c, h, WINDOW_SHELL)
{
	char	title[100];

	int	id = c->get_window_set()->get_id();
	if (id > 1)
		sprintf(title, "Debug %d: %s", id, label);
	else
		sprintf(title, "Debug: %s", label);

	child = focus = 0;
	dismiss = d;
	busy_count = 0;
	msg_widget = 0;
	string = 0;
	errors = 0;

	if (dismiss && !creator)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);

	window_widget = XtVaCreatePopupShell(label, topLevelShellWidgetClass,
		base_widget, XtNuserData, this, XtNtitle, title, 0);

	static Pixmap icon_pixmap;
	if(!icon_pixmap)
	{
		Screen *screen = XtScreen(window_widget);
		Pixel bg_pix;
		XtVaGetValues(window_widget, XtNbackground, &bg_pix, NULL, 0);
		icon_pixmap = XCreatePixmapFromData(XtDisplay(window_widget),
			RootWindowOfScreen(screen),
			DefaultColormapOfScreen(screen),
			debug_width, debug_height, DefaultDepthOfScreen(screen),
			debug_ncolors, debug_chars_per_pixel, 
			debug_colors, debug_pixels, bg_pix);
	}
	XtVaSetValues(window_widget, XtNiconPixmap, icon_pixmap, NULL, 0);

	OlAddCallback(window_widget, XtNwmProtocol, wm_cb, 0);

	// if the rubberTile widget is not available, use a form widget,
	// although you may have to restrict the resizing capabilities
	widget = XtVaCreateManagedWidget(label, rubberTileWidgetClass,
		window_widget,
		XtNuserData, this,
		XtNorientation, OL_VERTICAL,
		XtNtopMargin, PADDING,
		XtNbottomMargin, PADDING,
		XtNrightMargin, PADDING,
		XtNleftMargin, PADDING,
		0);

	if (help_msg)
		register_help(window_widget, label, help_msg);
}

Window_shell::~Window_shell()
{
	XtDestroyWidget(window_widget);	// other components don't need to call
					// XtDestroyWidget since it works recursively
	delete child;
	delete string;
}

void
Window_shell::popup()
{
	if (focus)
		XtVaSetValues(window_widget, XtNfocusWidget, focus->get_widget(), 0);
	XtPopup(window_widget, XtGrabNone);
}

void
Window_shell::popdown()
{
	XtPopdown(window_widget);
}

// raise and set input focus
void
Window_shell::raise()	
{
	Display *dpy = XtDisplay(window_widget);
	Window win = XtWindow(window_widget);

	XMapRaised(dpy, win);
	OlMoveFocus(focus ? focus->get_widget() : window_widget,
		OL_IMMEDIATE, CurrentTime);
}

void
Window_shell::add_component(Component *p)
{
	if (child)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	child = p;

	// if using a form widget instead of rubber tiles, set the Resizable
	// and Attach resources here, also refWidget, AddHeight, and Offset
	// when creating the footer
	XtVaSetValues(p->get_widget(), XtNweight, 1, 0);
	msg_widget = XtVaCreateManagedWidget("footer",
		staticTextWidgetClass, widget,
		XtNuserData, this,
		XtNstring, " ",
		XtNgravity, WestGravity,
		XtNweight, 0,
		XtNrefWidget, p->get_widget(),
		XtNrefSpace, PADDING,
		XtNstrip, FALSE,
		0);
}

void
Window_shell::set_focus(Component *p)
{
	if (focus)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	focus = p;
	XtVaSetValues(window_widget, XtNfocusWidget, p->get_widget(), 0);
}

void
Window_shell::set_busy(Boolean busy)
{
	if (busy)
	{
		if (!busy_count){
			static Cursor busy_cur;
			if(!busy_cur)
				busy_cur = GetOlBusyCursor(XtScreen(window_widget));
			XDefineCursor(XtDisplay(window_widget), 
					XtWindow(window_widget), busy_cur);
			XtVaSetValues(window_widget, XtNbusy, TRUE, 0);
		}
		busy_count++;
	}
	else
	{
		if (!busy_count)
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		busy_count--;
		if (!busy_count){
			XUndefineCursor(XtDisplay(window_widget),
				XtWindow(window_widget));
			XtVaSetValues(window_widget, XtNbusy, FALSE, 0);
		}
	}	
}

void
Window_shell::display_msg(Severity sev, const char *message)
{
	size_t	len = strlen(message);

	if (!string)
		string = new char[BUFSIZ]; // big enough for any readable message

	if (!errors || sev == E_NONE)
	{
		if (len >= BUFSIZ)
			len = BUFSIZ - 1;
		strncpy(string, message, len);
		// chop off ending new line
		if (string[len-1] == '\n')
			len--;
		string[len] = '\0';
	}
	else
	{
		// already displaying a message, append
		size_t	curlen = strlen(string);
		if (len + curlen + 1 >= BUFSIZ)
			len = BUFSIZ - (curlen + 2);
		strcat(string, "\n");
		strncat(string, message, len);
		curlen = curlen+len;
		if (string[curlen-1] == '\n')
			curlen--;
		string[curlen] = '\0';
	}
	if (sev > E_NONE)
		errors++;

	// reset the string
	XtVaSetValues(msg_widget, XtNstring, string, 0);
}

void
Window_shell::display_msg(Severity severity, Gui_msg_id mid, ...)
{
	va_list		ap;

	va_start(ap, mid);
	const char *message = do_vsprintf(gm_format(mid), ap);
	va_end(ap);

	display_msg(severity, message);
}

void
Window_shell::display_msg(Message *msg)
{
	if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		display_msg(msg->get_severity(), msg->format());
	else
		display_msg(E_NONE, msg->format());
}

// get rid of old messages
void
Window_shell::clear_msg()
{
	XtVaSetValues(msg_widget, XtNstring, " ", 0);
	errors = 0;
}
