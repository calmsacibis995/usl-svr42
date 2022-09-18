#ident	"@(#)debugger:gui.d/common/Help.C	1.12"

#include "Help.h"

Help_info Help_files[HELP_final] =
{
	{0,0,0},			// HELP_none
	{"context.help","1"},		// HELP_context_window
	{"context.help","11"},		// HELP_ps_pane
	{"context.help","12"},		// HELP_stack_pane
	{"context.help","13"},		// HELP_syms_pane

	// Context window, File menu
	{"context.help","14"},		// HELP_ctxt_file_menu
	{"context.help","141"},		// HELP_ctxt_create_dialog
	{"context.help","142"},		// HELP_ctxt_recreate_dialog,
	{"context.help","143"},		// HELP_ctxt_grab_core_dialog,
	{"context.help","144"},		// HELP_ctxt_grab_process_dialog,
	{"context.help","145"},		// HELP_release_cmd,
	{"context.help","146"},		// HELP_ctxt_cd_dialog
	{"context.help","147"},		// HELP_new_window_set_cmd,
	{"context.help","148"},		// HELP_windows_menu,
	{"context.help","149"},		// HELP_dismiss_cmd,
	{"context.help","140"},		// HELP_quit_cmd,

	// Context window, Edit menu
	{"context.help","15"},		// HELP_ctxt_edit_menu,
	{"context.help","151"},		// HELP_set_current_cmd,
	{"context.help","152"},		// HELP_export_cmd,

	// Context window, View menu
	{"context.help","16"},		// HELP_ctxt_view_menu,
	{"context.help","161"},		// HELP_ctxt_expand_dialog,
	{"context.help","162"},		// HELP_ctxt_show_value_dialog,
	{"context.help","163"},		// HELP_ctxt_set_value_dialog,
	{"context.help","164"},		// HELP_ctxt_dump_dialog,
	{"context.help","165"},		// HELP_ctxt_map_dialog,

	// Context window, Control menu
	{"context.help","17"},		// HELP_ctxt_control_menu,
	{"context.help","171"},		// HELP_run_cmd,
	{"context.help","172"},		// HELP_return_cmd,
	{"context.help","173"},		// HELP_ctxt_run_until_dialog,
	{"context.help","174"},		// HELP_step_stmt_cmd,
	{"context.help","175"},		// HELP_step_instr_cmd,
	{"context.help","176"},		// HELP_next_stmt_cmd,
	{"context.help","177"},		// HELP_next_instr_cmd,
	{"context.help","178"},		// HELP_ctxt_step_dialog,
	{"context.help","179"},		// HELP_ctxt_jump_dialog,
	{"context.help","170"},		// HELP_halt_cmd,

	// Context window, Event menu
	{"context.help","18"},		// HELP_ctxt_control_menu,
	{"context.help","181"},		// HELP_ctxt_stop_on_function_dialog,
	{"context.help","182"},		// HELP_set_watchpoint_cmd,
	{"context.help","183"},		// HELP_ctxt_stop_dialog,
	{"context.help","184"},		// HELP_ctxt_signal_dialog,
	{"context.help","185"},		// HELP_ctxt_syscall_dialog,
	{"context.help","186"},		// HELP_ctxt_on_stop_dialog,
	{"context.help","187"},		// HELP_ctxt_cancel_dialog,
	{"context.help","188"},		// HELP_ctxt_kill_dialog,
	{"context.help","189"},		// HELP_ctxt_ignore_signals_dialog,

	// Context window, Properties menu
	{"context.help","19"},		// HELP_ctxt_properties_menu
	{"context.help","191"},		// HELP_ctxt_symbols_dialog,
	{"context.help","192"},		// HELP_ctxt_panes_dialog,
	{"context.help","193"},		// HELP_ctxt_path_dialog,
	{"context.help","194"},		// HELP_ctxt_language_dialog,
	{"context.help","195"},		// HELP_ctxt_granularity_dialog,
	{"context.help","196"},		// HELP_ctxt_action_dialog

	// Source window, File menu
	{"source.help","2"},		// HELP_source_window
	{"source.help","21"},		// HELP_source_file_menu
	{"source.help","211"},		// HELP_source_open_dialog
	{"source.help","212"},		// HELP_new_source_cmd
	{"source.help","213"},		// HELP_source_windows_menu
	{"source.help","214"},		// HELP_source_dismiss_cmd
	{"source.help","215"},		// HELP_source_quit_cmd

	// Source window, Edit menu
	{"source.help","22"},		// HELP_source_edit_menu
	{"source.help","221"},		// HELP_copy_cmd

	// Source window, View menu
	{"source.help","23"},		// HELP_source_view_menu
	{"source.help","231"},		// HELP_source_show_line_dialog
	{"source.help","232"},		// HELP_source_show_function_dialog
	{"source.help","233"},		// HELP_search_dialog
	{"source.help","234"},		// HELP_source_show_value_dialog
	{"source.help","235"},		// HELP_source_set_value_dialog

	// Source window, Control menu
	{"source.help","24"},		// HELP_source_control_menu,
	{"source.help","241"},		// HELP_source_run_cmd,
	{"source.help","242"},		// HELP_source_return_cmd,
	{"source.help","243"},		// HELP_source_run_until_dialog,
	{"source.help","244"},		// HELP_source_step_stmt_cmd,
	{"source.help","245"},		// HELP_source_step_instr_cmd,
	{"source.help","246"},		// HELP_source_next_stmt_cmd,
	{"source.help","247"},		// HELP_source_next_instr_cmd,
	{"source.help","248"},		// HELP_source_step_dialog,
	{"source.help","249"},		// HELP_source_jump_dialog,
	{"source.help","240"},		// HELP_source_halt_cmd,

	// Source window, Event menu
	{"source.help","25"},		// HELP_source_event_menu
	{"source.help","251"},		// HELP_set_breakpoint
	{"source.help","252"},		// HELP_delete_breakpoint
	{"source.help","253"},		// HELP_source_stop_dialog
	{"source.help","254"},		// HELP_source_signal_dialog
	{"source.help","255"},		// HELP_source_syscall_dialog
	{"source.help","256"},		// HELP_source_onstop_dialog
	{"source.help","257"},		// HELP_source_cancel_dialog
	{"source.help","258"},		// HELP_source_kill_dialog
	{"source.help","259"},		// HELP_source_ignore_signals_dialog

	// Source window, Properties menu
	{"source.help","26"},		// HELP_source_properties_menu
	{"source.help","261"},		// HELP_source_path_dialog
	{"source.help","262"},		// HELP_source_language_dialog
	{"source.help","263"},		// HELP_source_granularity_dialog

	// Disassembly window, File menu
	{"dis.help","3"},		// HELP_dis_window,
	{"dis.help","31"},		// HELP_dis_file_menu,
	{"dis.help","311"},		// HELP_dis_windows_menu,
	{"dis.help","312"},		// HELP_dis_dismiss_cmd,
	{"dis.help","313"},		// HELP_dis_quit_cmd,

	// Disassembly window, Edit menu
	{"dis.help","32"},		// HELP_dis_edit_menu,
	{"dis.help","321"},		// HELP_dis_copy,

	// Disassembly window, View menu
	{"dis.help","33"},		// HELP_dis_view_menu,
	{"dis.help","331"},		// HELP_dis_show_location_dialog,
	{"dis.help","332"},		// HELP_dis_show_function_dialog,
	{"dis.help","333"},		// HELP_dis_search_dialog,
	{"dis.help","334"},		// HELP_dis_show_value_dialog,
	{"dis.help","335"},		// HELP_dis_set_value_dialog,

	// Disassembly window, Control menu
	{"dis.help","34"},		// HELP_dis_control_menu,
	{"dis.help","341"},		// HELP_dis_run_cmd,
	{"dis.help","342"},		// HELP_dis_return_cmd,
	{"dis.help","343"},		// HELP_dis_run_dialog,
	{"dis.help","344"},		// HELP_dis_step_stmt_cmd,
	{"dis.help","345"},		// HELP_dis_step_instr_cmd,
	{"dis.help","346"},		// HELP_dis_next_stmt_cmd,
	{"dis.help","347"},		// HELP_dis_next_instr_cmd,
	{"dis.help","348"},		// HELP_dis_step_dialog,
	{"dis.help","349"},		// HELP_dis_jump_dialog,
	{"dis.help","340"},		// HELP_dis_halt_cmd,

	// Disassembly window, Event menu
	{"dis.help","35"},		// HELP_dis_event_menu
	{"dis.help","351"},		// HELP_dis_set_breakpoint
	{"dis.help","352"},		// HELP_dis_delete_breakpoint
	{"dis.help","353"},		// HELP_dis_stop_dialog
	{"dis.help","354"},		// HELP_dis_signal_dialog
	{"dis.help","355"},		// HELP_dis_syscall_dialog
	{"dis.help","356"},		// HELP_dis_onstop_dialog
	{"dis.help","357"},		// HELP_dis_cancel_dialog
	{"dis.help","358"},		// HELP_dis_kill_dialog
	{"dis.help","359"},		// HELP_dis_ignore_signals_dialog

	// Event window, File menu
	{"event.help","4"},		// HELP_event_window
	{"event.help","41"},		// HELP_event_file_menu
	{"event.help","411"},		// HELP_event_windows_menu
	{"event.help","412"},		// HELP_event_dismiss_cmd
	{"event.help","413"},		// HELP_event_quit_cmd

	// Event window, Edit menu
	{"event.help","42"},		// HELP_event_editmenu
	{"event.help","421"},		// HELP_event_disable_cmd
	{"event.help","422"},		// HELP_event_enable_cmd
	{"event.help","423"},		// HELP_event_delete_cmd

	// Event window, Event menu
	{"event.help","43"},		// HELP_event_event_menu
	{"event.help","431"},		// HELP_event_change_dialog
	{"event.help","432"},		// HELP_event_stop_dialog
	{"event.help","433"},		// HELP_event_signal_dialog
	{"event.help","434"},		// HELP_event_syscall_dialog
	{"event.help","435"},		// HELP_event_onstop_dialog
	{"event.help","436"},		// HELP_event_ignore_signals

	// Event window, Properties menu
	{"event.help","44"},		// HELP_event_properties_menu
	{"event.help","441"},		// HELP_event_panes_dialog
	{"event.help","442"},		// HELP_event_granularity_dialog

	// Command window
	{"command.help","5"},		// HELP_command_window
	{"command.help","51"},		// HELP_transcript_pane
	{"command.help","52"},		// HELP_command_line

	// Command window, File menu
	{"command.help","53"},		// HELP_cmd_file
	{"command.help","531"},		// HELP_cmd_cd_dialog
	{"command.help","532"},		// HELP_cmd_script_dialog
	{"command.help","533"},		// HELP_cmd_windows_cmd
	{"command.help","534"},		// HELP_cmd_dismiss_cmd
	{"command.help","535"},		// HELP_cmd_quit_cmd

	// Command window, Edit menu
	{"command.help","54"},		// HELP_cmd_edit_menu
	{"command.help","541"},		// HELP_cmd_copy_cmd
	{"command.help","542"},		// HELP_cmd_input_dialog,
	{"command.help","543"},		// HELP_interrupt_cmd

	// Command window, Properties menu
	{"command.help","55"},		// HELP_cmd_properties_menu
	{"command.help","551"},		// HELP_cmd_action_dialog,
	{"command.help","552"},		// HELP_cmd_path_dialog
	{"command.help","553"},		// HELP_cmd_language_dialog

	// Popup windows
	{"popup.help","601"},		// HELP_cancel_dialog,
	{"popup.help","602"},		// HELP_change_dialog,
	{"popup.help","603"},		// HELP_cd_dialog,
	{"popup.help","604"},		// HELP_create_dialog,
	{"popup.help","605"},		// HELP_dump_dialog,
	{"popup.help","606"},		// HELP_expand_dialog,
	{"popup.help","607"},		// HELP_grab_core_dialog,
	{"popup.help","608"},		// HELP_grab_process_dialog,
	{"popup.help","609"},		// HELP_granularity_dialog,
	{"popup.help","610"},		// HELP_ignore_signals_dialog,
	{"popup.help","611"},		// HELP_input_dialog,
	{"popup.help","612"},		// HELP_jump_dialog,
	{"popup.help","613"},		// HELP_kill_dialog,
	{"popup.help","614"},		// HELP_language_dialog,
	{"popup.help","615"},		// HELP_map_dialog,
	{"popup.help","616"},		// HELP_on_stop_dialog,
	{"popup.help","617"},		// HELP_open_dialog,
	{"popup.help","618"},		// HELP_action_dialog,
	{"popup.help","619"},		// HELP_panes1_dialog,
	{"popup.help","620"},		// HELP_panes2_dialog,
	{"popup.help","621"},		// HELP_recreate_dialog,
	{"popup.help","622"},		// HELP_run_dialog,
	{"popup.help","623"},		// HELP_script_dialog,
	{"popup.help","624"},		// HELP_search_dialog,
	{"popup.help","625"},		// HELP_set_value_dialog,
	{"popup.help","626"},		// HELP_show_function_dialog_1,
	{"popup.help","627"},		// HELP_show_function_dialog_2,
	{"popup.help","628"},		// HELP_show_line_dialog,
	{"popup.help","629"},		// HELP_show_location_dialog,
	{"popup.help","630"},		// HELP_show_value_dialog,
	{"popup.help","631"},		// HELP_signal_dialog,
	{"popup.help","632"},		// HELP_source_path_dialog,
	{"popup.help","633"},		// HELP_step_dialog,
	{"popup.help","634"},		// HELP_stop_dialog,
	{"popup.help","635"},		// HELP_stop_on_function_dialog,
	{"popup.help","636"},		// HELP_symbols_dialog,
	{"popup.help","637"},		// HELP_syscall_dialog,
};
