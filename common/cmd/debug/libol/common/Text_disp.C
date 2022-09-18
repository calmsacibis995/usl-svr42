#ident	"@(#)debugger:libol/common/Text_disp.C	1.12"

#include "UI.h"
#include "Component.h"
#include "Text_area.h"
#include "Text_disp.h"
#include "str.h"
#include <stdlib.h>
#include <regexpr.h>
#include <Xol/TextEdit.h>

// arrow definition
#define arrow_width 18
#define arrow_height 9
static unsigned char arrow_bits[] = {
   0x00, 0x10, 0x00, 0x00, 0x30, 0x00, 0x00, 0x70, 0x00, 0xfe, 0xff, 0x00,
   0xfe, 0xff, 0x01, 0xfe, 0xff, 0x00, 0x00, 0x70, 0x00, 0x00, 0x30, 0x00,
   0x00, 0x10, 0x00};

// stop sign definition
#define stop_width 11
#define stop_height 11
static unsigned char stop_bits[] = {
   0xf8, 0x00, 0xfc, 0x01, 0x06, 0x03, 0xf7, 0x07, 0xf7, 0x07, 0x07, 0x07,
   0x7f, 0x07, 0x7f, 0x07, 0x06, 0x03, 0xfc, 0x01, 0xf8, 0x00};

static Pixmap stop_sign;
static Pixmap arrow;
static Text_display *searched_pane;

static void
get_textCB(Widget, Text_area *text, OlTextMotionCallData *ptr)
{
	if (!ptr || !text || !text->get_creator())
		return;

	text->set_selection(ptr->select_start, ptr->select_end);
}

static const char *
search_forward(const char *, const char *curp, const char *exp)
{
	if (step(curp, exp))
	{
		searched_pane->set_selection_size(loc2 - loc1);
		return loc1;
	}
	return 0;
}

static const char *
search_backward(const char *string, const char *curp, const char *exp)
{
	char	*last_match = 0;
	while (step(string, exp))
	{
		if (!curp || strstr(loc1, curp))
		{
			last_match = loc1;
			searched_pane->set_selection_size(loc2 - loc1);
		}
		string = locs = loc2;
	}
	return last_match;
}

static void
marginCB(Widget, Text_display *pane, OlTextMarginCallData *ptr)
{
	pane->fix_margin(ptr->rect);
}

Text_display::~Text_display()
{
	if (gc_base)
		XFreeGC(XtDisplay(text_area), gc_base);
	if (gc_stop)
		XFreeGC(XtDisplay(text_area), gc_stop);
}

Text_display::Text_display(Component *p, const char *name,
	Callback_ptr scb, void *c, Help_id h) : TEXT_AREA(p, name, c, scb, h)
{
	current_line = 0;
	empty = 1;
	search_expr = 0;
	compiled_expr = 0;
	selection_size = 0;
	breaks = 0;
	last_line = 0;
	gc_base = gc_stop = 0;
}

void
Text_display::setup_source_file(const char *path, int rows, int columns)
{
	Arg		args[10];
	int		n;

	n = 0;
	XtSetArg(args[n], XtNcharsVisible, columns); n++;
	XtSetArg(args[n], XtNlinesVisible, rows); n++;
	XtSetArg(args[n], XtNwrapMode, OL_WRAP_OFF); n++;
	XtSetArg(args[n], XtNuserData, this); n++;
	XtSetArg(args[n], XtNeditType, OL_TEXT_READ); n++;

	if (path)
	{
		empty = 0;
		XtSetArg(args[n], XtNsourceType, OL_DISK_SOURCE); n++;
		XtSetArg(args[n], XtNsource, path); n++;
	}
	else
	{
		empty = 1;
		XtSetArg(args[n], XtNsourceType, OL_STRING_SOURCE); n++;
		XtSetArg(args[n], XtNsource, ""); n++;
	}
	text_area = XtCreateManagedWidget(label, textEditWidgetClass,
		widget, args, n);

	is_source = 1;
	finish_setup();
}

