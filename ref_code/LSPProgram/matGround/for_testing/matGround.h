//
// MATLAB Compiler: 6.0 (R2015a)
// Date: Fri Dec 18 00:14:42 2015
// Arguments: "-B" "macro_default" "-W" "cpplib:matGround" "-T" "link:lib" "-d"
// "D:\Cloud Data\SkyDrive Pro\Project
// Code\Mat_ParaProgram\matGround\for_testing" "-v" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\grid_rod_resis_h.m" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\grid_rod_resis_v.m" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\ground_cmplx_plane.m" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\ground_pec.m" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\para_grid_fila.m" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\para_grid_self.m" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\save_grid_data_to_matlab.m"
// "D:\Cloud Data\SkyDrive Pro\Project
// Code\Mat_ParaProgram\update_ground_to_main.m" "-a" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\besseli_sub.m" "-a" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\besselk_sub.m" "-a" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\cal_L_filament.m" "-a" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\gauss_int_coef.m" "-a"
// "D:\Cloud Data\SkyDrive Pro\Project Code\Mat_ParaProgram\gmd_agi_self.m"
// "-a" "D:\Cloud Data\SkyDrive Pro\Project Code\Mat_ParaProgram\gmd_rec_rec.m"
// "-a" "D:\Cloud Data\SkyDrive Pro\Project Code\Mat_ParaProgram\gmd_rec_sub.m"
// "-a" "D:\Cloud Data\SkyDrive Pro\Project
// Code\Mat_ParaProgram\gmd_self_rec.m" "-a" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\induct_agi_ac.m" "-a" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\induct_bar_ac.m" "-a"
// "D:\Cloud Data\SkyDrive Pro\Project
// Code\Mat_ParaProgram\induct_bar_Grover.m" "-a" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\induct_bar_in_dc_appr.m" "-a" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\induct_cir_ext.m" "-a"
// "D:\Cloud Data\SkyDrive Pro\Project Code\Mat_ParaProgram\induct_gmd.m" "-a"
// "D:\Cloud Data\SkyDrive Pro\Project Code\Mat_ParaProgram\int_line_a_anal.m"
// "-a" "D:\Cloud Data\SkyDrive Pro\Project
// Code\Mat_ParaProgram\int_line_a_num.m" "-a" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\int_line_p.m" "-a" "D:\Cloud Data\SkyDrive
// Pro\Project Code\Mat_ParaProgram\resis_agi_ac.m" "-a" "D:\Cloud
// Data\SkyDrive Pro\Project Code\Mat_ParaProgram\resis_bar_ac.m" "-a"
// "D:\Cloud Data\SkyDrive Pro\Project
// Code\Mat_ParaProgram\resis_induct_cir_ac.m" 
//

#ifndef __matGround_h
#define __matGround_h 1

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

#ifdef EXPORTING_matGround
#define PUBLIC_matGround_C_API __global
#else
#define PUBLIC_matGround_C_API /* No import statement needed. */
#endif

#define LIB_matGround_C_API PUBLIC_matGround_C_API

#elif defined(_HPUX_SOURCE)

#ifdef EXPORTING_matGround
#define PUBLIC_matGround_C_API __declspec(dllexport)
#else
#define PUBLIC_matGround_C_API __declspec(dllimport)
#endif

#define LIB_matGround_C_API PUBLIC_matGround_C_API


#else

#define LIB_matGround_C_API

#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_matGround_C_API 
#define LIB_matGround_C_API /* No special import/export declaration */
#endif

extern LIB_matGround_C_API 
bool MW_CALL_CONV matGroundInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_matGround_C_API 
bool MW_CALL_CONV matGroundInitialize(void);

extern LIB_matGround_C_API 
void MW_CALL_CONV matGroundTerminate(void);



