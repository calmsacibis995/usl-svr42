#ident	"@(#)debugger:libutil/common/curr_frame.C	1.2"
#include "utility.h"
#include "LWP.h"
#include "Frame.h"
#include "Interface.h"

// get and set current frame

// in all cases, we assume the higher level functions
// have tested for a null LWP

int
curr_frame(LWP *lwp)
{
	Frame	*cframe;
	int	count = 0;

	cframe = lwp->curframe();
	while((cframe = cframe->caller()) != 0)
		count++;
	return count;
}

// total number of valid frames - 1
// actually, returns highest numbered frame,
// where frames are numbered from 0
int
count_frames(LWP *lwp)
{
	Frame	*cframe;
	int	count = 0;

	cframe = lwp->topframe();
	while((cframe = cframe->caller()) != 0)
		count++;
	return count;
}

int 
set_frame(LWP *lwp, int frameno)
{
	Frame	*cframe;
	int	count;

	count = count_frames(lwp);
	if ((frameno < 0) || (frameno > count))
	{
		printe(ERR_frame_range, E_ERROR, frameno, lwp->lwp_name());
		return 0;
	}
	cframe = lwp->topframe();
	while(frameno < count)
	{
		cframe = cframe->caller();
		count--;
	}
	if (!lwp->setframe(cframe))
		return 0;

	if (get_ui_type() == ui_gui)
		printm(MSG_set_frame, (unsigned long)lwp, frameno);
	return 1;
}
