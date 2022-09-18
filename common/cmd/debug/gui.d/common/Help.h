/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	HELP_H
#define	HELP_H
#ident	"@(#)debugger:gui.d/common/Help.h	1.11"

enum Help_id
{
	HELP_none,
	HELP_context_window,
	HELP_ps_pane,
	HELP_stack_pane,
	HELP_syms_pane,

	// Context window, File menu
	HELP_ctxt_file_menu,
	HELP_ctxt_create_dialog,
	HELP_ctxt_recreate_dialog,
	HELP_ctxt_grab_core_dialog,
	HELP_ctxt_grab_process_dialog,
	HELP_release_cmd,
	HELP_ctxt_cd_dialog,
	HELP_new_window_set_cmd,
	HELP_windows_menu,
	HELP_dismiss_cmd,
	HELP_quit_cmd,

	// Context window, Edit menu
	HELP_ctxt_edit_menu,
	HELP_set_current_cmd,
	HELP_export_cmd,

	// Context window, View menu
	HELP_ctxt_view_menu,
	HELP_ctxt_expand_dialog,
	HELP_ctxt_show_value_dialog,
	HELP_ctxt_set_value_dialog,
	HELP_ctxt_dump_dialog,
	HELP_ctxt_map_dialog,

	// Context window, Control menu
	HELP_ctxt_control_menu,
	HELP_run_cmd,
	HELP_return_cmd,
	HELP_ctxt_run_dialog,
	HELP_step_stmt_cmd,
	HELP_step_instr_cmd,
	HELP_next_stmt_cmd,
	HELP_next_instr_cmd,
	HELP_ctxt_step_dialog,
	HELP_ctxt_jump_dialog,
	HELP_halt_cmd,

	// Context window, Event menu
	HELP_ctxt_event_menu,
	HELP_ctxt_stop_on_function_dialog,
	HELP_set_watchpoint_cmd,
	HELP_ctxt_stop_dialog,
	HELP_ctxt_signal_dialog,
	HELP_ctxt_syscall_dialog,
	HELP_ctxt_on_stop_dialog,
	HELP_ctxt_cancel_dialog,
	HELP_ctxt_kill_dialog,
	HELP_ctxt_ignore_signals_dialog,

	// Context window, Properties menu
	HELP_ctxt_properties_menu,
	HELP_ctxt_symbols_dialog,
	HELP_ctxt_panes_dialog,
	HELP_ctxt_path_dialog,
	HELP_ctxt_language_dialog,
	HELP_ctxt_granularity_dialog,
	HELP_ctxt_action_dialog,

	// Source window, File Menu
	HELP_source_window,
	HELP_source_file_menu,
	HELP_source_open_dialog,
	HELP_new_source_cmd,
	HELP_source_windows_menu,
	HELP_source_dismiss_cmd,
	HELP_source_quit_cmd,

	// Source window, Edit menu
	HELP_source_edit_menu,
	HELP_copy_cmd,

	// Source window, View menu
	HELP_source_view_menu,
	HELP_source_show_line_dialog,
	HELP_source_show_function_dialog,
	HELP_source_search_dialog,
	HELP_source_show_value_dialog,
	HELP_source_set_value_dialog,

	// Source window, Control menu
	HELP_source_control_menu,
	HELP_source_run_cmd,
	HELP_source_return_cmd,
	HELP_source_run_dialog,
	HELP_source_step_stmt_cmd,
	HELP_source_step_instr_cmd,
	HELP_source_next_stmt_cmd,
	HELP_source_next_instr_cmd,
	HELP_source_step_dialog,
	HELP_source_jump_dialog,
	HELP_source_halt_cmd,

	// Source window, Event menu
	HELP_source_event_menu,
	HELP_set_breakpoint_cmd,
	HELP_delete_breakpoint_cmd,
	HELP_source_stop_dialog,
	HELP_source_signal_dialog,
	HELP_source_syscall_dialog,
	HELP_source_onstop_dialog,
	HELP_source_cancel_dialog,
	HELP_source_kill_dialog,
	HELP_source_ignore_signals_dialog,

	// Source window, Properties menu
	HELP_source_properties_menu,
	HELP_source_path_dialog,
	HELP_source_language_dialog,
	HELP_source_granularity_dialog,

	// Disassembly window, File menu
	HELP_dis_window,
	HELP_dis_file_menu,
	HELP_dis_windows_menu,
	HELP_dis_dismiss_cmd,
	HELP_dis_quit_cmd,

