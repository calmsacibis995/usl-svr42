#ident	"@(#)debugger:libol/common/Dialog_sh.C	1.10"

#include "Help.h"
#include "UI.h"
#include "Component.h"
#include "Dialog_sh.h"
#include "Dialogs.h"
#include "Windows.h"
#include "Message.h"
#include "Msgtab.h"
#include "str.h"
#include <string.h>
#include <stdio.h>
#include <X11/Xatom.h>

#include <Xol/PopupWindo.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/RubberTile.h>
#include <Xol/OlCursors.h>
#include <DnD/OlDnDVCX.h>

struct DNDdata {
	Drop_cb_action callback_action;
	Callback_ptr callback;
	char *drop_content;
	Atom atom;
};

static String fields[] =
{
	XtNlabel,
	XtNmnemonic,
	XtNselectProc,
	XtNclientData,
	XtNdefault,
	XtNsensitive,
};

struct Button_data
{
	XtArgVal label;		// button name
	XtArgVal mnemonic;	// keyboard shortcut
	XtArgVal callback;	// one of Dialog_shell's friend functions
	XtArgVal func;		// framework callback
	XtArgVal default_action; // Apply button
	XtArgVal sensitive;	// TRUE if callback implemented and state ok
};

// the dialog has been dismissed (by the window manager, for example)
static void
dialog_dismissCB(Widget w, Callback_ptr cb, XtPointer)
{
	Dialog_shell	*ptr;

	XtVaGetValues(w, XtNuserData, &ptr, 0);
	ptr->set_is_open(FALSE);

	if (cb && ptr->get_creator())
		(cb)(ptr->get_creator(), ptr, 0);
}

// pop down the dialog (Close button)
static void
dialog_popdownCB(Widget w, XtPointer, XtPointer)
{
	OlDefine	pin;
	Dialog_shell	*dptr;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr)
		return;

	XtVaGetValues(dptr->get_popup_window(), XtNpushpin, &pin, 0);
	if (pin == OL_OUT)
		XtPopdown(dptr->get_popup_window());
}

// invoke the associated framework callback
// if the callback doesn't call wait_for_response() (resetting popdown) the window
// will be popped down when the callback returns
static void
dialog_execCB(Widget w, Callback_ptr func, OlFlatCallData *arg)
{
	Dialog_shell	*dptr;
	int		mne = 0;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr)
		return;

	dptr->set_ok_to_popdown(TRUE);
	if (!func)
	{
		dptr->popdown();
		return;
	}

	dptr->set_busy(TRUE);
	dptr->clear_msg();

	if(arg)
		OlVaFlatGetValues(w, arg->item_index, XtNmnemonic, &mne, 0);
	(*func)(dptr->get_creator(), dptr, (void *)mne);
	if (!dptr->get_cmds_sent())
	{
		dptr->set_busy(FALSE);
		dptr->popdown();
	}
}
	
// invoke the associated framework callback, without closing the window
static void
dialog_non_execCB(Widget w, Callback_ptr func, OlFlatCallData *arg)
{
	Dialog_shell	*dptr;
	int		mne;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr)
		return;

	dptr->set_busy(TRUE);
	dptr->clear_msg();
	dptr->set_ok_to_popdown(FALSE);

	if(arg)
		OlVaFlatGetValues(w, arg->item_index, XtNmnemonic, &mne, 0);
	(*func)(dptr->get_creator(), dptr, (void *)mne);
	if (!dptr->get_cmds_sent())
		dptr->set_busy(FALSE);
}

// invoke the help window
static void
dialog_help(Widget w, XtPointer, XtPointer)
{
	Dialog_shell	*dptr;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr || !dptr->get_help_msg())
		return;

	display_help(dptr->get_widget(), HM_section, dptr->get_help_msg());
}

// get rid of old error messages
void
Dialog_shell::clear_msg()
{
	XtVaSetValues(msg_widget, XtNstring, " ", 0);
	errors = 0;
}

