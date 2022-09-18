#ident	"@(#)debugger:libol/common/Menu.C	1.12"

// GUI headers
#include "UI.h"
#include "Component.h"
#include "Menu.h"
#include "Window_sh.h"
#include "Windows.h"

// Debug headers
#include "str.h"

#include <stdio.h>
#include <string.h>

#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/MenuShell.h>
#include <Xol/RubberTile.h>
#include <X11/RectObj.h>

// Using flat buttons, the menu bar is a flat widget, and the descriptor
// for each button contains a slot for that button's menu pane.
// That menu pane, in turn, holds a flat widget for the menu items
// If flat buttons are not available, each button in the menu bar
// and in each menu is an individual widget

// Menu bar descriptor - bar_fields and Bar_data must be in sync
static String bar_fields[] =
{
	XtNlabel,
	XtNmnemonic,
	XtNpopupMenu,
};

struct Bar_data
{
	XtArgVal	label;		// button label
	XtArgVal	mnemonic;
	XtArgVal	popup;
};

// descriptor for each individual menu - menu_fields and Menu_data must be in sync
static String menu_fields[] =
{
	XtNlabel,
	XtNmnemonic,
	XtNclientData,
	XtNdefault,
	XtNselectProc,
	XtNsensitive,
	XtNpopupMenu,
};

struct Menu_data
{
	XtArgVal	label;		// button label
	XtArgVal	mnemonic;
	XtArgVal	entry;		// Menu_table entry
	XtArgVal	default_action;	// is this the default button?
	XtArgVal	callback;	// callback function
	XtArgVal	sensitive;	// is the operation implemented and valid?
	XtArgVal	popup;
};

// callback when button is selected - the widget's user data is the
// framework object, and cdata is it's callback function

static void
menu_buttonCB(Widget w, const Menu_table *tab, XtPointer)
{
	Menu		*ptr;
	Callback_ptr	func;

	XtVaGetValues(w, XtNuserData, (XtPointer)&ptr, 0);
	func = tab->callback;

	ptr->get_window()->get_window_shell()->clear_msg();
	if (tab->flags == Set_cb)
		(*func)(ptr->get_window_set(), ptr, ptr->get_window());
	else if (tab->flags == Window_cb)
		(*func)(ptr->get_window(), ptr, (void *)tab->cdata);
}

Menu::Menu(Component *p, const char *l, Boolean has_title, Widget pwidget,
	Base_window *w, Window_set *ws, const Menu_table *ptr, int nb, Help_id h)
	: COMPONENT(p, l, 0, h)
{
	window = w;
	window_set = ws;
	nbuttons = nb;
	table = ptr;
	
	OlFlatHelpId	id;
	Menu_data	*menu_data;
	Arg		args[3];
	int		i, n = 0;
	char		title[100];

	delete_table = FALSE;
	if (ws->get_id() > 1)
		sprintf(title, "%s %d", label, ws->get_id());
	else
		strcpy(title, label);

	if (has_title)
	{
		XtSetArg(args[n], XtNpushpin, OL_OUT); n++;
		XtSetArg(args[n], XtNtitle, title); n++;
	}
	XtSetArg(args[n], XtNuserData, this); n++;
	widget = XtCreatePopupShell(title, popupMenuShellWidgetClass, pwidget,
		args, n);

	list = menu_data = new Menu_data[nbuttons];
	for (i = 0; i < nbuttons; ++ptr, ++i)
	{
		menu_data[i].label = (XtArgVal)ptr->label;
		menu_data[i].mnemonic = (XtArgVal)ptr->mnemonic;
		menu_data[i].entry = (XtArgVal)ptr;
		menu_data[i].default_action = (i == 0)
			? (XtArgVal)TRUE : (XtArgVal)FALSE;

		if (ptr->flags == Menu_button)
		{
			// cascading menu - should not have pushpin or title
			Menu *child = new Menu(this, ptr->label, FALSE, pwidget, w, ws,
				(Menu_table *)ptr->callback, ptr->cdata, ptr->help_msg);
			children.add(child);
			menu_data[i].callback = NULL;
			menu_data[i].sensitive = w->is_sensitive(ptr->sensitivity);
			menu_data[i].popup = (XtArgVal)child->get_widget();
		}
		else
		{
			menu_data[i].callback = (XtArgVal)menu_buttonCB;
			menu_data[i].sensitive = (ptr->callback)
				? w->is_sensitive(ptr->sensitivity) : FALSE;
			menu_data[i].popup = NULL;
		}
	}

	menu = XtVaCreateManagedWidget("menu", flatButtonsWidgetClass, widget,
		XtNlayoutType, OL_FIXEDCOLS,
		XtNmeasure, 1,
		XtNitemFields, menu_fields,
		XtNnumItemFields, XtNumber(menu_fields),
		XtNitems, menu_data,
		XtNnumItems, i,
		XtNuserData, this,
		0);

	id.widget = menu;
	for (i = 0, ptr = table; i < nbuttons; i++, ptr++)
	{
                if (ptr->help_msg)
                {
                        Help_info *help_entry = &Help_files[ptr->help_msg];
                        OlDtHelpInfo *help_data = (OlDtHelpInfo *)help_entry->data;

                        if(!help_data)
                        {
                                help_entry->data = help_data = new OlDtHelpInfo;
                                help_data->title = (String)ptr->label;
                                help_data->path = (String)Help_path;
                                help_data->filename = (String)help_entry->filename;
                                help_data->section = (String)help_entry->section ;
                                help_data->app_title = NULL;
                        }
                        id.item_index = i;
                        OlRegisterHelp(OL_FLAT_HELP, &id, NULL,
                                OL_DESKTOP_SOURCE, (XtPointer)help_data);
                }
	}
}

