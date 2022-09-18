#ident	"@(#)debugger:libol/common/Table.C	1.15"

#include "UI.h"
#include "Component.h"
#include "Table.h"
#include "Windows.h"
#include "str.h"

#include <Xol/FList.h>
#include <Xol/Form.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define	Round(x,y)	(((x)+(y))/(y))*(y)

static String fields[] =
{
	XtNformatData,
	XtNset,
};

static char		*blank_str;

#define hand_format 1
#define hand_width 11
#define hand_height 11
#define hand_ncolors 3
#define hand_chars_per_pixel 1
static char * hand_colors[] = {
" " , "$background",
"." , "#000000000000",
"X" , "#FFFFAAAA0000"
} ;
static char * hand_pixels[] = {
"    .      ",
"   ..      ",
"  ........ ",
" .XXXXXXXX.",
"..XXX..... ",
".XXXXXX.   ",
".XXXX...   ",
"..XXXX.    ",
"  .....    ",
"           ",
"           "
} ;

#define solid_format 1
#define solid_width 11
#define solid_height 11
#define solid_ncolors 1
#define solid_chars_per_pixel 1
static char * solid_colors[] = {
" " , "$background",
} ;
static char * solid_pixels[] = {
"           ",
"           ",
"           ",
"           ",
"           ",
"           ",
"           ",
"           ",
"           ",
"           ",
"           "
} ;

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

struct Glyph_info
{
	Pixmap	pm;
	Pixel	bg;
};

class Glyph_set
{
	Vector	gset;
public:
	Glyph_set()	{ gset.clear(); }
	Glyph_info	*find(Pixel);
	Glyph_info	*find_pm(Pixmap);
	void		add(Pixmap,Pixel);
};

void
Glyph_set::add(Pixmap pm, Pixel bg)
{
	Glyph_info ginfo;
	ginfo.pm = pm;
	ginfo.bg = bg;
	gset.add(&ginfo, sizeof(Glyph_info));
}

Glyph_info *
Glyph_set::find(Pixel bg)
{
	Glyph_info *giP, *glastP;

	giP = (Glyph_info *)gset.ptr();
	glastP = giP + (gset.size()/sizeof(Glyph_info));
	for(; giP < glastP; ++giP)
		if(giP->bg == bg)
			return giP;
	return NULL;
}

Glyph_info *
Glyph_set::find_pm(Pixmap pm)
{
	Glyph_info *giP, *glastP;

	giP = (Glyph_info *)gset.ptr();
	glastP = giP + (gset.size()/sizeof(Glyph_info));
	for(; giP < glastP; ++giP)
		if(giP->pm == pm)
			return giP;
	return NULL;
}

static Glyph_set hands;
static Glyph_set solids;

static void
table_select_CB(Widget w, Table *table, OlFlatCallData *ptr)
{
	Table_calldata	tdata;

	if (!ptr || !table)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (!table->get_select_cb())
		return;

	tdata.index = ptr->item_index;
	tdata.dropped_on = 0;
	if (gui_mode == OL_MOTIF_GUI)
	{
		table->update_glyphs(ptr->item_index, TRUE);
		OlFlatRefreshItem(w, ptr->item_index, TRUE);
	}
	(table->get_select_cb())(table->get_creator(), table, &tdata);
}

static void
table_unselect_CB(Widget w, Table *table, OlFlatCallData *ptr)
{
	if (!table || !table->get_deselect_cb())
		return;
	if (gui_mode == OL_MOTIF_GUI)
	{
		table->update_glyphs(ptr->item_index, FALSE);
		OlFlatRefreshItem(w, ptr->item_index, TRUE);
	}
	(table->get_deselect_cb())(table->get_creator(), table,
		(void *)ptr->item_index);
}

