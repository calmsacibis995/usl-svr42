# $XConsortium: todis2.sed,v 1.1 91/02/16 09:51:45 rws Exp $
#  Copyright (c) 1990 by Sun Microsystems, Inc.
#  A Sed script for converting to the PHIGS DIS C binding 
#
# Pinq_type;
s/PSET/PINQ_SET/g
s/PREALIZED/PINQ_REALIZED/g
# Pint_style;
s/PHOLLOW/PSTYLE_HOLLOW/g
s/PSOLID/PSTYLE_SOLID/g
s/PPATTERN/PSTYLE_PAT/g
s/PHATCH/PSTYLE_HATCH/g
s/PEMPTY/PSTYLE_EMPTY/g
s/PVIS_OFF/PEDGE_OFF/g
s/PVIS_ON/PEDGE_ON/g
# Pin_class;
s/PIN_LOCATOR/PIN_LOC/g
s/PIN_VALUATOR/PIN_VAL/g
# Pstatus;
s/Pstatus/Pin_status/g
s/POK/PIN_STATUS_OK/g
s/PNONE/PIN_STATUS_NONE/g
s/PNOINPUT/PIN_STATUS_NO_IN/g
# Ppfcf;
s/Ppfcf/Pline_fill_ctrl_flag/g
s/PPF_POLYLINE/PFLAG_LINE/g
s/PPF_FILLAREA/PFLAG_FILL/g
s/PPF_FILLAREA_SET/PFLAG_FILL_SET/g
s/PMORE/PSIMULT_MORE/g
s/PMO/PCAT_MO/g
# Pmod_mode;
s/PNIVE/PMODE_NIVE/g
s/PUWOR/PMODE_UWOR/g
s/PUQUM/PMODE_UQUM/g
# Psimult_events;
s/Psimult_events/Pmore_simult_events/g
s/PNOMORE/PSIMULT_NO_MORE/g
# Pop_mode;
s/PREQUEST/POP_REQ/g
s/PSAMPLE/POP_SAMPLE/g
s/PEVENT/POP_EVENT/g
# Ppath_order;
s/PTOP_FIRST/PORDER_TOP_FIRST/g
s/PBOTTOM_FIRST/PORDER_BOTTOM_FIRST/g
# Pproj_type;
s/PPARALLEL/PTYPE_PARAL/g
s/PPERSPECTIVE/PTYPE_PERSPECT/g
# Pprompt_switch;
s/Pprompt_switch/Ppr_switch/g
# Pref_flag;
s/PDELETE/PFLAG_DELETE/g
s/PKEEP/PFLAG_KEEP/g
# Pregen_flag;
s/PPOSTPONE/PUPD_POSTPONE/g
s/PPERFORM/PUPD_PERFORM/g
# Ppri;
s/Ppri/Prel_pri/g
s/PHIGHER/PPRI_HIGHER/g
s/PLOWER/PPRI_LOWER/g
# Psearch_dir;
s/PBACKWARD/PDIR_BACKWARD/g
s/PFORWARD/PDIR_FORWARD/g
# Psearch_status;
s/PFAILURE/PSEARCH_STATUS_FAILURE/g
s/PSUCCESS/PSEARCH_STATUS_SUCCESS/g
# Pstruct_net_source;
s/PCSS/PNET_CSS/g
s/PARCHIVE/PNET_ARCHIVE/g
# Pstruct_st
s/PSTCL/PSTRUCT_ST_STCL/g
s/PSTOP/PSTRUCT_ST_STOP/g
# Pstruct_status;
s/PSTAT_NON_EXISTENT/PSTRUCT_STATUS_NON_EXISTENT/g
s/PSTAT_EMPTY/PSTRUCT_STATUS_EMPTY/g
s/PSTAT_NON_EMPTY/PSTRUCT_STATUS_NON_EMPTY/g
# Psys_st;
s/PPHCL/PSYS_ST_PHCL/g
s/PPHOP/PSYS_ST_PHOP/g
# Pupd_st;
s/PNOTPENDING/PUPD_NOT_PENDING/g
s/PPENDING/PUPD_PENDING/g
# Pvert_align;
s/PVERT_NORMAL/PVERT_NORM/g
# Pvisual_st;
s/PCORRECT/PVISUAL_ST_CORRECT/g
s/PDEFERRED/PVISUAL_ST_DEFER/g
s/PSIMULATED/PVISUAL_ST_SIMULATED/g
# Pws_cat;
s/POUTPUT/PCAT_OUT/g
s/PINPUT/PCAT_IN/g
s/POUTIN/PCAT_OUTIN/g
s/PMI/PCAT_MI/g
# Pws_class;
s/PVECTOR/PCLASS_VEC/g
s/PRASTER/PCLASS_RASTER/g
s/POTHER/PCLASS_OTHER/g
# Pws_dep_ind
s/PWORKSTATION_INDEPENDENT/PWS_INDEP/g
s/PWORKSTATION_DEPENDENT/PWS_DEP/g
# Pws_st;
s/PWSCL/PWS_ST_WSCL/g
s/PWSOP/PWS_ST_WSOP/g
# Pdistg_mode;
s/Pdistg_mode/Pdisting_mode/g
s/PDISTG_NO/PDISTING_NO/g
s/PDISTG_YES/PDISTING_YES/g
s/PF_LOCATOR/PIN_LOC/g
s/PF_PICK/PIN_PICK/g
s/PF_STROKE/PIN_STROKE/g
s/PF_CHOICE/PIN_CHOICE/g
s/PF_VALUATOR/PIN_VAL/g
s/PF_STRING/PIN_STRING/g
s/pexecute_struct/pexec_struct/g
s/pret_ances_struct/pret_paths_ances/g
s/pret_descs_struct/pret_paths_descs/g
s/pinq_hlhsr_ids/pinq_hlhsr_id_facs/g
s/pinq_hlhsr_modes/pinq_hlhsr_mode_facs/g
s/pinq_ances_struct/pinq_paths_ances/g
s/pinq_descs_struct/pinq_paths_descs/g
s/PLN_SOLID/PLINE_SOLID/g
s/PLN_DASH/PLINE_DASH/g
s/PLN_DOTDASH/PLINE_DOT_DASH/g
s/PLN_DOT/PLINE_DOT/g
s/PMK_POINT/PMARKER_DOT/g
s/PMK_PLUS/PMARKER_PLUS/g
s/PMK_STAR/PMARKER_ASTERISK/g
s/PMK_O/PMARKER_CIRCLE/g
s/PMK_X/PMARKER_CROSS/g
s/PAN_UNCON/PANNO_STYLE_UNCONNECTED/g
s/PAN_LEADLINE/PANNO_STYLE_LEAD_LINE/g
s/PRGB/PMODEL_RGB/g
s/PCIE/PMODEL_CIELUV/g
s/PHSV/PMODEL_HSV/g
s/PHLS/PMODEL_HLS/g
s/PDEFAULT_MEM_SIZE/PDEF_MEM_SIZE/g
s/POFF/PEDGE_OFF/g
s/Pchar/char/g
s/Pvector/Pvec/g
s/Pvector3/Pvec3/g
s/Pexecute_ref/Pelem_ref/g
s/Pcolr_bundle/Pcolr_rep/g
s/Pdisp_size/Pdisp_space_size/g
s/Pdyn_mod_struct/Pdyns_structs/g
s/Pmod_ws/Pdyns_ws_attrs/g
s/Ppat_bundle/Ppat_rep/g
s/Pnum_dev/Pnum_in/g
s/Pfont_prec/Ptext_font_prec/g
s/Pview /Pview_rep /g
s/Pview	/Pview_rep	/g
s/Pview;/Pview_rep;/g
s/Pview3/Pview_rep3/g
s/Pws_tables/Pws_st_tables/g
s/Ppara_surf_charac/Ppara_surf_characs/g
s/Pflush_class/Pin_class/g
s/Pvisibility/Pedge_flag/g
s/Pdims/Pint_size/g
s/Pext_line_bundle/Pline_bundle_plus/g
s/Pext_marker_bundle/Pmarker_bundle_plus/g
s/Pext_int_bundle/Pint_bundle_plus/g
s/Pext_text_bundle/Ptext_bundle_plus/g
s/Pext_edge_bundle/Pedge_bundle_plus/g
s/Pext_pat_bundle/Ppat_rep_plus/g
s/Pext_line_facs/Pline_facs_plus/g
s/Pext_int_facs/Pint_facs_plus/g
s/Pext_ws_tables/Pws_tables_plus/g
s/Parea_props/Prefl_props/g
s/Plong/Pint/g
s/Pwstype/Pint/g
s/Pws_type_list/Pint_list/g