Menu *
Menu::find_item(char *label)
{
	Menu *mp;

	mp = (Menu *)children.first();
	while(mp != NULL){
		if(strcmp(mp->label, label) == 0)
			return mp;
		mp = (Menu *)children.next();
	}
	return NULL;
}

// add an item to end of Menu_data list
void
Menu::add_item(Menu_table *item)
{

	// contiguous Menu_table is needed for check_sensitivity
	Menu_table *new_table = new Menu_table[nbuttons+1];
	memcpy(new_table, table, sizeof(Menu_table) * nbuttons);
	memcpy(&new_table[nbuttons], item, sizeof(Menu_table));
	if (delete_table)
		delete (Menu_table *)table;
	delete_table = TRUE;
	table = new_table;

	Menu_data *new_list = new Menu_data[nbuttons+1];
	memcpy(new_list, list, sizeof(Menu_data)*nbuttons);
	new_list[nbuttons].label = (XtArgVal)item->label;
	new_list[nbuttons].mnemonic = (XtArgVal)item->mnemonic;
	new_list[nbuttons].entry = (XtArgVal)item;
	new_list[nbuttons].default_action = (XtArgVal)FALSE;
	new_list[nbuttons].callback = (XtArgVal)menu_buttonCB;
	new_list[nbuttons].sensitive = (XtArgVal)TRUE;
	new_list[nbuttons].popup = NULL;
	++nbuttons;

	for (int i = 0; i < nbuttons; i++)
		new_list[i].entry = (XtArgVal)&table[i];
	delete list;
	list = new_list;

	XtVaSetValues(menu, 
		XtNitems, new_list, 
		XtNnumItems, nbuttons, 
		NULL, 0);
}

void
Menu::delete_item(char * label)
{
	Menu_data *new_list, *old_list;
	int i;

	old_list = (Menu_data *)list;
	for(i = 0; i < nbuttons; ++i){
		if(strcmp((char *)old_list->label, label) == 0)
			break;
		++old_list;
	}
	if(i >= nbuttons)
		return;
	new_list = new Menu_data[nbuttons-1];
	if(i > 0)
		memcpy(new_list, list, sizeof(Menu_data)*(i));
	if(i < nbuttons-1)
		memcpy(new_list+i, old_list+1, sizeof(Menu_data)*(nbuttons-i-1));
	--nbuttons;
	delete list;
	list = new_list;
	XtVaSetValues(menu, 
		XtNitems, new_list, 
		XtNnumItems, nbuttons, 
		NULL, 0);
}

Menu::~Menu()
{
	for (Menu *ptr = (Menu *)children.first(); ptr; ptr = (Menu *)children.next())
		delete ptr;
	if (delete_table)
		delete (Menu_table *)table;
}

void
Menu::set_sensitive(int button, Boolean value)
{
	OlVaFlatSetValues(menu, button, XtNsensitive, value, 0);
}

Menu_bar::Menu_bar(Component *p, Base_window *w, Window_set *ws,
	const Menu_bar_table *table, int nb, Help_id h) : COMPONENT(p, 0, 0, h)
{
	const Menu_bar_table	*tab;
	int			i;

	bar_table = table;
	nbuttons = nb;
	OlFlatHelpId	id;
	Bar_data	*bar_data;

	// allocate space for the menu bar's descriptor
	list = bar_data = new Bar_data[nbuttons];
	children = new Menu *[nbuttons];
	widget = XtVaCreateManagedWidget("menu", rubberTileWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNorientation, OL_HORIZONTAL,
		XtNshadowThickness, 0,
		0);
	
	for (i = 0, tab = table; i < nbuttons; ++tab, ++i)
	{
		Widget		menupane = 0;

		if (!tab->table)
			continue;
		children[i] = new Menu(this, tab->label, TRUE, widget, w, ws, tab->table,
			tab->nbuttons, tab->help_msg);
		bar_data[i].label = (XtArgVal)tab->label;
		bar_data[i].mnemonic = (XtArgVal)tab->mnemonic;
		bar_data[i].popup = (XtArgVal)children[i]->get_widget();
	}

	Widget menu = XtVaCreateManagedWidget("menu", flatButtonsWidgetClass,
		widget,
		XtNuserData, this,
		XtNitemFields, bar_fields,
		XtNnumItemFields, XtNumber(bar_fields),
		XtNitems, bar_data,
		XtNnumItems, i,
		XtNmenubarBehavior, TRUE,
		XtNgravity, WestGravity,
		0);

	id.widget = menu;
	for (i = 0, tab = table; i < nbuttons-1; ++tab, ++i)
	{
                if (tab->help_msg)
                {
                        Help_info *help_entry = &Help_files[tab->help_msg];
                        OlDtHelpInfo *help_data = (OlDtHelpInfo *)help_entry->data;

                        if(!help_data)
                        {
                                help_entry->data = help_data = new OlDtHelpInfo;
                                help_data->title = (String)tab->label;
                                help_data->path = (String)Help_path;
                                help_data->filename = (String)help_entry->filename;
                                help_data->section = (String)help_entry->section ;
                                help_data->app_title = NULL;
                        }
                        id.item_index = i;
                        OlRegisterHelp(OL_FLAT_HELP, &id, NULL,
                                OL_DESKTOP_SOURCE, (XtPointer)help_data);
                }
	}
}

Menu *
Menu_bar::find_item(char *label)
{
	const Menu_bar_table *tab = bar_table;
	int i;

	for(i = 0; i < nbuttons; ++i){
		if(strcmp(tab->label,label) == 0)
			return children[i];
		++tab;
	}
	return NULL;
}

Menu_bar::~Menu_bar()
{
	for (int i = 0; i < nbuttons; i++)
	{
		delete children[i];
	}
	delete children;
	delete (Bar_data *)list;
}
