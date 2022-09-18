#ident	"@(#)debugger:libol/common/Alert_sh.C	1.3"

// GUI headers
#include "Alert_sh.h"
#include "UI.h"

// Debug headers
#include "Message.h"
#include "UIutil.h"

#include <stdarg.h>
#include <Xol/Notice.h>
#include <Xol/FButtons.h>

// Note - buttons are implemented using the Flat Button widget.
// If that is not available, use the Oblong Button widget

// callback for the first, or single, button
static void
alert_ok_CB(Widget, Alert_shell *ptr, XtPointer)
{
	if (!ptr->get_handler() || !ptr->get_object())
		return;
	(*ptr->get_handler())(ptr->get_object(), ptr, (void *)1);
}

// callback for the second button
static void
alert_not_ok_CB(Widget, Alert_shell *ptr, XtPointer)
{
	if (!ptr->get_handler() || !ptr->get_object())
		return;
	(*ptr->get_handler())(ptr->get_object(), ptr, 0);
}

static void
alert_popdown_CB(Widget, Alert_shell *ptr, XtPointer)
{
	delete ptr;
}

// fields and Button_data MUST be in sync
static String fields[] =
{
	XtNlabel,	// button label
	XtNselectProc,	// button callback
};

struct Button_data
{
	XtArgVal	label;
	XtArgVal	callback;
};

// data used to initialize the buttons.  label fields are filled in
// with the action/no_action strings in the constructor
static Button_data button_data[] =
{
	{ 0,	(XtArgVal)alert_ok_CB },
	{ 0,	(XtArgVal)alert_not_ok_CB },
};

Alert_shell::Alert_shell(const char *string, const char *action, const char *no_action,
	Callback_ptr h, void *obj) : COMPONENT(0, "notice", 0, HELP_none)
{
	Widget		text;
	Widget		control;
	Widget		buttons;
	int		nbuttons = no_action ? 2 : 1;

	button_data[0].label = (XtArgVal)action;
	button_data[1].label = (XtArgVal)no_action;
	handler = h;
	object = obj;
	widget = XtVaCreatePopupShell("notice", noticeShellWidgetClass,
		base_widget, XtNuserData, this, 0);
	XtAddCallback(widget, XtNpopdownCallback, (XtCallbackProc)alert_popdown_CB,
		(XtPointer)this);

	XtVaGetValues(widget, XtNtextArea, &text, XtNcontrolArea, &control, 0);
	XtVaSetValues(text, XtNstring, string, 0);

	buttons = XtVaCreateManagedWidget("buttons",
		flatButtonsWidgetClass, control,
		XtNnumItems, nbuttons,
		XtNitems, button_data,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNuserData, this,
		XtNclientData, this,
		0);

	XtPopup(widget, XtGrabExclusive);
}

Alert_shell::~Alert_shell()
{
	XtDestroyWidget(widget);
}