void
Text_display::setup_disassembly(int rows, int columns)
{
	text_area = XtVaCreateManagedWidget(label, textEditWidgetClass, widget,
		XtNsourceType, OL_STRING_SOURCE,
		XtNsource, "",
		XtNlinesVisible, rows,
		XtNcharsVisible, columns,
		XtNwrapMode, OL_WRAP_OFF,
		XtNuserData, this,
		XtNeditType, OL_TEXT_READ,
		0);

	is_source = 0;
	finish_setup();
}

void
Text_display::finish_setup()
{
	if (select_cb)
		XtAddCallback(text_area, XtNmotionVerification,
			(XtCallbackProc)get_textCB, (XtPointer)this);
	textbuf = OlTextEditTextBuffer(text_area);
	setup_tab_table();

	if (is_source)
	{
		last_line = LineOfPosition((TextBuffer *)textbuf,
			LastTextBufferPosition((TextBuffer *)textbuf)) + 1;
		x_stop = XTextWidth(font, "000000 ", sizeof("000000 "));
	}
	else
	{
		last_line = 0;
		x_stop = XTextWidth(font, " ", 1);
	}
	x_arrow = x_stop + stop_width + 2;
	left_margin = x_arrow + arrow_width + 2;
	XtVaSetValues(text_area, XtNleftMargin, left_margin, 0);
	font_height = OlFontHeight(font, 0);
	XtVaGetValues(text_area, XtNtopMargin, &top_margin, 0);

	RegisterTextBufferScanFunctions(search_forward, search_backward);
	XtAddCallback(text_area, XtNmargin, (XtCallbackProc)marginCB, (XtPointer)this);
}

void
Text_display::set_file(const char *path)
{
	delete breaks;
	breaks = 0;
	empty = 0;
	current_line = 0;
	last_line = 0;

	XtVaSetValues(text_area,
		XtNsourceType, OL_DISK_SOURCE,
		XtNsource, path,
		0);
}

void
Text_display::set_buf(const char *buf)
{
	delete breaks;
	breaks = 0;
	empty = 0;
	current_line = 0;
	last_line = 0;

	XtVaSetValues(text_area,
		XtNsourceType, OL_STRING_SOURCE,
		XtNsource, buf,
		0);
}

void
Text_display::set_breaklist(int *breaklist)
{
	if (!last_line)
		last_line = LineOfPosition((TextBuffer *)textbuf,
			LastTextBufferPosition((TextBuffer *)textbuf)) + 1;

	int break_size = (last_line + 7)/8;
	breaks = new char[break_size];
	memset(breaks, 0, break_size);
	if (breaklist)
	{
		for (int i = 0; breaklist[i]; i++)
			set_stop(breaklist[i]);
	}
}

void
Text_display::set_selection(int start, int end)
{
	selection_start = start;
	selection_size = end - start;
	if (!select_cb)
		return;

	int line;
	if (start < end)
		line = LineOfPosition((TextBuffer *)textbuf, start) + 1;
	else
		line = 0;
	(select_cb)(creator, this, (void *)line);
}

Search_return
Text_display::search(const char *s, int forwards)
{
	if (empty)
		return SR_notfound;

	TextLocation	loc;
	ScanResult	ret;
	int		pos;

	if (strcmp(s, search_expr) != 0)
	{
		delete (char *)search_expr;
		delete compiled_expr;
		search_expr = makestr(s);
		compiled_expr = compile(s, 0, 0);
		if (!compiled_expr)
			return SR_bad_expression;
	}

	searched_pane = this;
	if (forwards)
	{
		loc = LocationOfPosition((TextBuffer *)textbuf,
			selection_start + selection_size - 1);
		ret = ForwardScanTextBuffer((TextBuffer*)textbuf, compiled_expr, &loc);
	}
	else
	{
		loc = LocationOfPosition((TextBuffer *)textbuf, selection_start);
		ret = BackwardScanTextBuffer((TextBuffer*)textbuf, compiled_expr, &loc);
	}

	switch(ret)
	{
	case SCAN_WRAPPED:
	case SCAN_FOUND:
		pos = PositionOfLocation((TextBuffer *)textbuf, loc);
		OlTextEditSetCursorPosition(text_area, pos, pos + selection_size, pos);
		return SR_found;

	case SCAN_NOTFOUND:	return SR_notfound;
	case SCAN_INVALID:
	default:		return SR_bad_expression;
	}
}