extern LIB_matGround_C_API 
void MW_CALL_CONV matGroundPrintStackTrace(void);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_h(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[]);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_v(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[]);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxGround_cmplx_plane(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                        *prhs[]);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxGround_pec(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxPara_grid_fila(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxPara_grid_self(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxSave_grid_data_to_matlab(int nlhs, mxArray *plhs[], int nrhs, 
                                              mxArray *prhs[]);

extern LIB_matGround_C_API 
bool MW_CALL_CONV mlxUpdate_ground_to_main(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                           *prhs[]);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__BORLANDC__)

#ifdef EXPORTING_matGround
#define PUBLIC_matGround_CPP_API __declspec(dllexport)
#else
#define PUBLIC_matGround_CPP_API __declspec(dllimport)
#endif

#define LIB_matGround_CPP_API PUBLIC_matGround_CPP_API

#else

#if !defined(LIB_matGround_CPP_API)
#if defined(LIB_matGround_C_API)
#define LIB_matGround_CPP_API LIB_matGround_C_API
#else
#define LIB_matGround_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_matGround_CPP_API void MW_CALL_CONV grid_rod_resis_h(int nargout, mwArray& Rgrod, const mwArray& Rgrod_in1, const mwArray& pt_start_grid, const mwArray& pt_end_grid, const mwArray& dv_grid, const mwArray& re_grid, const mwArray& len_grid, const mwArray& Rsoil);

extern LIB_matGround_CPP_API void MW_CALL_CONV grid_rod_resis_v(int nargout, mwArray& Rgrod, const mwArray& Rgrod_in1, const mwArray& pt_start_grid, const mwArray& pt_end_grid, const mwArray& dv_grid, const mwArray& re_grid, const mwArray& len_grid, const mwArray& Rsoil);

extern LIB_matGround_CPP_API void MW_CALL_CONV ground_cmplx_plane(int nargout, mwArray& Rg, mwArray& Lg, mwArray& Rgself, mwArray& Lgself, const mwArray& pt_start, const mwArray& pt_end, const mwArray& dv, const mwArray& re, const mwArray& len, const mwArray& cond_soil, const mwArray& freq, const mwArray& offset);

extern LIB_matGround_CPP_API void MW_CALL_CONV ground_pec(int nargout, mwArray& Rg, mwArray& Lg, mwArray& Rgself, mwArray& Lgself, const mwArray& pt_start, const mwArray& pt_end, const mwArray& dv, const mwArray& re, const mwArray& len, const mwArray& frq, const mwArray& offset);

extern LIB_matGround_CPP_API void MW_CALL_CONV para_grid_fila(int nargout, mwArray& Rgird, mwArray& Lgrid, const mwArray& pt_start, const mwArray& pt_end, const mwArray& dv, const mwArray& re, const mwArray& len);

extern LIB_matGround_CPP_API void MW_CALL_CONV para_grid_self(int nargout, mwArray& Rgrid, mwArray& Lgrid, const mwArray& Rgrid_in1, const mwArray& Lgrid_in1, const mwArray& type_flag, const mwArray& dim1, const mwArray& dim2, const mwArray& len, const mwArray& R_pul, const mwArray& Lin_pul, const mwArray& mur, const mwArray& Sig);

extern LIB_matGround_CPP_API void MW_CALL_CONV save_grid_data_to_matlab(const mwArray& Rgrid, const mwArray& Lgrid, const mwArray& Rself_grid, const mwArray& Lself_grid, const mwArray& Rfit_grid, const mwArray& Lfit_grid, const mwArray& Rdc_fit_grid, const mwArray& Ldc_fit_grid, const mwArray& pt_start_grid, const mwArray& pt_end_grid, const mwArray& dv_grid, const mwArray& re_grid, const mwArray& shape_grid, const mwArray& dim1_grid, const mwArray& dim2_grid, const mwArray& len_grid, const mwArray& R_pul_grid, const mwArray& Lin_pul_grid, const mwArray& sig_grid, const mwArray& mur_grid, const mwArray& bran_type_grid, const mwArray& bran_name_grid, const mwArray& nod_start_grid, const mwArray& nod_end_grid, const mwArray& bran_output_grid, const mwArray& nod_output_grid, const mwArray& Nbran_out_grid, const mwArray& Nnod_out_grid, const mwArray& Nnod_gnd_grid, const mwArray& Nfit_grid, const mwArray& frq_grid, const mwArray& fpath_grid, const mwArray& fname_grid);

extern LIB_matGround_CPP_API void MW_CALL_CONV update_ground_to_main(int nargout, mwArray& Rmtx, mwArray& Lmtx, mwArray& Rself, mwArray& Lself, const mwArray& Rmtx_in1, const mwArray& Lmtx_in1, const mwArray& Rself_in1, const mwArray& Lself_in1, const mwArray& Rg, const mwArray& Lg, const mwArray& Rgself, const mwArray& Lgself, const mwArray& ver);

#endif
#endif