// create the row of buttons at the bottom of the window
// if flat buttons are not available use a controlAreaWidget- the widget
// will have to be created first - before processing the buttons
void
Dialog_shell::make_buttons(const Button *b, int nb)
{
	if (!b)
		return;

	Callback_ptr	apply = 0;
	Button_data	*items = new Button_data[nb];
	Button_data	*item = items;

	for (int i = 0; i < nb; b++, i++)
	{
		XtCallbackProc	cb = 0;
		XtArgVal	is_default = (i == 0);
		const char	*name;
		unsigned char	mne;

		switch (b->type)
		{
		// OK and Apply are the same for OpenLook,
		// since the window may be pinned
		case B_ok:
		case B_apply:
			if (apply)	// already saw B_ok or B_apply
				continue;
			name = b->label ? b->label : "Apply";
			apply = b->func;
			if (b->func)
				cb = (XtCallbackProc)dialog_execCB;
			break;

		// Close dismisses the popup, without changing any controls
		case B_close:
			name = b->label ? b->label : "Close";
			cb = dialog_popdownCB;
			break;

		// Cancel is equivalent to Reset followed by Close
		case B_cancel:
			name = b->label ? b->label : "Cancel";
			cb = (XtCallbackProc)dialog_execCB;
			break;

		// Reset the dialog to its state when last popped up
		case B_reset:
			name = b->label ? b->label : "Reset";
			if (b->func)
				cb = (XtCallbackProc)dialog_non_execCB;
			break;

		// execute the associated action then dismiss the window
		case B_exec:
			name = b->label ? b->label : "Apply";
			if (b->func)
				cb = (XtCallbackProc)dialog_execCB;
			break;

		// execute the associated action without dismissing the window
		case B_non_exec:
			name = b->label;
			if (b->func)
				cb = (XtCallbackProc)dialog_non_execCB;
			break;

		case B_help:
			if (!help_msg)
				continue;
			name = "Help";
			cb = (XtCallbackProc)dialog_help;
			break;

		default:
			break;
		}
		mne = b->mnemonic ? b->mnemonic : name[0];

		item->callback = (XtArgVal)cb;
		item->func = (XtArgVal)b->func;
		item->label = (XtArgVal)name;
		item->mnemonic = mne;
		item->default_action = is_default;
		item->sensitive = cb ? TRUE : FALSE;
		item++;
	}

	nbuttons = item-items;	// may be less than the value of i
	buttons = items;
	control_area = XtVaCreateManagedWidget("lower control",
		flatButtonsWidgetClass, widget,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNnumItems, nbuttons,
		XtNitems, items,
		XtNweight, 0,
		XtNuserData, this,
		0);
}

// Dialog_shell is implemented as transient shell with box, control area
// (for buttons) and footer children, instead of using the OpenLook
// popup window widget, because, unfortunately, the upper control area
// in a popup window widget doesn't allow for resizing.
// The form and button areas are here, but the message widget is created
// only as needed

Dialog_shell::Dialog_shell(Component *p, const char *s, Callback_ptr d, Dialog_box *c,
	const Button *bptr, int num_buttons, Help_id h, Callback_ptr dcb, 
	Drop_cb_action dcb_act) : COMPONENT(p, s, c, h, DIALOG_SHELL)
{
	char	title[100];

	int	id = ((Dialog_box *)creator)->get_window_set()->get_id();
	if (id > 1)
		sprintf(title, "Debug %d: %s", id, label);
	else
		sprintf(title, "Debug: %s", label);

	dismiss = d;
	ok_to_popdown = FALSE;
	cmds_sent = 0;
	busy_widget = 0;
	child = 0;
	error_string = 0;
	errors = 0;
	is_open = FALSE;
	drop_cb = dcb;
	drop_cb_act = dcb_act;

	popup_window = XtVaCreatePopupShell(label, transientShellWidgetClass,
		parent->get_widget(),
		XtNtransientFor, parent->get_widget(),
		XtNtitle, title,
		XtNpushpin, OL_OUT,
		XtNmenuButton, FALSE,
		XtNuserData, this,
		0);

	XtAddCallback(popup_window, XtNpopdownCallback,
		(XtCallbackProc)dialog_dismissCB, (XtPointer)dismiss);

	// if the rubberTile widget is not available, use a form widget,
	// although you may have to restrict the resizing capabilities
	widget = XtVaCreateManagedWidget(label, rubberTileWidgetClass,
		popup_window,
		XtNuserData, this,
		XtNorientation, OL_VERTICAL,
		XtNleftMargin, PADDING,
		XtNrightMargin, PADDING,
		XtNtopMargin, PADDING,
		XtNbottomMargin, PADDING,
		0);
	make_buttons(bptr, num_buttons);

	msg_widget = XtVaCreateManagedWidget("error widget",
		staticTextWidgetClass, widget,
		XtNuserData, this,
		XtNstring, " ",
		XtNgravity, WestGravity,
		XtNrefWidget, control_area,
		XtNrefSpace, PADDING,
		XtNweight, 0,
		0);

	if (help_msg)
		register_help(popup_window, label, help_msg);
}


