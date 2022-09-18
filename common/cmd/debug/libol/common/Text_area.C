#ident	"@(#)debugger:libol/common/Text_area.C	1.1"

#include "UI.h"
#include "Component.h"
#include "Text_area.h"
#include "str.h"
#include <stdlib.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>

static void
get_textCB(Widget, Text_area *text, OlTextMotionCallData *ptr)
{
	if (!ptr || !text || !text->get_creator())
		return;

	text->set_selection(ptr->select_start, ptr->select_end);
}

// A Text_area is implemented with a TextEdit widget inside a
// scrolled window
Text_area::Text_area(Component *p, const char *name, int rows, int columns,
	Callback_ptr scb, void *c, Help_id h) : COMPONENT(p, name, c, h)
{
	string = 0;
	select_cb = scb;

	widget = XtVaCreateManagedWidget("text_widget", scrolledWindowWidgetClass,
		parent->get_widget(), XtNuserData, this, 0);

	textbuf = ReadStringIntoTextBuffer("", 0, 0);
	text_area = XtVaCreateManagedWidget(label, textEditWidgetClass,
		widget,
		XtNsourceType, OL_TEXT_BUFFER_SOURCE,
		XtNsource, textbuf,
		XtNcharsVisible, columns,
		XtNlinesVisible, rows,
		XtNwrapMode, OL_WRAP_OFF,
		XtNuserData, this,
		0);

	if (select_cb)
		XtAddCallback(text_area, XtNmotionVerification,
			(XtCallbackProc)get_textCB, (XtPointer)this);
	setup_tab_table();

	if (help_msg)
		register_help(widget, label, help_msg);
}

// A Text_area is implemented with a TextEdit widget inside a
// scrolled window
Text_area::Text_area(Component *p, const char *name, void *c,
	Callback_ptr scb, Help_id h) : COMPONENT(p, name, c, h)
{
	string = 0;
	select_cb = scb;

	widget = XtVaCreateManagedWidget("text_widget", scrolledWindowWidgetClass,
		parent->get_widget(), XtNuserData, this, 0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Text_area::~Text_area()
{
	delete string;
	delete tab_table;
}

void
Text_area::setup_tab_table()
{
	int		n_width;

	XtVaGetValues(text_area, XtNfont, &font, 0);
	n_width = XTextWidth(font, "n", 1);
	tab_table = new Position[21];
	for (int i = 0; i < 20; i++)
		tab_table[i] = (i + 1) * n_width * 8;
	tab_table[20] = 0;
	XtVaSetValues(text_area, XtNtabTable, tab_table, 0);
}

void
Text_area::add_text(const char *s)
{
	TextPosition	next_position = LastTextBufferPosition((TextBuffer *)textbuf);
	size_t		len = strlen(s);

	(void) OlTextEditSetCursorPosition(text_area, next_position, next_position,
		next_position);
	(void) OlTextEditInsert(text_area, (char *)s, len);
}

// return entire contents
char *
Text_area::get_text()
{
	delete string;
	string = 0;
	(void) OlTextEditCopyBuffer(text_area, &string);
	return string;
}

char *
Text_area::get_selection()
{
	Boolean ret;
	int	start, end, pos;

	delete string;
	string = 0;

	ret = OlTextEditGetCursorPosition(text_area, &start, &end, &pos);
	if (!ret || (start == end))
		return 0;

	ret = OlTextEditReadSubString(text_area, &string, start, end-1);
	if (!ret || !string)
		return 0;
	return string;
}

// blank out text area
void
Text_area::clear()
{
	(void) OlTextEditClearBuffer(text_area);
}

void
Text_area::copy_selection()
{
	(void) OlTextEditCopySelection(text_area, 0);
}

void
Text_area::set_selection(int start, int end)
{
	if (!select_cb)
		return;

	select_cb(creator, this, (start < end) ? (void *)1 : (void *)0);
}

void
Text_area::position(int line)
{
	TextPosition start = PositionOfLine((TextBuffer *)textbuf, line-1);
	XtVaSetValues(text_area, XtNdisplayPosition, start, 0);
}
