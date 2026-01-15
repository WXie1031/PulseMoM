//
// MATLAB Compiler: 6.0 (R2015a)
// Date: Wed Feb 08 10:58:27 2017
// Arguments: "-B" "macro_default" "-W" "cpplib:matGrid" "-T" "link:lib" "-d"
// "E:\LSP_Project\MatProgram\matGrid\for_testing" "-v"
// "E:\LSP_Project\MatProgram\gnd2layer\grid_rod_resis_h.m"
// "E:\LSP_Project\MatProgram\gnd2layer\grid_rod_resis_v.m"
// "E:\LSP_Project\MatProgram\gnd2layer\para_grid_cir_fix_frq.m"
// "E:\LSP_Project\MatProgram\spicenet\spice_grid_normal.m"
// "E:\LSP_Project\MatProgram\spicenet\spice_grid_self_vf.m"
// "E:\LSP_Project\MatProgram\spicenet\spice_grid_y_mtx_vf_na.m"
// "E:\LSP_Project\MatProgram\spicenet\spice_grid_y_na.m"
// "E:\LSP_Project\MatProgram\spicenet\spice_grid_ym_mtx_vf_na.m"
// "E:\LSP_Project\MatProgram\update_ground_to_main.m" "-a"
// "E:\LSP_Project\MatProgram\cal_L_fila.m" "-a"
// "E:\LSP_Project\MatProgram\gauss_int_coef.m" "-a"
// "E:\LSP_Project\MatProgram\gmdcal\gmd_agi_self.m" "-a"
// "E:\LSP_Project\MatProgram\gmdcal\gmd_rec_rec.m" "-a"
// "E:\LSP_Project\MatProgram\gmdcal\gmd_rec_sub.m" "-a"
// "E:\LSP_Project\MatProgram\gmdcal\gmd_self_rec.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_agi_ac.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_bar_ac.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_bar_ext.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_bar_Grover.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_bar_in_dc_appr.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_cir_ext.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_gmd.m" "-a"
// "E:\LSP_Project\MatProgram\induct\induct_tape.m" "-a"
// "E:\LSP_Project\MatProgram\intpeec\int_fila_a_anal.m" "-a"
// "E:\LSP_Project\MatProgram\intpeec\int_fila_p.m" "-a"
// "E:\LSP_Project\MatProgram\intpeec\int_tape_p_p3d.m" "-a"
// "E:\LSP_Project\MatProgram\intpeec\int_tape_p_p3d_sub.m" "-a"
// "E:\LSP_Project\MatProgram\intpeec\int_tape_v_p3d.m" "-a"
// "E:\LSP_Project\MatProgram\intpeec\int_tape_v_p3d_sub.m" "-a"
// "E:\LSP_Project\MatProgram\resis\resis_agi_ac.m" "-a"
// "E:\LSP_Project\MatProgram\mag2d\resis_bar_ac.m" "-a"
// "E:\LSP_Project\MatProgram\resis\zin_cir_ac.m" 
//

#ifndef __matGrid_h
#define __matGrid_h 1

#if defined(__cplusplus) && !defined(mclmcrrt_h) && defined(__linux__)
#  pragma implementation "mclmcrrt.h"
#endif
#include "mclmcrrt.h"
#include "mclcppclass.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__SUNPRO_CC)
/* Solaris shared libraries use __global, rather than mapfiles
 * to define the API exported from a shared library. __global is
 * only necessary when building the library -- files including
 * this header file to use the library do not need the __global
 * declaration; hence the EXPORTING_<library> logic.
 */

#ifdef EXPORTING_matGrid
#define PUBLIC_matGrid_C_API __global
#else
#define PUBLIC_matGrid_C_API /* No import statement needed. */
#endif

#define LIB_matGrid_C_API PUBLIC_matGrid_C_API

#elif defined(_HPUX_SOURCE)

#ifdef EXPORTING_matGrid
#define PUBLIC_matGrid_C_API __declspec(dllexport)
#else
#define PUBLIC_matGrid_C_API __declspec(dllimport)
#endif

#define LIB_matGrid_C_API PUBLIC_matGrid_C_API


#else

#define LIB_matGrid_C_API

#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_matGrid_C_API 
#define LIB_matGrid_C_API /* No special import/export declaration */
#endif

extern LIB_matGrid_C_API 
bool MW_CALL_CONV matGridInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV matGridInitialize(void);

extern LIB_matGrid_C_API 
void MW_CALL_CONV matGridTerminate(void);