Dialog_shell::~Dialog_shell()
{
	XtDestroyWidget(popup_window);
	delete child;
	delete error_string;
	delete buttons;
}

void
Dialog_shell::add_component(Component *box)
{
	if (!box || !box->get_widget() || child)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	child = box;

	XtVaSetValues(box->get_widget(),
		XtNweight, 1,
		XtNrefSpace, PADDING,
		0);

	// buttons go at the bottom, even though created before box
	XtVaSetValues(control_area,
		XtNrefWidget, box->get_widget(),
		XtNrefSpace, PADDING,
		0);
}

// called when selection value (file name) has been obtained
static void
selectionCB(Widget w, XtPointer client_data, Atom * selection, Atom *type, 
	XtPointer value, unsigned long *, int *)
{
	DNDdata *dp = (DNDdata *)client_data;
	Display *dpy = XtDisplay(w);

	if(*type == OL_XA_FILE_NAME(dpy))
	{
		char *fname = (char *)value;

		dp->drop_content = makestr(fname);
		if(dp->callback_action == Drop_cb_popdown)
			dialog_execCB(w,dp->callback,NULL);
		else
			dialog_non_execCB(w,dp->callback,NULL);
		XtFree(fname);
		OlDnDDragNDropDone(w, *selection, 
			XtLastTimestampProcessed(dpy), NULL, NULL);
	}
}

// called when a drop is made from the desktop
static Boolean
triggerNotify(Widget w, Window, Position, Position, Atom selection, 
	Time timestamp, OlDnDDropSiteID, OlDnDTriggerOperation, 
	Boolean, Boolean, XtPointer closure)
{
	DNDdata *dp = (DNDdata *)closure;

	if(!dp)
		return False;	// internal error
	if(dp->drop_content)
	{
		delete dp->drop_content;
		dp->drop_content = NULL;
	}
	XtGetSelectionValue(w, selection, dp->atom, 
		selectionCB, (XtPointer)dp, timestamp);
	return True;
}

// return the item dropped from the desktop
char *
Dialog_shell::get_drop_item()
{
	if(is_drop_site() && drop_data != NULL)
		return ((DNDdata *)drop_data)->drop_content;
	return NULL;
}

void
Dialog_shell::popup()
{
	Display *dpy = XtDisplay(popup_window);
	
	if (is_open){
		Window win = XtWindow(popup_window);

		XRaiseWindow(dpy, win);
		OlMoveFocus(popup_window, OL_IMMEDIATE, CurrentTime);
		return;
	}

	clear_msg();
	is_open = TRUE;
	ok_to_popdown = TRUE;
	XtPopup(popup_window, XtGrabNone);
	if(is_drop_site()){
		DNDdata *dp;

		// register the drop site for desktop
		drop_data = dp = new DNDdata;
		dp->atom = OL_XA_FILE_NAME(dpy);
		dp->callback_action = drop_cb_act;
		dp->callback = drop_cb;
		dp->drop_content = NULL;
		OlDnDRegisterDDI(popup_window,
			OlDnDSitePreviewNone,
			(OlDnDTMNotifyProc)triggerNotify,
			(OlDnDPMNotifyProc)NULL,
			True,	/* interest */
			(XtPointer)dp
		);
	}
}

