#ident	"@(#)debugger:libol/common/Text_line.C	1.3"

#include "UI.h"
#include "Component.h"
#include "Text_line.h"
#include "Message.h"
#include <stdlib.h>
#include <Xol/TextField.h>

// getstring CB is called when the user hits return
static void
getstringCB(Widget, Text_line *ptr, OlTextFieldVerify *data)
{
	(*ptr->get_return_cb())(ptr->get_creator(), ptr, data->string);
}

// A Text_line is simply an OpenLook TextField widget

Text_line::Text_line(Component *p, const char *s, const char *text, int width,
	Boolean edit, Callback_ptr ret, Callback_ptr get, Callback_ptr lose,
	void *c, Help_id h) : COMPONENT(p, s, c, h)
{
	return_cb = ret;
	get_sel_cb = get;
	lost_sel_cb = lose;
	editable = edit;
	string = 0;
	if (!width)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		width = 10;
	}

	widget = XtVaCreateManagedWidget(label, textFieldWidgetClass,
		parent->get_widget(), 
		XtNcharsVisible, width,
		XtNsensitive, edit,
		XtNstring, text,
		XtNuserData, this,
		0);

	if (return_cb)
	{
		if (creator)
			XtAddCallback(widget, XtNverification, (XtCallbackProc)getstringCB,
				(XtPointer)this);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	if (!editable)
		XtSetSensitive(widget, FALSE);
	if (help_msg)
		register_help(widget, label, help_msg);
}

Text_line::~Text_line()
{
	delete string;
}

// blank out the display
void
Text_line::clear()
{
	XtVaSetValues(widget, XtNstring, "", 0);
}

// returns the current contents
char *
Text_line::get_text()
{
	delete string;
	XtVaGetValues(widget, XtNstring, &string, 0);
	return string;
}

void
Text_line::set_text(const char *s)
{
	if (!s)
		s = "";
	XtVaSetValues(widget, XtNstring, s, 0);
}

void
Text_line::set_cursor(int pos)
{
	XtVaSetValues(widget, XtNcursorPosition, pos, 0);
	OlMoveFocus(widget, OL_IMMEDIATE, CurrentTime);
}

void
Text_line::set_editable(Boolean e)
{
	if (e == editable)
		return;

	editable = e;
	XtSetSensitive(widget, e);
}
