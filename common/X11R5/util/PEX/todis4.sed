# $XConsortium: todis4.sed,v 1.2 91/02/16 09:51:47 rws Exp $
#  Copyright (c) 1990 by Sun Microsystems, Inc.
#  A Sed script for converting to the PHIGS DIS C binding 
#
s/\.ln/\.line_attrs\/* WARNING: may be ln *\//g
s/\.index[^a-zA-Z_]/\.ind \/* WARNING: may be index *\//g
s/->ln/->line_attrs\/* WARNING: may be ln *\//g
s/->index[^a-zA-Z_]/->ind \/* WARNING: may be index *\//g
# misc changes that won't fit in the other files
# PHIGS PLUS function name changes
s/pext_cell_array3/pcell_array3_plus/g
s/ptri3_data/ptri_strip3_data/g
s/pquad3_data/pquad_mesh3_data/g
s/pset_area_prop/pset_refl_props/g
s/pset_int_reflect_eq/pset_refl_eqn/g
s/pset_back_area_prop/pset_back_refl_props/g
s/pset_back_int_reflect_eq/pset_back_refl_eqn/g
s/pset_ext_line_rep/pset_line_rep_plus/g
s/pset_ext_marker_rep/pset_marker_rep_plus/g
s/pset_ext_text_rep/pset_text_rep_plus/g
s/pset_ext_int_rep/pset_int_rep_plus/g
s/pset_ext_edge_rep/pset_edge_rep_plus/g
s/pset_ext_pat_rep/pset_pat_rep_plus/g
s/pinq_ext_line_rep/pinq_line_rep_plus/g
s/pinq_ext_marker_rep/pinq_marker_rep_plus/g
s/pinq_ext_text_rep/pinq_text_rep_plus/g
s/pinq_ext_int_rep/pinq_int_rep_plus/g
s/pinq_ext_edge_rep/pinq_edge_rep_plus/g
s/pinq_ext_pat_rep/pinq_pat_rep_plus/g
s/pinq_ext_line_facs/pinq_line_facs_plus/g
s/pinq_pred_ext_line_rep/pinq_pred_line_rep_plus/g
s/pinq_pred_ext_marker_rep/pinq_pred_marker_rep_plus/g
s/pinq_pred_ext_text_rep/pinq_pred_text_rep_plus/g
s/pinq_ext_int_facs/pinq_int_facs_plus/g
s/pinq_pred_ext_int_rep/pinq_pred_int_rep_plus/g
s/pinq_pred_ext_edge_rep/pinq_pred_edge_rep_plus/g
s/pinq_pred_ext_pat_rep/pinq_pred_pat_rep_plus/g
s/pinq_ext_ws_st_table/pinq_ws_st_table_plus/g
s/pset_line_shade_method/pset_line_shad_meth/g
s/pset_int_shade_method/pset_int_shad_meth/g
s/pset_back_int_shade_method/pset_back_int_shad_meth/g
s/pset_face_distg_mode/pset_face_disting_mode/g
s/pset_para_surf_charac/pset_para_surf_characs/g
s/PEFTN2000/PE_FTN_2000/g
s/PEFTN2001/PE_FTN_2001/g
s/PEFTN2002/PE_FTN_2002/g
s/PEFTN2003/PE_FTN_2003/g
s/PEFTN2004/PE_FTN_2004/g
s/PEFTN2005/PE_FTN_2005/g
s/PEFTN2006/PE_FTN_2006/g
s/ppolyline[3]*/\/\*WARNING: parameters changed \*\/ &/g
s/ppolymarker[3]*/\/\*WARNING: parameters changed \*\/ &/g
s/pfill_area3/\/\*WARNING: parameters changed \*\/ &/g
s/pfill_area[^_3]/\/\*WARNING: parameters changed \*\/ &/g
s/pcell_array3/\/\*WARNING: parameters changed \*\/ &/g
s/pcell_array[^_3]/\/\*WARNING: parameters changed \*\/ &/g
s/pgdp[3]*/\/\*WARNING: parameters changed \*\/ &/g
s/pset_pat_size/\/\*WARNING: parameters changed t\*\/ &/g
s/pinq_phigs_facs/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_pick_st[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_anno_facs/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_int_facs[^3]/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_int_gdp[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_def_loc_data[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_def_stroke_data[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_def_val_data[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_def_choice_data[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_def_pick_data[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pinq_def_string_data[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pincr_spa_search[3]*/\/\*WARNING: parameters changed\*\/ &/g
s/pescape/\/\*WARNING: parameters changed\*\/ &/g
s/pcreate_store/\/\*WARNING: parameters changed \*\/ &/g