static void
table_drop_CB(Widget w, Table *table, OlFlatDropCallData *ptr)
{
	Widget		destination;
	Component	*component = 0;
	Table_calldata	tdata;

	if (!table || !table->get_drop_cb())
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
	if (component)
	{
		while (component->get_parent() && component->get_type() == OTHER)
			component = component->get_parent();
	}

	tdata.index = ptr->item_data.item_index;
	tdata.dropped_on = component;
	(table->get_drop_cb())(table->get_creator(), table, &tdata);
}

static void
table_overflow_CB(Widget, Table *table, OlFListItemsLimitExceededCD *ptr)
{
	display_msg(E_ERROR, GE_list_overflow);
	table->set_overflow(ptr->preferred);
	ptr->ok = TRUE;
}

static char *
invert_glyph(Widget w, Glyph_type gtype)
{
	Pixel fg;
	Pixmap pm;
	Glyph_info *giP;
	Display *dpy = XtDisplay(w);
	Screen *screen = XtScreen(w);

	XtVaGetValues(w, XtNforeground, &fg, 0);
	if(gtype == Gly_hand)
	{
		if(giP = hands.find(fg))
			pm = giP->pm;
		else
		{
			pm = XCreatePixmapFromData(
				dpy,
				RootWindowOfScreen(screen),
				DefaultColormapOfScreen(screen),
				hand_width,
				hand_height,
				DefaultDepthOfScreen(screen),
				hand_ncolors,
				hand_chars_per_pixel,
				hand_colors,
				hand_pixels,
				fg);
			hands.add(pm, fg);
		}
	}
	else if(gtype == Gly_blank)
	{
		if(giP = solids.find(fg))
			pm = giP->pm;
		else
		{
			pm = XCreatePixmapFromData(
				dpy,
				RootWindowOfScreen(screen),
				DefaultColormapOfScreen(screen),
				solid_width,
				solid_height,
				DefaultDepthOfScreen(screen),
				solid_ncolors,
				solid_chars_per_pixel,
				solid_colors,
				solid_pixels,
				fg);
			solids.add(pm, fg);
		}
	}
	return (char *)pm;
}

void
Table::set_overflow(int new_size)
{
	// record overflow chunk
	overflow = rows - new_size;
	rows = new_size;
}

void
Table::update_glyphs(int row, Boolean highlight)
{
	char **xptr = item_data[row].strings;

	for (int col = 0; col < columns; ++col)
	{
		if (column_spec[col].column_type != Col_glyph)
			continue;

		Glyph_type gtype = hands.find_pm((Pixmap)xptr[col]) != NULL ? 
				Gly_hand : Gly_blank;
		update_glyph(&xptr[col], highlight, gtype);
	}
}

void
Table::update_glyph(char **xptr, Boolean highlight, Glyph_type g_type)
{
	char *def_pm = g_type == Gly_hand ? (char *)hand_pm : (char *)solid_pm;

	*xptr = (highlight && gui_mode == OL_MOTIF_GUI) ? 
			invert_glyph(input, g_type) : def_pm;
}

void
Table::make_glyphs(Widget w)
{
	Display *dpy = XtDisplay(w);
	Screen  *screen = XtScreen(w);
	Pixel bg_pix;
	Glyph_info *giP;

	XtVaGetValues(w,
		XtNbackground, &bg_pix,
		NULL, 0);
	if(giP = hands.find(bg_pix))
		hand_pm = giP->pm;
	else {
		hand_pm = XCreatePixmapFromData(
			dpy,
			RootWindowOfScreen(screen),
			DefaultColormapOfScreen(screen),
			hand_width, 
			hand_height, 
			DefaultDepthOfScreen(screen),
			hand_ncolors, 
			hand_chars_per_pixel,
			hand_colors, 
			hand_pixels,
			bg_pix);
		hands.add(hand_pm, bg_pix);
	}
	if(giP = solids.find(bg_pix))
		solid_pm = giP->pm;
	else {
		solid_pm = XCreatePixmapFromData(
			dpy,
			RootWindowOfScreen(screen),
			DefaultColormapOfScreen(screen),
			solid_width, 
			solid_height, 
			DefaultDepthOfScreen(screen),
			solid_ncolors, 
			solid_chars_per_pixel,
			solid_colors, 
			solid_pixels,
			bg_pix);
		solids.add(solid_pm, bg_pix);
	}
}