	// Disassembly window, Edit menu
	HELP_dis_edit_menu,
	HELP_dis_copy_cmd,

	// Disassembly window, View menu
	HELP_dis_view_menu,
	HELP_dis_show_location_dialog,
	HELP_dis_show_function_dialog,
	HELP_dis_search_dialog,
	HELP_dis_show_value_dialog,
	HELP_dis_set_value_dialog,

	// Disassembly window, Control menu
	HELP_dis_control_menu,
	HELP_dis_run_cmd,
	HELP_dis_return_cmd,
	HELP_dis_run_dialog,
	HELP_dis_step_stmt_cmd,
	HELP_dis_step_instr_cmd,
	HELP_dis_next_stmt_cmd,
	HELP_dis_next_instr_cmd,
	HELP_dis_step_dialog,
	HELP_dis_jump_dialog,
	HELP_dis_halt_cmd,

	// Disassembly window, Event menu
	HELP_dis_event_menu,
	HELP_dis_set_breakpoint_cmd,
	HELP_dis_delete_breakpoint_cmd,
	HELP_dis_stop_dialog,
	HELP_dis_signal_dialog,
	HELP_dis_syscall_dialog,
	HELP_dis_onstop_dialog,
	HELP_dis_cancel_dialog,
	HELP_dis_kill_dialog,
	HELP_dis_ignore_signals_dialog,

	// Event window, File menu
	HELP_event_window,
	HELP_event_file_menu,
	HELP_event_windows_menu,
	HELP_event_dismiss_cmd,
	HELP_event_quit_cmd,

	// Event window, Edit menu
	HELP_event_edit_menu,
	HELP_event_disable_cmd,
	HELP_event_enable_cmd,
	HELP_event_delete_cmd,

	// Event window, Event menu
	HELP_event_event_menu,
	HELP_event_change_dialog,
	HELP_event_stop_dialog,
	HELP_event_signal_dialog,
	HELP_event_syscall_dialog,
	HELP_event_onstop_dialog,
	HELP_event_ignore_signals,

	// Event window, Properties menu
	HELP_event_properties_menu,
	HELP_event_pane_dialog,
	HELP_event_granularity_dialog,

	// Command window
	HELP_command_window,
	HELP_transcript_pane,
	HELP_command_line,

	// Command window, File menu
	HELP_cmd_file_menu,
	HELP_cmd_cd_dialog,
	HELP_cmd_script_dialog,
	HELP_cmd_windows_cmd,
	HELP_cmd_dismiss_cmd,
	HELP_cmd_quit_cmd,

	// Command window, Edit menu
	HELP_cmd_edit_menu,
	HELP_cmd_copy_cmd,
	HELP_cmd_input_dialog,
	HELP_interrupt_cmd,

	// Command window, Properties menu
	HELP_cmd_properties_menu,
	HELP_cmd_action_dialog,
	HELP_cmd_path_dialog,
	HELP_cmd_language_dialog,

	// Popup windows
	HELP_cancel_dialog,
	HELP_change_dialog,
	HELP_cd_dialog,
	HELP_create_dialog,
	HELP_dump_dialog,
	HELP_expand_dialog,
	HELP_grab_core_dialog,
	HELP_grab_process_dialog,
	HELP_granularity_dialog,
	HELP_ignore_signals_dialog,
	HELP_input_dialog,
	HELP_jump_dialog,
	HELP_kill_dialog,
	HELP_language_dialog,
	HELP_map_dialog,
	HELP_on_stop_dialog,
	HELP_open_dialog,
	HELP_action_dialog,
	HELP_panes1_dialog,
	HELP_panes2_dialog,
	HELP_recreate_dialog,
	HELP_run_dialog,
	HELP_script_dialog,
	HELP_search_dialog,
	HELP_set_value_dialog,
	HELP_show_function_dialog_1,
	HELP_show_function_dialog_2,
	HELP_show_line_dialog,
	HELP_show_location_dialog,
	HELP_show_value_dialog,
	HELP_signal_dialog,
	HELP_path_dialog,
	HELP_step_dialog,
	HELP_stop_dialog,
	HELP_stop_on_function_dialog,
	HELP_symbols_dialog,
	HELP_syscall_dialog,
	HELP_final
};

enum Help_mode
{
	HM_section,	// general help for specific section
	HM_toc,		// table of contents for specific section
};

struct Help_info {
	const char *filename;	// file name of help file
	const char *section;	// section within the file
	void 	   *data;	// libol data
};

extern Help_info	Help_files[];
extern char		*Help_path;

#endif
