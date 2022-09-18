#ident	"@(#)debugger:libol/common/Sel_list.C	1.4"

#include "UI.h"
#include "Component.h"
#include "Sel_list.h"
#include "Vector.h"
#include "str.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

#define CHUNK_SIZE	15

static String fields[] =
{
	XtNformatData,
};

struct Item_data
{
	const char **strings;
};

static void
select_CB(Widget, Selection_list *list, OlFlatCallData *ptr)
{
	if (!ptr || !list)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	(list->get_select_cb())(list->get_creator(), list,
		(void *)ptr->item_index);
}

static void
deselect_CB(Widget, Selection_list *list, OlFlatCallData *ptr)
{
	if (!ptr || !list)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	(list->get_deselect_cb())(list->get_creator(), list,
		(void *)ptr->item_index);
}

static void
drop_CB(Widget w, Selection_list *list, OlFlatDropCallData *ptr)
{
	Widget		destination;
	Component	*component = 0;

	if (!list || !list->get_drop_cb())
		return;

	destination = XtWindowToWidget(XtDisplay(w), ptr->dst_info->window);

	XtVaGetValues(destination, XtNuserData, &component, 0);
	while (!component)
	{
		destination = XtParent(destination);
		if (!destination)
			break;
		XtVaGetValues(destination, XtNuserData, &component, 0);
	}
	(list->get_drop_cb())(list->get_creator(), list, component);
}

static void
overflow_CB(Widget, Selection_list *list, OlFListItemsLimitExceededCD *cdp)
{
	display_msg(E_ERROR, GE_list_overflow);
	list->set_overflow(cdp->preferred);
	cdp->ok = TRUE;
}

void
Selection_list::set_overflow(int new_size)
{
	total_items = new_size;
	overflow = TRUE;
}

Selection_list::Selection_list(Component *p, const char *s, int rows, int cols,
	const char *format, int total, const char **ival, Select_mode m, void *c,
	Callback_ptr sel_cb, Callback_ptr desel_cb, Callback_ptr cb, Help_id h)
	: COMPONENT(p, s, c, h)
{
	if (!ival)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	drop_cb = cb;
	select_cb = sel_cb;
	deselect_cb = desel_cb;

	columns = cols;
	total_items = total;
	visible_items = rows;
	pointers = 0;
	item_data = 0;
	overflow = FALSE;
	allocate_pointers(total_items, columns);

	for (int i = 0; i < total_items; i++)
	{
		const char **ptr = item_data[i];
		for (int j = 0; j < columns; j++, ival++)
		{
			ptr[j] = makestr(*ival);
		}	
	}

	widget = XtVaCreateManagedWidget(label,
		scrolledWindowWidgetClass, parent->get_widget(),
		XtNuserData, this,
		0);

	list = XtVaCreateManagedWidget(label, flatListWidgetClass, widget,
		XtNformat, format,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNitems, item_data,
		XtNnumItems, total_items,
		XtNnoneSet, TRUE,
		XtNexclusives, (m == SM_single) ? TRUE : FALSE,
		XtNclientData, this,
		XtNuserData, this,
		XtNviewHeight, visible_items,
		0);

	XtAddCallback(list, XtNitemsLimitExceeded, 
		(XtCallbackProc)overflow_CB,
		this);

	if (drop_cb)
		XtVaSetValues(list, XtNdropProc, (XtCallbackProc)drop_CB, 0);
	if (select_cb)
		XtVaSetValues(list, XtNselectProc, (XtCallbackProc)select_CB, 0);
	if (deselect_cb)
		XtVaSetValues(list, XtNunselectProc, (XtCallbackProc)deselect_CB, 0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Selection_list::~Selection_list()
{
	delete pointers;
	delete item_data;
}

void
Selection_list::allocate_pointers(int rows, int columns)
{
	char	***olddata;
	char	**oldptr;
	char	**ptr;
	int	i;

	olddata = (char ***)item_data;
	oldptr = pointers;
	if (rows)
	{
		pointers = new char *[rows * columns];
		item_data = new char **[rows];
		ptr = pointers;
		for (i = 0; i < rows; i++)
		{
			item_data[i] = ptr;
			ptr += columns;
		}
	}
	else
	{
		pointers = 0;
		item_data = 0;
	}

	for (i = 0; i < total_items * columns; i++)
		delete oldptr[i];
	delete oldptr;
	delete olddata;
}

void
Selection_list::set_list(int new_total, const char **values)
{
	if (new_total != total_items)
	{
		allocate_pointers(new_total, columns);
		total_items = new_total;
	}
	for (int i = 0; i < total_items; i++)
	{
		const char **ptr = item_data[i];
		for (int j = 0; j < columns; j++, values++)
		{
			ptr[j] = makestr(*values);
		}	
	}

	XtVaSetValues(list,
		XtNitemsTouched, TRUE,
		XtNnumItems, total_items,
		XtNitems, item_data,
		0);
	if (overflow)
	{
		// flatList overflow. by now, 'total_items' has been
		// reset to a smaller number. so, truncate the list in 
		// 'item_data' to this new size.
		char **ptr = pointers;
		delete item_data;
		pointers = new char *[total_items*columns];
		memcpy(pointers, ptr, total_items*columns*sizeof(char*));
		delete ptr;
		item_data = new char **[total_items];
		ptr = pointers;
		for (int i = 0; i < total_items; ++i)
		{
			item_data[i] = ptr;
			ptr += columns;
		}
		XtVaSetValues(list,
			XtNitems, item_data,
			0);
		overflow = FALSE;
	}
        
}

static Vector vector;

int
Selection_list::get_selections(int *&sel)
{
	int nsels = 0;

	vector.clear();
	for (int i = 0; i < total_items; i++)
	{
		Boolean is_set = FALSE;
		OlVaFlatGetValues(list, i, XtNset, &is_set, 0);
		if (is_set)
		{
			vector.add(&i, sizeof(int));
			nsels++;
		}
	}
	sel = (int *)vector.ptr();
	return nsels;
}

const char *
Selection_list::get_item(int row, int column)
{
	if (row >= total_items || column >= columns)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;
	}

	return item_data[row][column];
}

void
Selection_list::select(int row)
{
	OlVaFlatSetValues(list, row, XtNset, TRUE, 0);
}

void
Selection_list::deselect(int row)
{
	OlVaFlatSetValues(list, row, XtNset, FALSE, 0);
}
