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

#include <stdio.h>
#define EXPORTING_matGround 1
#include "matGround.h"

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
#ifndef LIB_matGround_C_API
#define LIB_matGround_C_API /* No special import/export declaration */
#endif

LIB_matGround_C_API 
bool MW_CALL_CONV matGroundInitializeWithHandlers(
    mclOutputHandlerFcn error_handler,
    mclOutputHandlerFcn print_handler)
{
    int bResult = 0;
  if (_mcr_inst != NULL)
    return true;
  if (!mclmcrInitialize())
    return false;
  if (!GetModuleFileName(GetModuleHandle("matGround"), path_to_dll, _MAX_PATH))
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

LIB_matGround_C_API 
bool MW_CALL_CONV matGroundInitialize(void)
{
  return matGroundInitializeWithHandlers(mclDefaultErrorHandler, mclDefaultPrintHandler);
}

LIB_matGround_C_API 
void MW_CALL_CONV matGroundTerminate(void)
{
  if (_mcr_inst != NULL)
    mclTerminateInstance(&_mcr_inst);
}

LIB_matGround_C_API 
void MW_CALL_CONV matGroundPrintStackTrace(void) 
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


LIB_matGround_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_h(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[])
{
  return mclFeval(_mcr_inst, "grid_rod_resis_h", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_C_API 
bool MW_CALL_CONV mlxGrid_rod_resis_v(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                      *prhs[])
{
  return mclFeval(_mcr_inst, "grid_rod_resis_v", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_C_API 
bool MW_CALL_CONV mlxGround_cmplx_plane(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                        *prhs[])
{
  return mclFeval(_mcr_inst, "ground_cmplx_plane", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_C_API 
bool MW_CALL_CONV mlxGround_pec(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
  return mclFeval(_mcr_inst, "ground_pec", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_C_API 
bool MW_CALL_CONV mlxPara_grid_fila(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
  return mclFeval(_mcr_inst, "para_grid_fila", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_C_API 
bool MW_CALL_CONV mlxPara_grid_self(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
  return mclFeval(_mcr_inst, "para_grid_self", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_C_API 
bool MW_CALL_CONV mlxSave_grid_data_to_matlab(int nlhs, mxArray *plhs[], int nrhs, 
                                              mxArray *prhs[])
{
  return mclFeval(_mcr_inst, "save_grid_data_to_matlab", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_C_API 
bool MW_CALL_CONV mlxUpdate_ground_to_main(int nlhs, mxArray *plhs[], int nrhs, mxArray 
                                           *prhs[])
{
  return mclFeval(_mcr_inst, "update_ground_to_main", nlhs, plhs, nrhs, prhs);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV grid_rod_resis_h(int nargout, mwArray& Rgrod, const mwArray& Rgrod_in1, 
                                   const mwArray& pt_start_grid, const mwArray& 
                                   pt_end_grid, const mwArray& dv_grid, const mwArray& 
                                   re_grid, const mwArray& len_grid, const mwArray& Rsoil)
{
  mclcppMlfFeval(_mcr_inst, "grid_rod_resis_h", nargout, 1, 7, &Rgrod, &Rgrod_in1, &pt_start_grid, &pt_end_grid, &dv_grid, &re_grid, &len_grid, &Rsoil);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV grid_rod_resis_v(int nargout, mwArray& Rgrod, const mwArray& Rgrod_in1, 
                                   const mwArray& pt_start_grid, const mwArray& 
                                   pt_end_grid, const mwArray& dv_grid, const mwArray& 
                                   re_grid, const mwArray& len_grid, const mwArray& Rsoil)
{
  mclcppMlfFeval(_mcr_inst, "grid_rod_resis_v", nargout, 1, 7, &Rgrod, &Rgrod_in1, &pt_start_grid, &pt_end_grid, &dv_grid, &re_grid, &len_grid, &Rsoil);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV ground_cmplx_plane(int nargout, mwArray& Rg, mwArray& Lg, mwArray& 
                                     Rgself, mwArray& Lgself, const mwArray& pt_start, 
                                     const mwArray& pt_end, const mwArray& dv, const 
                                     mwArray& re, const mwArray& len, const mwArray& 
                                     cond_soil, const mwArray& freq, const mwArray& 
                                     offset)
{
  mclcppMlfFeval(_mcr_inst, "ground_cmplx_plane", nargout, 4, 8, &Rg, &Lg, &Rgself, &Lgself, &pt_start, &pt_end, &dv, &re, &len, &cond_soil, &freq, &offset);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV ground_pec(int nargout, mwArray& Rg, mwArray& Lg, mwArray& Rgself, 
                             mwArray& Lgself, const mwArray& pt_start, const mwArray& 
                             pt_end, const mwArray& dv, const mwArray& re, const mwArray& 
                             len, const mwArray& frq, const mwArray& offset)
{
  mclcppMlfFeval(_mcr_inst, "ground_pec", nargout, 4, 7, &Rg, &Lg, &Rgself, &Lgself, &pt_start, &pt_end, &dv, &re, &len, &frq, &offset);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV para_grid_fila(int nargout, mwArray& Rgird, mwArray& Lgrid, const 
                                 mwArray& pt_start, const mwArray& pt_end, const mwArray& 
                                 dv, const mwArray& re, const mwArray& len)
{
  mclcppMlfFeval(_mcr_inst, "para_grid_fila", nargout, 2, 5, &Rgird, &Lgrid, &pt_start, &pt_end, &dv, &re, &len);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV para_grid_self(int nargout, mwArray& Rgrid, mwArray& Lgrid, const 
                                 mwArray& Rgrid_in1, const mwArray& Lgrid_in1, const 
                                 mwArray& type_flag, const mwArray& dim1, const mwArray& 
                                 dim2, const mwArray& len, const mwArray& R_pul, const 
                                 mwArray& Lin_pul, const mwArray& mur, const mwArray& Sig)
{
  mclcppMlfFeval(_mcr_inst, "para_grid_self", nargout, 2, 10, &Rgrid, &Lgrid, &Rgrid_in1, &Lgrid_in1, &type_flag, &dim1, &dim2, &len, &R_pul, &Lin_pul, &mur, &Sig);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV save_grid_data_to_matlab(const mwArray& Rgrid, const mwArray& Lgrid, 
                                           const mwArray& Rself_grid, const mwArray& 
                                           Lself_grid, const mwArray& Rfit_grid, const 
                                           mwArray& Lfit_grid, const mwArray& 
                                           Rdc_fit_grid, const mwArray& Ldc_fit_grid, 
                                           const mwArray& pt_start_grid, const mwArray& 
                                           pt_end_grid, const mwArray& dv_grid, const 
                                           mwArray& re_grid, const mwArray& shape_grid, 
                                           const mwArray& dim1_grid, const mwArray& 
                                           dim2_grid, const mwArray& len_grid, const 
                                           mwArray& R_pul_grid, const mwArray& 
                                           Lin_pul_grid, const mwArray& sig_grid, const 
                                           mwArray& mur_grid, const mwArray& 
                                           bran_type_grid, const mwArray& bran_name_grid, 
                                           const mwArray& nod_start_grid, const mwArray& 
                                           nod_end_grid, const mwArray& bran_output_grid, 
                                           const mwArray& nod_output_grid, const mwArray& 
                                           Nbran_out_grid, const mwArray& Nnod_out_grid, 
                                           const mwArray& Nnod_gnd_grid, const mwArray& 
                                           Nfit_grid, const mwArray& frq_grid, const 
                                           mwArray& fpath_grid, const mwArray& fname_grid)
{
  mclcppMlfFeval(_mcr_inst, "save_grid_data_to_matlab", 0, 0, 33, &Rgrid, &Lgrid, &Rself_grid, &Lself_grid, &Rfit_grid, &Lfit_grid, &Rdc_fit_grid, &Ldc_fit_grid, &pt_start_grid, &pt_end_grid, &dv_grid, &re_grid, &shape_grid, &dim1_grid, &dim2_grid, &len_grid, &R_pul_grid, &Lin_pul_grid, &sig_grid, &mur_grid, &bran_type_grid, &bran_name_grid, &nod_start_grid, &nod_end_grid, &bran_output_grid, &nod_output_grid, &Nbran_out_grid, &Nnod_out_grid, &Nnod_gnd_grid, &Nfit_grid, &frq_grid, &fpath_grid, &fname_grid);
}

LIB_matGround_CPP_API 
void MW_CALL_CONV update_ground_to_main(int nargout, mwArray& Rmtx, mwArray& Lmtx, 
                                        mwArray& Rself, mwArray& Lself, const mwArray& 
                                        Rmtx_in1, const mwArray& Lmtx_in1, const mwArray& 
                                        Rself_in1, const mwArray& Lself_in1, const 
                                        mwArray& Rg, const mwArray& Lg, const mwArray& 
                                        Rgself, const mwArray& Lgself, const mwArray& ver)
{
  mclcppMlfFeval(_mcr_inst, "update_ground_to_main", nargout, 4, 9, &Rmtx, &Lmtx, &Rself, &Lself, &Rmtx_in1, &Lmtx_in1, &Rself_in1, &Lself_in1, &Rg, &Lg, &Rgself, &Lgself, &ver);
}

