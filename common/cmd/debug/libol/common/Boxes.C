#ident	"@(#)debugger:libol/common/Boxes.C	1.5"

#include "UI.h"
#include "Component.h"
#include "Boxes.h"
#include <Xol/Form.h>
#include <Xol/RubberTile.h>
#include <Xol/Panes.h>
#include <Xol/Handles.h>

// Boxes do not need to save the creator since they have no callbacks

Packed_box::Packed_box(Component *p, const char *s, Orientation o, Help_id h)
	: COMPONENT(p, s, 0, h)
{
	orientation = o;
	widget = XtVaCreateManagedWidget(label, formWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNshadowThickness, 0,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Packed_box::~Packed_box()
{
	Component *ptr;

	ptr = (Component *)children.first();
	for (; ptr; ptr = (Component *)children.next())
		delete ptr;
}

void
Packed_box::add_component(Component *p)
{
	Component	*sib = (Component *)children.last();
	Arg		args[10];
	int		n = 0;

	if (orientation == OR_horizontal)
	{
		// make resizable in the vertical dimension
		XtSetArg(args[n], XtNyResizable, TRUE); n++;
		XtSetArg(args[n], XtNyAttachBottom, TRUE); n++;
		XtSetArg(args[n], XtNxResizable, FALSE); n++;
		if (sib)
		{
			// position to the right of the previous widget
			XtSetArg(args[n], XtNxRefWidget, sib->get_widget()); n++;
			XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
			XtSetArg(args[n], XtNxOffset, PADDING); n++;
		}
	}
	else
	{
		// make resizable horizontally
		XtSetArg(args[n], XtNxResizable, TRUE); n++;
		XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
		XtSetArg(args[n], XtNyResizable, FALSE); n++;
		if (sib)
		{
			// position below the previous widget
			XtSetArg(args[n], XtNyRefWidget, sib->get_widget()); n++;
			XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
			XtSetArg(args[n], XtNyOffset, PADDING); n++;
		}
	}
	XtSetValues(p->get_widget(), args, n);

	children.add(p);
}

// Expansion box is simple, if Rubber Tiles are available.  Trying to do
// it with a form is more difficult...
Expansion_box::Expansion_box(Component *p, const char *s, Orientation o,
	Help_id h) : COMPONENT(p, s, 0, h)
{
	OlDefine	orient = (o == OR_horizontal) ? OL_HORIZONTAL : OL_VERTICAL;
	orientation = o;

	widget = XtVaCreateManagedWidget(label, rubberTileWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNshadowThickness, 0,
		XtNorientation, orient,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Expansion_box::~Expansion_box()
{
	Component *ptr;

	ptr = (Component *)children.first();
	for (; ptr; ptr = (Component *)children.next())
		delete ptr;
}

void
Expansion_box::add_component(Component *p)
{
	Arg		args[3];
	int		n = 0;

	Component *sib = (Component *)children.last();
	if (sib)
	{
		XtSetArg(args[n], XtNrefWidget, sib->get_widget()); n++;
		XtSetArg(args[n], XtNrefSpace, PADDING); n++;
	}

	// weight of zero means it won't be resized
	XtSetArg(args[n], XtNweight, 0); n++;
	XtSetValues(p->get_widget(), args, n);
	children.add(p);
}

void
Expansion_box::add_elastic(Component *p)
{
	Arg		args[3];
	int		n = 0;

	Component *sib = (Component *)children.last();
	if (sib)
	{
		XtSetArg(args[n], XtNrefWidget, sib->get_widget()); n++;
		XtSetArg(args[n], XtNrefSpace, PADDING); n++;
	}

	// weight of 1 will give this component benefits of resizing
	XtSetArg(args[n], XtNweight, 1); n++;
	XtSetValues(p->get_widget(), args, n);
	children.add(p);
}


// If panes are not available, this can be implemented with a form widget,
// but that would not allow the children to be resized relative to one another
Divided_box::Divided_box(Component *p, const char *s, Help_id h)
	: COMPONENT(p, s, 0, h)
{
	// make a panes widget with handles
	widget = XtVaCreateManagedWidget(label, panesWidgetClass,
		parent->get_widget(),
		// workaround for toolkit bug:
		//   when layoutWidth is also constrained, in the
		//   presence of the scrolledWindow widget, the panes
		//   will shrink, eventually leaving the display area
		//   too small for any line to be visible.
		// layoutWidth's default is OL_MINIMIZE
		//XtNlayoutWidth, OL_IGNORE,
		XtNlayoutHeight, OL_IGNORE,
		XtNshadowThickness, 0,
		0);
	(void)XtCreateManagedWidget("handles", handlesWidgetClass,
		widget, 0, 0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Divided_box::~Divided_box()
{
	Component *ptr;

	ptr = (Component *)children.first();
	for (; ptr; ptr = (Component *)children.next())
		delete ptr;
}


void
Divided_box::add_component(Component *p)
{
	children.add(p);
}