// turn off the busy indicator, and if the button pushed was an execution
// button and there were no errors, dismiss the window.  If there were
// errors, the window is left up so the user has time to read the error
// message (of course, that means that the user has to do something to
// get rid of the popup)
void
Dialog_shell::popdown()
{
	OlDefine	pin = OL_OUT;
	if (cmds_sent)
		return;

	set_busy(FALSE);
	XtVaGetValues(popup_window, XtNpushpin, &pin, 0);
	XtVaSetValues(popup_window, XtNmenuButton, FALSE, 0);	// OpenLook bug?
	if (pin == OL_OUT && ok_to_popdown && !errors)
	{
		XtPopdown(popup_window);
		if(is_drop_site() && drop_data != NULL)
		{
			DNDdata *dp = (DNDdata *)drop_data;

			if(dp->drop_content)
				delete dp->drop_content;
			delete dp;
		}
	}
}

Boolean
Dialog_shell::is_pinned()
{
	OlDefine	pin = OL_OUT;
	XtVaGetValues(popup_window, XtNpushpin, &pin, 0);
	XtVaSetValues(popup_window, XtNmenuButton, FALSE, 0);	// OpenLook bug?
	if (pin == OL_IN)
		return TRUE;
	return FALSE;
}
	
void
Dialog_shell::cmd_complete()
{
	if (--cmds_sent <= 0)
		popdown();
}

void
Dialog_shell::wait_for_response()
{
	cmds_sent++;
}

// display an error message in the dialog's footer
void
Dialog_shell::error(Severity, const char *message)
{
	size_t	len = strlen(message);

	if (!error_string)
		error_string = new char[BUFSIZ]; // big enough for error messages

	if (!errors)
	{
		if (len >= BUFSIZ)
			len = BUFSIZ - 1;
		strncpy(error_string, message, len);
		// chop off ending new line
		if (error_string[len-1] == '\n')
			len--;
		error_string[len] = '\0';
	}
	else
	{
		// already displaying a message, append
		size_t	curlen = strlen(error_string);
		if (len + curlen + 1 >= BUFSIZ)
			len = BUFSIZ - (curlen + 2);
		strcat(error_string, "\n");
		strncat(error_string, message, len);
		curlen = curlen+len;
		if (error_string[curlen-1] == '\n')
			curlen--;
		error_string[curlen] = '\0';
	}
	errors++;

	// reset the string
	XtVaSetValues(msg_widget, XtNstring, error_string, 0);
}

void
Dialog_shell::error(Severity severity, Gui_msg_id mid, ...)
{
	va_list		ap;

	va_start(ap, mid);
	const char *message = do_vsprintf(gm_format(mid), ap);
	va_end(ap);

	error(severity, message);
}

void
Dialog_shell::error(Message *msg)
{
	if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		error(msg->get_severity(), msg->format());
	else
		error(E_NONE, msg->format());
}

// set_busy sets the busy indicator until the command is completed
// set_busy(FALSE) unsets the busy indicator

void
Dialog_shell::set_busy(Boolean busy)
{
	if (busy)
	{
		if (busy_widget)
			return;
		busy_widget = popup_window;	// can't set busy on flat button
		static Cursor busy_cur;
		if(!busy_cur)
			busy_cur = GetOlBusyCursor(XtScreen(busy_widget));
		XDefineCursor(XtDisplay(busy_widget), XtWindow(busy_widget), busy_cur);
		XtVaSetValues(busy_widget, XtNbusy, TRUE, 0);
	}
	else if (busy_widget)
	{
		XUndefineCursor(XtDisplay(busy_widget), XtWindow(busy_widget));
		XtVaSetValues(busy_widget, XtNbusy, FALSE, 0);
		busy_widget = 0;
	}
}

void
Dialog_shell::set_label(const char *new_label, char mnemonic, const char* p)
{
	char	title[100];
	int	id = ((Dialog_box *)creator)->get_window_set()->get_id();

	if( p == 0 )
	{
		if (id > 1)
			sprintf(title, "Debug: %d: %s", id, new_label);
		else
			sprintf(title, "Debug: %s", new_label);
	}
	else
	{
		if (id > 1)
			sprintf(title, "Debug: %d: %s", id, p);
		else
			sprintf(title, "Debug: %s", p);
	}
	XtVaSetValues(popup_window, XtNtitle, title, 0);

	OlVaFlatSetValues(control_area, 0,
		XtNlabel, new_label,
		XtNmnemonic, mnemonic,
		0);
}

void
Dialog_shell::set_focus(Component *p)
{
	OlMoveFocus(p->get_widget(), OL_IMMEDIATE, CurrentTime);
}