int
Text_display::has_breakpoint(int line)
{
	if (!breaks)
		return 0;
	int	bit = 1 << (line % 8);
	return breaks[line/8] & bit;
}

void
Text_display::set_line(int line)
{
	clear_arrow(current_line);
	current_line = line;
	if (current_line)
		draw_arrow(line);
}

void
Text_display::position(int line, Boolean highlight)
{
	int	lower;
	int	upper;

	if (empty)
		return;

	TextPosition start = PositionOfLine((TextBuffer *)textbuf, line-1);
	TextPosition end = PositionOfLine((TextBuffer *)textbuf, line);

	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		XtVaSetValues(text_area, XtNdisplayPosition, start, 0);
	OlTextEditSetCursorPosition(text_area, start,
		highlight ? end : start, start);
}

void
Text_display::draw_number(int line, int top_line)
{
	char	buf[10];

	sprintf(buf, "%d ", line);
	int width = XTextWidth(font, buf, strlen(buf));
	int x = x_stop - width;
	int y = top_margin + font->ascent + font_height * (line - top_line);

	if (!gc_base)
		set_gc();
	XDrawString(XtDisplay(text_area), XtWindow(text_area), gc_base,
		x, y, buf, strlen(buf));
}

void
Text_display::clear_arrow(int line)
{
	int	lower, upper;

	if (!line)
		return;
	current_line = 0;
	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		return;

	int y = top_margin + font_height * (line - lower)
		+ (font_height - arrow_height)/2;
	XClearArea(XtDisplay(text_area), XtWindow(text_area),
		x_arrow, y, arrow_width, arrow_height, FALSE);
}