// initial items for flatList creation
static char     *init_string[] = { "" };
static Item_data init_data = { init_string, FALSE };

Table::Table(Component *p, const char *name, Select_mode m, const Column *cspec,
	int c, int visible, Boolean wrap, Callback_ptr scb, Callback_ptr ucb,
	Callback_ptr dcb, void *cr, Help_id h) : COMPONENT(p, name, cr, h)
{
	XFontStruct	*font;
	Widget		window;
	int		i;

	wrapped = wrap;
	mode = m;
	columns = c;
	rows = 0;
	select_cb = scb;
	deselect_cb = ucb;
	drop_cb = dcb;
	sensitive = FALSE;
	item_data = 0;
	delay = FALSE;
	old_data = 0;
	delete_start = delete_total = 0;
	overflow = 0;

	column_spec = new Column[columns];
	memcpy(column_spec, cspec, sizeof(Column) * columns);

	widget = XtVaCreateManagedWidget(label, formWidgetClass, parent->get_widget(),
		XtNuserData, this, 0);

	window = XtVaCreateManagedWidget(label, scrolledWindowWidgetClass,
		widget, XtNuserData, this, 0);

	input = XtVaCreateManagedWidget(label, flatListWidgetClass, window,
		XtNformat, "%s",	// to be reset
		XtNitems, &init_data,	// to be reset
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNnumItems, 1,
		XtNexclusives, (mode == SM_single) ? TRUE : FALSE,
		XtNnoneSet, TRUE,
		XtNselectProc, (XtCallbackProc)table_select_CB,
		XtNunselectProc, (XtCallbackProc)table_unselect_CB,
		XtNdropProc, (XtCallbackProc)table_drop_CB,
		XtNclientData, this,
		XtNuserData, this,
		XtNviewHeight, visible,
		0);

	XtAddCallback(input, 
		XtNitemsLimitExceeded, (XtCallbackProc)table_overflow_CB,
		this);

	XtSetSensitive(input, FALSE);
	XtVaGetValues(input, XtNfont, &font, 0);
	font_width = OlFontWidth(font, 0);
	fix_column_spec(font);

	if (!blank_str)
		blank_str = makestr("");
	Item_data *itemP;
	blank_data = itemP = new Item_data;
	itemP->strings = new char *[columns];
	int glyphs_made = 0;
	for (i = 0; i < columns; i++)
	{
		if(column_spec[i].column_type == Col_glyph)
		{
			if(!glyphs_made)
			{
				make_glyphs(input);
				glyphs_made = 1;
			}
			itemP->strings[i] = (char *)solid_pm;
		}
		else
			itemP->strings[i] = blank_str;
	}
	itemP->is_set = FALSE;

	format = make_format(wrap);
	XtVaSetValues(input,
		XtNformat, format,
		XtNitems, blank_data,
		NULL, 0);

	make_title(font, window);
	if (help_msg)
		register_help(widget, label, help_msg);
}

Table::~Table()
{
	delete format;
	delete column_spec;
	delete blank_data;
	cleanup(item_data, rows);
	delete item_data;
	if (old_data)
	{
		cleanup(old_data + delete_start, delete_total);
		delete old_data;
	}
}

void
Table::delay_updates()
{
	set_sensitive(FALSE);
	delay = TRUE;
	vector.clear();
	delete_start = delete_total = 0;
	old_data = 0;
}

