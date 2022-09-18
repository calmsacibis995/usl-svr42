#ident	"@(#)debugger:gui.d/common/Status.C	1.6"

#include "Component.h"
#include "Table.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Status.h"
#include "Windows.h"

static Column st_spec[] = {
	{ "Program",	9,	Col_text },
	{ "Process",	9,	Col_text },
	{ "State",	9,	Col_text },
	{ "Function",	12,	Col_text },
	{ "Location",	18,	Col_text },
};

Status_pane::Status_pane(Expansion_box *container)
{
	pane = new Table(container, "status", SM_single, st_spec,
		sizeof(st_spec)/sizeof(Column), 1, FALSE);
	pane->show_border();
	container->add_component(pane);
	last_process = 0;

	pane->insert_row(0, 0, 0, 0, 0, 0);
}

Status_pane::~Status_pane()
{
}

void
Status_pane::update(Process *proc)
{
	if (proc)
	{
		pane->set_row(0, 
			proc->get_program()->get_name(),
			proc->get_name(),
			proc->get_state_string(),
			proc->get_function(),
			proc->get_location());
	}
	else
		// clear row
		pane->set_row(0, 0, 0, 0, 0, 0,);
}
