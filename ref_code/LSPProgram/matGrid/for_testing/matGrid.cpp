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

#include <stdio.h>
#define EXPORTING_matGrid 1
#include "matGrid.h"

static HMCRINSTANCE _mcr_inst = NULL;


#if defined( _MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__LCC__)
#ifdef __LCC__
#undef EXTERN_C
#endif
#include <windows.h>

static char path_to_dll[_MAX_PATH];

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, void *pv)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (GetModuleFileName(hInstance, path_to_dll, _MAX_PATH) == 0)
            return FALSE;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
    }
    return TRUE;
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

static int mclDefaultPrintHandler(const char *s)
{
  return mclWrite(1 /* stdout */, s, sizeof(char)*strlen(s));
}

#ifdef __cplusplus
} /* End extern "C" block */
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int mclDefaultErrorHandler(const char *s)
{
  int written = 0;
  size_t len = 0;
  len = strlen(s);
  written = mclWrite(2 /* stderr */, s, sizeof(char)*len);
  if (len > 0 && s[ len-1 ] != '\n')
    written += mclWrite(2 /* stderr */, "\n", sizeof(char));
  return written;
}

#ifdef __cplusplus
} /* End extern "C" block */
#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_matGrid_C_API
#define LIB_matGrid_C_API /* No special import/export declaration */
#endif

LIB_matGrid_C_API 
bool MW_CALL_CONV matGridInitializeWithHandlers(
    mclOutputHandlerFcn error_handler,
    mclOutputHandlerFcn print_handler)
{
    int bResult = 0;
  if (_mcr_inst != NULL)
    return true;
  if (!mclmcrInitialize())
    return false;
  if (!GetModuleFileName(GetModuleHandle("matGrid"), path_to_dll, _MAX_PATH))
    return false;
    {
        mclCtfStream ctfStream = 
            mclGetEmbeddedCtfStream(path_to_dll);
        if (ctfStream) {
            bResult = mclInitializeComponentInstanceEmbedded(   &_mcr_inst,
                                                                error_handler, 
                                                                print_handler,
                                                                ctfStream);
            mclDestroyStream(ctfStream);
        } else {
            bResult = 0;
        }
    }  
    if (!bResult)
    return false;
  return true;
}

LIB_matGrid_C_API 
bool MW_CALL_CONV matGridInitialize(void)
{
  return matGridInitializeWithHandlers(mclDefaultErrorHandler, mclDefaultPrintHandler);
}

LIB_matGrid_C_API 
void MW_CALL_CONV matGridTerminate(void)
{
  if (_mcr_inst != NULL)
    mclTerminateInstance(&_mcr_inst);
}

LIB_matGrid_C_API 
void MW_CALL_CONV matGridPrintStackTrace(void) 
{
  char** stackTrace;
  int stackDepth = mclGetStackTrace(&stackTrace);
  int i;
  for(i=0; i<stackDepth; i++)
  {
    mclWrite(2 /* stderr */, stackTrace[i], sizeof(char)*strlen(stackTrace[i]));
    mclWrite(2 /* stderr */, "\n", sizeof(char)*strlen("\n"));
  }
  mclFreeStackTrace(&stackTrace, stackDepth);
}