void
Text_display::draw_arrow(int line)
{
	int	lower, upper;

	if (!line)
		return;
	current_line = line;
	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		return;

	int	y = top_margin + font_height * (line - lower)
		+ (font_height - arrow_height)/2;
	Screen	*screen = XtScreen(text_area);

	if (!gc_base)
		set_gc();
	if (!arrow)
	{
		if ((arrow = XCreateBitmapFromData(XtDisplay(text_area), XtWindow(text_area), 
			(char *)arrow_bits, arrow_width, arrow_height)) == 0)
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	XCopyPlane(XtDisplay(text_area), arrow, XtWindow(text_area), gc_base, 0, 0,
			arrow_width, arrow_height, x_arrow, y, 1);
}

void
Text_display::draw_stop(int line)
{
	int	lower, upper;

	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		return;

	int y = top_margin + font_height * (line - lower)
		+ (font_height - stop_height)/2;
	Screen *screen = XtScreen(text_area);

	if (!gc_base)
		set_gc();
	if (!stop_sign)
	{
		if ((stop_sign = XCreateBitmapFromData(XtDisplay(text_area), XtWindow(text_area),
			(char *)stop_bits, stop_width, stop_height)) == 0)
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	XCopyPlane(XtDisplay(text_area), stop_sign, XtWindow(text_area), gc_stop,
			0, 0, stop_width, stop_height, x_stop, y, 1);
}

void
Text_display::clear_stop(int line)
{
	int	lower, upper;
	int	bit = 1 << (line % 8);

	breaks[line/8] &= ~bit;

	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		return;

	int y = top_margin + font_height * (line - lower)
		+ (font_height - stop_height)/2;
	XClearArea(XtDisplay(text_area), XtWindow(text_area),
		x_stop, y, stop_width, stop_height, FALSE);
}

void
Text_display::set_stop(int line)
{
	int bit = 1 << (line % 8);

	breaks[line/8] |= bit;
	draw_stop(line);
}

void
Text_display::pane_bounds(int &lower, int &upper)
{
	int	top;
	int	lines_visible;

	if (empty)
	{
		lower = upper = 0;
		return;
	}

	XtVaGetValues(text_area,
		XtNdisplayPosition, &top,
		XtNlinesVisible, &lines_visible, 0);

	lower = LineOfPosition((TextBuffer *)textbuf, top) + 1;
	upper = lower + lines_visible - 1;
}

void
Text_display::set_gc()
{
	XGCValues	gc_values;
	unsigned long	gc_mask;
	unsigned long	foreground;
	unsigned long	background;
	Colormap	cmap;
	XColor		red;

	XtVaGetValues(text_area,
		XtNfontColor, &foreground, 
		XtNbackground, &background,
		0);

	gc_values.foreground = foreground;
	gc_values.background = background;
	gc_values.font = font->fid;
	gc_mask = GCFont | GCForeground | GCBackground;
	gc_base = XCreateGC(XtDisplay(text_area), XtWindow(text_area), gc_mask, &gc_values);

	cmap = DefaultColormapOfScreen(XtScreen(text_area));
	if (XParseColor(XtDisplay(text_area), cmap, "Red", &red)
		&& XAllocColor(XtDisplay(text_area), cmap, &red))
		gc_values.foreground = red.pixel;
	gc_stop = XCreateGC(XtDisplay(text_area), XtWindow(text_area), gc_mask, &gc_values);
}

void
Text_display::fix_margin(XRectangle *rect)
{
	if (empty)
	{
		XClearArea(XtDisplay(text_area), XtWindow(text_area), rect->x, rect->y,
			left_margin, rect->height, FALSE);
		return;
	}

	int	top, top_line;
	int	line;
	int	lines_visible;
	int	max_lines;
	int	stop;

	XtVaGetValues(text_area,
		XtNdisplayPosition, &top,
		XtNlinesVisible, &lines_visible, 0);

	top_line = LineOfPosition((TextBuffer *)textbuf, top) + 1;
	max_lines = ((int)rect->height+font_height-1)/font_height;
	if (!last_line)
		last_line = LineOfPosition((TextBuffer *)textbuf,
			LastTextBufferPosition((TextBuffer *)textbuf)) + 1;

	line = top_line + (rect->y/font_height);
	stop = top_line + lines_visible;
	if (line + max_lines < stop)
		stop = line + max_lines;

	for (; line < stop; line++)
	{
		if (line > last_line)
		{
			int y = top_margin + font->ascent
				+ font_height * (line - top_line);
			XClearArea(XtDisplay(text_area), XtWindow(text_area), 0, y,
				left_margin, rect->height - (y - rect->y), FALSE);
			break;
		}
		else
		{
			if (is_source)
				draw_number(line, top_line);
			if (line == current_line)
				draw_arrow(line);
			if (has_breakpoint(line))
				draw_stop(line);
		}
	}
}

// blank out text area
void
Text_display::clear()
{
	empty = 1;
	if (is_source)
	{
		XtVaSetValues(text_area,
			XtNsourceType, OL_STRING_SOURCE,
			XtNsource, "",
			XtNcursorPosition, 0,
			XtNselectStart, 0,
			XtNselectEnd, 0,
			XtNdisplayPosition, 0,
			0);
	}
	else
		(void) OlTextEditClearBuffer(text_area);
}

int
Text_display::get_last_line()
{
	return last_line;
}

int
Text_display::get_position()
{
	if (empty)
		return 0;

	int top;
	XtVaGetValues(text_area, XtNdisplayPosition, &top, 0);
	return LineOfPosition((TextBuffer *)textbuf, top) + 1;
}