extern LIB_matGrid_C_API 
void MW_CALL_CONV matGridPrintStackTrace(void);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_h(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_v(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxPara_grid_cir_fix_frq(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                           *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_normal(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                       *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_self_vf(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                        *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_y_mtx_vf_na(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                            *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_y_na(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                     *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_ym_mtx_vf_na(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                             *prhs[]);

extern LIB_matGrid_C_API 
bool MW_CALL_CONV mlxUpdate_ground_to_main(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                           *prhs[]);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__BORLANDC__)

#ifdef EXPORTING_matGrid
#define PUBLIC_matGrid_CPP_API __declspec(dllexport)
#else
#define PUBLIC_matGrid_CPP_API __declspec(dllimport)
#endif

#define LIB_matGrid_CPP_API PUBLIC_matGrid_CPP_API

#else

#if !defined(LIB_matGrid_CPP_API)
#if defined(LIB_matGrid_C_API)
#define LIB_matGrid_CPP_API LIB_matGrid_C_API
#else
#define LIB_matGrid_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_matGrid_CPP_API void MW_CALL_CONV grid_rod_resis_h(int nargout, mwArray& Rgrod, const mwArray& pt_start, const mwArray& pt_end, const mwArray& dv, const mwArray& re, const mwArray& len, const mwArray& Rsoil);

extern LIB_matGrid_CPP_API void MW_CALL_CONV grid_rod_resis_v(int nargout, mwArray& Rgrod, const mwArray& pt_start, const mwArray& pt_end, const mwArray& dv, const mwArray& re, const mwArray& len, const mwArray& Rsoil);

extern LIB_matGrid_CPP_API void MW_CALL_CONV para_grid_cir_fix_frq(int nargout, mwArray& Rgrid, mwArray& Cgrid, const mwArray& pt_start, const mwArray& pt_end, const mwArray& dv, const mwArray& re, const mwArray& len, const mwArray& sig_soil, const mwArray& epr_soil);

extern LIB_matGrid_CPP_API void MW_CALL_CONV spice_grid_normal(const mwArray& Rgrid, const mwArray& Lgrid, const mwArray& Rgrod, const mwArray& bran_name_grid, const mwArray& nod_start_grid, const mwArray& nod_end_grid, const mwArray& bran_type_grid, const mwArray& nod_out_grid, const mwArray& nod_gnd_grid, const mwArray& Rsoil, const mwArray& fpath_grid, const mwArray& fname_grid);

extern LIB_matGrid_CPP_API void MW_CALL_CONV spice_grid_self_vf(const mwArray& Rgrid, const mwArray& Lgrid, const mwArray& Grid, const mwArray& Cgrid, const mwArray& Rgrod, const mwArray& Rfit_grid, const mwArray& Lfit_grid, const mwArray& bran_name_grid, const mwArray& nod_start_grid, const mwArray& nod_end_grid, const mwArray& nod_grid, const mwArray& bran_out_grid, const mwArray& nod_out_grid, const mwArray& R_soil, const mwArray& fpath_grid, const mwArray& fname_grid);

extern LIB_matGrid_CPP_API void MW_CALL_CONV spice_grid_y_mtx_vf_na(const mwArray& dfit3, const mwArray& efit3, const mwArray& rfit3, const mwArray& pfit3, const mwArray& nod_grid, const mwArray& nod_out_grid, const mwArray& Rsoil, const mwArray& fpath_grid, const mwArray& fname_grid, const mwArray& flag_ver);

extern LIB_matGrid_CPP_API void MW_CALL_CONV spice_grid_y_na(const mwArray& Rmtx, const mwArray& Lmtx, const mwArray& Rgrid, const mwArray& Cgrid, const mwArray& bran_name_grid, const mwArray& nod_start_grid, const mwArray& nod_end_grid, const mwArray& bran_out_grid, const mwArray& nod_out_grid, const mwArray& Rsoil, const mwArray& fpath_grid, const mwArray& fname_grid);

extern LIB_matGrid_CPP_API void MW_CALL_CONV spice_grid_ym_mtx_vf_na(const mwArray& dfit3, const mwArray& efit3, const mwArray& rfit3, const mwArray& pfit3, const mwArray& nod_grid, const mwArray& nod_out_grid, const mwArray& Rsoil, const mwArray& fpath_grid, const mwArray& fname_grid, const mwArray& flag_Rg);

extern LIB_matGrid_CPP_API void MW_CALL_CONV update_ground_to_main(int nargout, mwArray& Rmtx, mwArray& Lmtx, mwArray& Rself, mwArray& Lself, const mwArray& Rmtx_in1, const mwArray& Lmtx_in1, const mwArray& Rself_in1, const mwArray& Lself_in1, const mwArray& Rg, const mwArray& Lg, const mwArray& Rgself, const mwArray& Lgself, const mwArray& ver);

#endif
#endif