void
Table::cleanup(Item_data *ptr, int total)
{
	for (int i = 0; i < total; i++, ptr++)
	{
		for (int j = 0; j < columns; j++)
		{
			if (column_spec[j].column_type == Col_glyph)
				continue;
			if (ptr->strings[j] != blank_str)
				delete ptr->strings[j];
		}
		delete ptr->strings;
	}
}

void
Table::set_row(Item_data *ip, va_list ap)
{
	char **ptr = ip->strings;
	for (int i = 0; i < columns; i++)
	{
		if(column_spec[i].column_type == Col_glyph)
		{
			Glyph_type g_type = va_arg(ap, Glyph_type);
			update_glyph(&ptr[i], (Boolean)ip->is_set, g_type);
		}
		else
		{
			char *s = va_arg(ap, char *);
			char *olds = ptr[i];

			ptr[i] = s ? makestr(s) : blank_str;
			if (olds && olds != blank_str)
				delete olds;
		}
	}
}

void
Table::insert_row(int row ...)
{
	Item_data	new_row;
	va_list		ap;
	int		i;

	if (row > rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	new_row.is_set = FALSE;
	new_row.strings = new char *[columns];
	memset(new_row.strings, 0, columns * sizeof(char *));
	va_start(ap, row);
	set_row(&new_row, ap);
	va_end(ap);

	if (delay)
	{
		if (row && !vector.size())
		{
			vector.add(item_data, row * sizeof(Item_data));
			old_data = item_data;
		}
		vector.add(&new_row, sizeof(Item_data));
		if (row < rows)
			vector.add((Item_data *)item_data + row,
				(rows - row) * sizeof(Item_data));
		item_data = (Item_data *)vector.ptr();
		rows++;
	}
	else
	{
		rows++;
		Item_data	*new_data = new Item_data[rows];

		for (i = 0; i < row; i++)
			new_data[i] = item_data[i];
		new_data[i++] = new_row;
		for (; i < rows; i++)
			new_data[i] = item_data[i-1];

		set_sensitive(TRUE);
		XtVaSetValues(input,
			XtNnumItems, rows,
			XtNitems, new_data,
			XtNitemsTouched, TRUE,
			NULL, 0);
		delete item_data;
		item_data = new_data;
	}
}

void
Table::delete_rows(int row, int total)
{
	if (row + total > rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	rows -= total;
	if (rows)
	{
		vector.clear();
		vector.add(item_data, row * sizeof(Item_data));
		vector.add(item_data + row + total, (rows - row) * sizeof(Item_data));
		old_data = item_data;

		if (delay)
		{
			delete_start = row;
			delete_total = total;
			item_data = (Item_data *)vector.ptr();
		}
		else
		{
			item_data = new Item_data[rows];
			memcpy(item_data, vector.ptr(), rows * sizeof(Item_data));
			XtVaSetValues(input,
				XtNnumItems, rows,
				XtNitems, item_data,
				XtNitemsTouched, TRUE,
				NULL, 0);
			cleanup(old_data + row, total);
			delete old_data;
		}
	}
	else if (!delay)
	{
		XtVaSetValues(input,
			XtNnumItems, 1,
			XtNitems, blank_data,
			XtNitemsTouched, TRUE,
			NULL, 0);
		set_sensitive(FALSE);
	}
}

void
Table::set_row(int row ...)
{
	va_list 	ap;

	if (row >= rows)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (!sensitive)
	{
		sensitive = TRUE;
		XtSetSensitive(input, TRUE);
	}

	va_start(ap, row);
	set_row(&item_data[row], ap);
	va_end(ap);

	if (!delay)
		OlVaFlatSetValues(input, row, XtNformatData, item_data[row].strings, 0);
}

void
Table::set_cell(int row, int col, Glyph_type g_type)
{
	char	**xptr;

	if (row >= rows || col >= columns || 
		column_spec[col].column_type != Col_glyph)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (!sensitive)
	{
		sensitive = TRUE;
		XtSetSensitive(input, TRUE);
	}

	xptr = &(item_data[row].strings[col]);
	update_glyph(xptr, (Boolean)item_data[row].is_set, g_type);

	if (!delay)
		OlVaFlatSetValues(input, row, XtNformatData, item_data[row].strings, 0);
}

void
Table::set_cell(int row, int col, const char *s)
{
	Item_data	*xptr;
	char		*olds;

	if (row >= rows || col >= columns
		|| column_spec[col].column_type == Col_glyph)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (!sensitive)
	{
		sensitive = TRUE;
		XtSetSensitive(input, TRUE);
	}

	xptr = &item_data[row];
	olds = xptr->strings[col];
	xptr->strings[col] = s ? makestr(s) : blank_str;

	if (!delay)
		OlVaFlatSetValues(input, row, XtNformatData, xptr->strings, 0);

	if (olds != blank_str)
		delete olds;
}

Cell_value
Table::get_cell(int row, int col)
{
	Cell_value value;

	if (row >= rows || col >= columns)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		value.string = 0;
		return value;
	}

	if (column_spec[col].column_type == Col_glyph)
	{
		Item_data *ip = &item_data[row];
		if (hands.find_pm((Pixmap)ip->strings[col]))
			value.glyph = Gly_hand;
		else if (solids.find_pm((Pixmap)ip->strings[col]))
			value.glyph = Gly_blank;
	}
	else 
		value.string =  item_data[row].strings[col];
	return value;
}

void
Table::clear()
{
	set_sensitive(TRUE);
	XtVaSetValues(input,
		XtNitems, blank_data,
		XtNitemsTouched, TRUE,
		XtNnumItems, 1,
		0);
	set_sensitive(FALSE);

	cleanup(item_data, rows);
	delete item_data;
	item_data = 0;
	rows = 0;
	delay = FALSE;
	delete_start = delete_total = 0;
}

void
Table::set_sensitive(Boolean s)
{
	if (sensitive != s)
		XtSetSensitive(input, s);
	sensitive = s;
}

void
Table::finish_updates()
{
	Item_data	*new_data = 0;
	Arg		args[5];
	int		n = 0;

	set_sensitive(TRUE);
	if (vector.size())
	{
		item_data = new Item_data[rows];
		memcpy(item_data, vector.ptr(), rows * sizeof(Item_data));
		XtSetArg(args[n], XtNitems, item_data); n++;
		XtSetArg(args[n], XtNnumItems, rows); n++;
	}
	else if (!rows)
	{
		XtSetArg(args[n], XtNnumItems, 1); n++;
		XtSetArg(args[n], XtNitems, blank_data); n++;
	}
	XtSetArg(args[n], XtNitemsTouched, TRUE); n++;
	XtSetValues(input, args, n);
	if (!rows)
		set_sensitive(FALSE);

	if (overflow)
	{
		// flatList overflow. by now, 'rows' has been reset.
		// so truncate the list in 'item_data'.
		old_data = item_data;
		cleanup(old_data + rows, overflow);
		delete_total = 0;
		item_data = new Item_data[rows];
		memcpy(item_data, old_data, rows * sizeof(Item_data));
		XtVaSetValues(input, XtNitems, item_data, 0);
		overflow = 0;
	}

	if (delete_total)
	{
		cleanup(old_data + delete_start, delete_total);
		delete_total =  0;
	}
	delete old_data;
	delay = FALSE;
}

int
Table::get_selections(int *&sel)
{
	int nsels = 0;

	vscratch1.clear();
	for (int i = 0; i < rows; i++)
	{
		if (item_data[i].is_set)
		{
			vscratch1.add(&i, sizeof(int));
			nsels++;
		}
	}
	sel = (int *)vscratch1.ptr();
	return nsels;
}

void
Table::deselect(int row)
{
	if (item_data[row].is_set)
	{
		update_glyphs(row, FALSE);
		OlVaFlatSetValues(input, row, XtNset, FALSE, 0);
		if (deselect_cb && creator)
			(deselect_cb)(creator, this, (void *)row);
		item_data[row].is_set = FALSE;
	}
}

void
Table::deselect_all()
{
	Item_data	*ptr = item_data;

	for (int i = 0; i < rows; i++, ptr++)
	{
		if (ptr->is_set)
		{
			update_glyphs(i, FALSE);
			OlVaFlatSetValues(input, i, XtNset, FALSE, 0);
			if (deselect_cb && creator)
				(deselect_cb)(creator, this, (void *)i);
			ptr->is_set = FALSE;
		}
	}
}

void
Table::wrap_columns(Boolean wrap)
{
	if (wrap == wrapped)
		return;

	char	*new_format = make_format(wrap);

	XtVaSetValues(input, XtNformat, new_format, 0);

	delete format;
	format = new_format;
	wrapped = wrap;
}

char *
Table::make_format(Boolean wrap)
{
	const Column	*cspec = column_spec;
	char		*new_format = new char[columns * 6];
	const char	*f = "";

	new_format[0] = '\0';
	for (int i = 0; i < columns; i++, cspec++)
	{

		switch (cspec->column_type)
		{
		case Col_wrap_text:	f = wrap ? "%s%%%dw " : "%s%%%ds ";	break;
		case Col_text:		f = "%s%%%ds ";				break;
		case Col_numeric:	f = "%s%%-%ds ";			break;

		case Col_glyph:		f = "%s%%%dp";				break;

		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;
		}

		sprintf(new_format, f, new_format, 
				cspec->column_type == Col_glyph ? 
				Round(hand_width,font_width) :
				cspec->size);
	}
	return new_format;
}

void
Table::fix_column_spec(XFontStruct *font)
{
	Column	*cspec;
	int	n_width = XTextWidth(font, "n", 1);
	int	i;

	if (font_width == n_width)
		return;

	for (i = 0, cspec = column_spec; i < columns; i++, cspec++)
	{
		if (cspec->column_type == Col_glyph)
			continue;

		cspec->size = (int)(cspec->size * ((float)n_width/font_width));
		if (cspec->size == 0)
			cspec->size = 1;
	}

}

void
Table::make_title(XFontStruct *font, Widget window)
{
	int	total_width;
	int	blank_width;
	Widget	title;
	char	title_buf[BUFSIZ];
	Column	*cspec;
	int	i;

	title_buf[0] = '\0';
	total_width = 0;
	blank_width = XTextWidth(font, " ", 1);
	for (i = 0, cspec = column_spec; i < columns; i++, cspec++)
	{
		int	blanks_needed;
		int	str_width;

		if (cspec->column_type == Col_glyph)
		{
			// round up to integral # of font_widths 
			total_width += Round(hand_width, font_width);
		}
		else
			total_width += (cspec->size+1) * font_width;
		if (cspec->heading)
			strcat(title_buf, cspec->heading);
		str_width = XTextWidth(font, title_buf, strlen(title_buf));
		blanks_needed = (total_width - str_width + blank_width)/blank_width;
		for (int j = 0; j < blanks_needed; j++)
			strcat(title_buf, " ");
	}

	title = XtVaCreateManagedWidget("title", staticTextWidgetClass, widget,
		XtNstring, title_buf,
		XtNgravity, WestGravity,
		XtNstrip, FALSE,
		XtNuserData, this,
		0);

	XtVaSetValues(window, XtNyRefWidget, title,
		XtNyAddHeight, TRUE,
		XtNyOffset, PADDING,
		XtNxResizable, TRUE,
		XtNyResizable, TRUE,
		XtNxAttachRight, TRUE,
		XtNyAttachBottom, TRUE,
		0);

}

Boolean
Table::is_delayed()
{
	return delay;
}

void
Table::show_border()
{
	XtVaSetValues(widget, XtNborderWidth, 1, 0);
}