LIB_matGrid_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_h(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[])
{
  return mclFeval(_mcr_inst, "grid_rod_resis_h", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_v(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[])
{
  return mclFeval(_mcr_inst, "grid_rod_resis_v", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxPara_grid_cir_fix_frq(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                           *prhs[])
{
  return mclFeval(_mcr_inst, "para_grid_cir_fix_frq", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_normal(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                       *prhs[])
{
  return mclFeval(_mcr_inst, "spice_grid_normal", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_self_vf(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                        *prhs[])
{
  return mclFeval(_mcr_inst, "spice_grid_self_vf", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_y_mtx_vf_na(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                            *prhs[])
{
  return mclFeval(_mcr_inst, "spice_grid_y_mtx_vf_na", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_y_na(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
  return mclFeval(_mcr_inst, "spice_grid_y_na", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxSpice_grid_ym_mtx_vf_na(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                             *prhs[])
{
  return mclFeval(_mcr_inst, "spice_grid_ym_mtx_vf_na", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_C_API 
bool MW_CALL_CONV mlxUpdate_ground_to_main(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                           *prhs[])
{
  return mclFeval(_mcr_inst, "update_ground_to_main", nlhs, plhs, nrhs, prhs);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV grid_rod_resis_h(int nargout, mwArray& Rgrod, const mwArray& pt_start, 
                                   const mwArray& pt_end, const mwArray& dv, const 
                                   mwArray& re, const mwArray& len, const mwArray& Rsoil)
{
  mclcppMlfFeval(_mcr_inst, "grid_rod_resis_h", nargout, 1, 6, &Rgrod, &pt_start, &pt_end, &dv, &re, &len, &Rsoil);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV grid_rod_resis_v(int nargout, mwArray& Rgrod, const mwArray& pt_start, 
                                   const mwArray& pt_end, const mwArray& dv, const 
                                   mwArray& re, const mwArray& len, const mwArray& Rsoil)
{
  mclcppMlfFeval(_mcr_inst, "grid_rod_resis_v", nargout, 1, 6, &Rgrod, &pt_start, &pt_end, &dv, &re, &len, &Rsoil);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV para_grid_cir_fix_frq(int nargout, mwArray& Rgrid, mwArray& Cgrid, 
                                        const mwArray& pt_start, const mwArray& pt_end, 
                                        const mwArray& dv, const mwArray& re, const 
                                        mwArray& len, const mwArray& sig_soil, const 
                                        mwArray& epr_soil)
{
  mclcppMlfFeval(_mcr_inst, "para_grid_cir_fix_frq", nargout, 2, 7, &Rgrid, &Cgrid, &pt_start, &pt_end, &dv, &re, &len, &sig_soil, &epr_soil);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV spice_grid_normal(const mwArray& Rgrid, const mwArray& Lgrid, const 
                                    mwArray& Rgrod, const mwArray& bran_name_grid, const 
                                    mwArray& nod_start_grid, const mwArray& nod_end_grid, 
                                    const mwArray& bran_type_grid, const mwArray& 
                                    nod_out_grid, const mwArray& nod_gnd_grid, const 
                                    mwArray& Rsoil, const mwArray& fpath_grid, const 
                                    mwArray& fname_grid)
{
  mclcppMlfFeval(_mcr_inst, "spice_grid_normal", 0, 0, 12, &Rgrid, &Lgrid, &Rgrod, &bran_name_grid, &nod_start_grid, &nod_end_grid, &bran_type_grid, &nod_out_grid, &nod_gnd_grid, &Rsoil, &fpath_grid, &fname_grid);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV spice_grid_self_vf(const mwArray& Rgrid, const mwArray& Lgrid, const 
                                     mwArray& Grid, const mwArray& Cgrid, const mwArray& 
                                     Rgrod, const mwArray& Rfit_grid, const mwArray& 
                                     Lfit_grid, const mwArray& bran_name_grid, const 
                                     mwArray& nod_start_grid, const mwArray& 
                                     nod_end_grid, const mwArray& nod_grid, const 
                                     mwArray& bran_out_grid, const mwArray& nod_out_grid, 
                                     const mwArray& R_soil, const mwArray& fpath_grid, 
                                     const mwArray& fname_grid)
{
  mclcppMlfFeval(_mcr_inst, "spice_grid_self_vf", 0, 0, 16, &Rgrid, &Lgrid, &Grid, &Cgrid, &Rgrod, &Rfit_grid, &Lfit_grid, &bran_name_grid, &nod_start_grid, &nod_end_grid, &nod_grid, &bran_out_grid, &nod_out_grid, &R_soil, &fpath_grid, &fname_grid);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV spice_grid_y_mtx_vf_na(const mwArray& dfit3, const mwArray& efit3, 
                                         const mwArray& rfit3, const mwArray& pfit3, 
                                         const mwArray& nod_grid, const mwArray& 
                                         nod_out_grid, const mwArray& Rsoil, const 
                                         mwArray& fpath_grid, const mwArray& fname_grid, 
                                         const mwArray& flag_ver)
{
  mclcppMlfFeval(_mcr_inst, "spice_grid_y_mtx_vf_na", 0, 0, 10, &dfit3, &efit3, &rfit3, &pfit3, &nod_grid, &nod_out_grid, &Rsoil, &fpath_grid, &fname_grid, &flag_ver);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV spice_grid_y_na(const mwArray& Rmtx, const mwArray& Lmtx, const 
                                  mwArray& Rgrid, const mwArray& Cgrid, const mwArray& 
                                  bran_name_grid, const mwArray& nod_start_grid, const 
                                  mwArray& nod_end_grid, const mwArray& bran_out_grid, 
                                  const mwArray& nod_out_grid, const mwArray& Rsoil, 
                                  const mwArray& fpath_grid, const mwArray& fname_grid)
{
  mclcppMlfFeval(_mcr_inst, "spice_grid_y_na", 0, 0, 12, &Rmtx, &Lmtx, &Rgrid, &Cgrid, &bran_name_grid, &nod_start_grid, &nod_end_grid, &bran_out_grid, &nod_out_grid, &Rsoil, &fpath_grid, &fname_grid);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV spice_grid_ym_mtx_vf_na(const mwArray& dfit3, const mwArray& efit3, 
                                          const mwArray& rfit3, const mwArray& pfit3, 
                                          const mwArray& nod_grid, const mwArray& 
                                          nod_out_grid, const mwArray& Rsoil, const 
                                          mwArray& fpath_grid, const mwArray& fname_grid, 
                                          const mwArray& flag_Rg)
{
  mclcppMlfFeval(_mcr_inst, "spice_grid_ym_mtx_vf_na", 0, 0, 10, &dfit3, &efit3, &rfit3, &pfit3, &nod_grid, &nod_out_grid, &Rsoil, &fpath_grid, &fname_grid, &flag_Rg);
}

LIB_matGrid_CPP_API 
void MW_CALL_CONV update_ground_to_main(int nargout, mwArray& Rmtx, mwArray& Lmtx, 
                                        mwArray& Rself, mwArray& Lself, const mwArray& 
                                        Rmtx_in1, const mwArray& Lmtx_in1, const mwArray& 
                                        Rself_in1, const mwArray& Lself_in1, const 
                                        mwArray& Rg, const mwArray& Lg, const mwArray& 
                                        Rgself, const mwArray& Lgself, const mwArray& ver)
{
  mclcppMlfFeval(_mcr_inst, "update_ground_to_main", nargout, 4, 9, &Rmtx, &Lmtx, &Rself, &Lself, &Rmtx_in1, &Lmtx_in1, &Rself_in1, &Lself_in1, &Rg, &Lg, &Rgself, &Lgself, &ver);
}

