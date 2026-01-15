function main_prog_air(pt_start,pt_end,re, shp_id,dim1,dim2, R_pul,sig,mur,epr, ...
    grp_id, grp_type, pt_2d,...
    bran_name,nod_start,nod_end,nod_sr,nod_gnd,bran_out,nod_out, ...
    soil_sig,soil_epr, gnd_off, frq_cut, Nfit,dl, fpath,fname, ...
    flag_p,flag_sol,flag_mesh,flag_gnd,flag_mor,flag_sr)
if flag_sol == 1
    frq = 50e3;
    Nfit = 1;
else
    frq_pt = [1 50 100 200 300 400 500 700 1e3 1.2e3 1.5e3 2e3 2.5e3 3e3 ...
        3.5e3 4e3 5e3 7e3 7.5e3 10e3 12e3 15e3 20e3 25e3 30e3 35e3 40e3 ...
        50e3 70e3 100e3 120e3 150e3 200e3 250e3 300e3 400e3 500e3 700e3 1e6 ...
        1.2e6 1.5e6 2e6 2.5e6 3.5e6 5e6];
    frq = frq_pt(frq_pt<=frq_cut);
end

Nfrq = length(frq);
%frq = logspace(0,log10(frq_cut),Nfrq);
mur_cof = ones(1,Nfrq);

if frq_cut<=500e3
    Nfit = min(2, Nfit);
elseif frq_cut>500e3
    Nfit = min(Nfit,3);
end


dl_type = 1;

[dv, len] = line_dv(pt_start, pt_end);

%% 1. meshing
[p_sta_L,p_end_L,dv_L,re_L,l_L, shpid_L,d1_L,d2_L, R_L, sig_L,mur_L,epr_L, ...
    p_sta_P,p_end_P,pt_P,re_P,dv_P,l_P, shpid_P,d1_P,d2_P, ...
    g_id_L,g_type_L,p_2d_L, bname,nd_start,nd_end, nd_P] = main_mesh_line_3d ...
    (pt_start,pt_end,dv,re,len, shp_id,dim1,dim2, R_pul,sig,mur,epr, ...
    grp_id,grp_type,pt_2d, bran_name,nod_start,nod_end, dl,dl_type, flag_p);



%% 2 calculate the RLP parameter
[Rmtx,Lmtx,Pmtx] = para_main_fila_rlp(p_sta_L,p_end_L,dv_L,re_L,l_L, ...
    p_sta_P,p_end_P,dv_P,re_P,l_P, flag_p);
%Lmtx = Lmtx-diag(diag(Lmtx));

if flag_sol == 1 
    [Rself,Lself] = para_self_fix_frq(shpid_L,d1_L,d2_L, ...
        l_L, R_L,sig_L,mur_L);
elseif (flag_sol == 2) || (flag_sol == 3)
    [Rself,Lself] = para_self_multi_frq(shpid_L, d1_L, d2_L, ...
        l_L, R_L, sig_L, mur_L*mur_cof, frq);
else
    Rself=R_L; Lself=diag(Lmtx);
end

%% 3 update meshing group parameter
% 1 -- pul mode (calculate pul parameter matrix and fit others)
% 2 -- complete mode (calculate complete parameter matrix)
% 3 -- NN mode (calculate parameter matrix to 100kHz, predict others)
% 4 -- NN self mode (calculate parameter matrix to 100kHz, predict self terms of others)

if flag_sol > 1
    [Rself,Lself, Rmtx,Lmtx,Pmtx] = main_mesh2d(Rself,Lself, Rmtx,Lmtx,Pmtx, ...
        p_2d_L,shpid_L, d1_L,d2_L, R_L,sig_L,mur_L,epr_L, l_L, ...
        g_id_L,g_type_L, frq_cut,frq, flag_mesh,flag_p);
end

%% 4 add ground effect
ver = 1;
if flag_gnd == 1
    [Rg,Lg,Pg,Rgself,Lgself] = ground_pec(p_sta_L,p_end_L,dv_L,re_L,l_L, ...
        p_sta_P,p_end_P,dv_P,re_P,l_P, frq, gnd_off, flag_p);
    [Rmtx, Lmtx, Pmtx, Rself, Lself] = update_ground_to_main(...
        Rmtx, Lmtx, Pmtx, Rself, Lself, Rg, Lg, Pg, Rgself, Lgself, ver);
    
elseif flag_gnd == 2
    
    [Rg,Lg,Pg,Rgself,Lgself] = ground_cmplx_plane(p_sta_L,p_end_L,dv_L,re_L,l_L, soil_sig, frq, gnd_off);
    [Rmtx, Lmtx, Pmtx, Rself, Lself] = update_ground_to_main(...
        Rmtx, Lmtx, Pmtx, Rself, Lself, Rg, Lg, Pg, Rgself, Lgself, ver);
    
else
    Rg=[]; Lg=[]; Pg=[]; Rgself=[]; Lgself=[];
end


%% 5 generate network and merge the nodes
% Pdiag = diag(diag(Pmtx));
% [Cmtx, nd_Pnew, pt_Pnew] = sol_net_p(Pmtx, p_sta_L,p_end_L,pt_P, nd_start,nd_end, nd_P);
[Cmtx, nd_Pnew, pt_Pnew] = sol_net_p(Pmtx, p_sta_L,p_end_L,pt_P, nd_start,nd_end, nd_P);

% Amtx = sol_a_mtx(nd_start, nd_end, nd_Pnew);

if flag_sol == 3
    Amtx = sol_a_mtx(nd_start, nd_end, nd_Pnew);
    Nn = size(nd_Pnew,1);
    Nout = size(nod_out,1);
    Yn = zeros(Nn,Nn,Nfrq);
    Yn_new = zeros(Nn,Nn,Nfrq);
    Yp = zeros(Nout,Nout,Nfrq);
    Tmtx = eye(Nn,Nn);
    for ik = 1:Nout
        for ig = 1:Nn
            if strcmp(deblank(nod_out(ik,:)),deblank(nd_Pnew(ig,:)))
                Ttmp = Tmtx(ig,:);
                Tmtx(ig,:) = Tmtx(ik,:);
                Tmtx(ik,:) = Ttmp;
                Tmtx = -Tmtx;
            end
        end
    end
    
    for ik = 1:Nfrq
        Rtmp = diag(Rself(:,ik));
        Ltmp = Lmtx-diag(diag(Lmtx))+diag(Lself(:,ik));
        Yn(:,:,ik) = Amtx/inv(Rtmp+1j*2*pi*frq(ik)*Ltmp)*Amtx' + Cmtx/(1j*2*pi*frq(ik));
        Yn_new(:,:,ik) = Tmtx*Yn(:,:,ik)*Tmtx';
        Yp(:,:,ik) = Yn_new(1:Nout,1:Nout) - Yn_new(1:Nout,Nout+1:Nn)/Yn_new(Nout+1:Nn,Nout+1:Nn)*Yn_new(Nout+1:Nn,1:Nout);
    end
end


%% 6 vector fitting
if flag_sol == 1
    Rdc_fit = Rself;
    Ldc_fit = Lself;
    Rfit = [];
    Lfit = [];
elseif flag_sol == 2
    %     Nf_fit = 50;
    %     Nc = size(p_sta_L, 1);
    %     frq_fit = logspace(0,log10(frq_cut),Nf_fit);
    %     Rsfit_tmp = zeros(Nc,Nf_fit);
    %     Lsfit_tmp = zeros(Nc,Nf_fit);
    %     for ik = 1:Nc
    %         Rsfit_tmp(ik,:) = interp1(frq,Rself(ik,:), frq_fit);
    %         Lsfit_tmp(ik,:) = interp1(frq,Lself(ik,:), frq_fit);
    %     end
    %
    %     [Rdc_fit,Ldc_fit,Rfit,Lfit] = main_vectfit_z(Rsfit_tmp,Lsfit_tmp, ...
    %         frq_fit, Nfit, Lmtx);
    
    [Rdc_fit,Ldc_fit,Rfit,Lfit] = main_vectfit_z(Rself,Lself, ...
        frq, Nfit, Lmtx);
    
    %     Rfit(16:end,:)=0;
    % Lfit(16:end,:)=0;
    % Rdc_fit(16:end,:)=Rself(16:end,20);
    % Ldc_fit(16:end,:)=Lself(16:end,30);
    
    [Rmtx, Lmtx] = update_vectfit_to_main(Rmtx, Lmtx, Rdc_fit,Ldc_fit);
    
elseif flag_sol == 3
    
    Nfit = min(15, size(Yp,3)-2);
    Np = size(Yp,1);
    Nvf = 201;
    frq_vf = logspace(0, log10(max(frq)), Nvf);
    Yp_vf = zeros(Np, Np, Nvf);
    for ik = 1:Np
        for ig = 1:Np
            Yreal = interp1(frq, real(squeeze(Yp(ik,ig,:))), frq_vf);
            Yimag = interp1(frq, imag(squeeze(Yp(ik,ig,:))), frq_vf);
            Yreal(Nvf) = real(Yp(ik,ig,Nfrq));
            Yimag(Nvf) = imag(Yp(ik,ig,Nfrq));
            Yp_vf(ik,ig,:) = Yreal + 1j*Yimag;
        end
    end
    [dfit3,efit3,rfit3,pfit3,Yfit3] = main_vectfit_y_mtx3(Yp_vf,frq_vf, Nfit, 0);
    
else
    Nc = size(p_sta_L,1);
    Rfit = zeros(Nc,12);
    Lfit = zeros(Nc,12);
end


%% 7. Model Order Reduction
if flag_mor == 1
    if flag_sol ~= 3
       
        if isempty(nod_sr)
            disp('No source node is set. Please check.');
        elseif isempty(nod_gnd)
            disp('No ground node is set. Please check.');
        else
            Nc = size(Lmtx,1);
            mor_ord = ceil(Nc/3);
%             mor_ord = 2;
            fmor = 500e3;
             
            Lmtx = msr_tree(Rmtx,Lmtx, nd_start,nd_end,nd_Pnew, nod_sr,nod_gnd, fmor, mor_ord);
        end
    end
    
end
%% 8 save the data in matlab form
% save_data_air(Rmtx, Lmtx, Pmtx, Rself, Lself, Lg, Rg, Rgself, Lgself, ...
%     Rdc_fit, Ldc_fit, Rfit, Lfit,  ...
%     pt_start, pt_end, dv, re, shape, dim1, dim2, len, R_pul, sig, mur, ...
%     grp_id, grp_type, pt_2d, ...
%     bran_name, nod_start, nod_end, bran_out, nod_out, ...
%     soil_sig, gnd_off, frq_cut,Nfrq, Nfit, dl, fpath, fname);
nod_name = nd_Pnew;
save_data_air(Rmtx,Lmtx,Cmtx, Rself,Lself, Rg,Lg,Pg, Rgself, Lgself, ...
    Rdc_fit, Ldc_fit, Rfit, Lfit,  ...
    p_sta_L, p_end_L, dv_L, re_L, shpid_L, d1_L,d2_L, l_L, R_L, sig_L,mur_L,epr_L, ...
    g_id_L,g_type_L,p_2d_L, ...
    bname, nd_start, nd_end, nod_name, nod_sr, nod_gnd, bran_out, nod_out, ...
    soil_sig, gnd_off, frq,frq_cut,Nfrq, Nfit, dl, fpath, fname);


%% 9 writing spice netlist
if flag_sr == 0
    if flag_sol == 3
        
    else
        %         Rfit = 0*Rfit;    Lfit=0*Lfit;
        spice_subckt_self_vf(Rmtx,Lmtx,Cmtx, Rfit,Lfit, bname,nd_Pnew, ...
            nd_start, nd_end, bran_out, nod_out, fpath, fname);
    end
    
    %     spice_air_wct(Rmtx,Lmtx,Cmtx, Rfit,Lfit, bname,nd_Pnew, ...
    %         nd_start, nd_end, bran_out, nod_out, fpath, fname);
    
else
    
    %     Imax=100e3;
    %     tau1=0.454;
    %     tau2=143;
    %     n=10;
    %
    %     Nt=1000;
    %     dt=1e-6;
    %     Tmax = Nt*dt;
    %
    %     [i_sr, tus] = sr_heidler(Imax, tau1, tau2, n, Tmax*1e6, dt*1e6, 0);
    %     ts = tus*1e-6;
    
    
    % total source = source1 + source2
    % us
    Tmax = 50;
    dt = 1e-1;
    
    %% source 1
    sr_amp=13.1e3;
    sr_tau1 = 0.22;
    sr_tau2 = 88;
    n = 2;
    cof1 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));
    [ist1] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);
    ist1 = ist1*cof1;
    
    %% source 2
    sr_amp=8.7e3;
    sr_tau1 = 0.21;
    sr_tau2 = 61;
    n = 2;
    cof2 = exp(-(sr_tau1/sr_tau2)*(n*sr_tau2/sr_tau1)^(1/n));
    [ist2, tus, ts] = sr_heidler(sr_amp,sr_tau1,sr_tau2,n,Tmax,dt);
    ist2 = cof2*ist2;
    
    %% total source
    i_sr = (ist1 + ist2);
    i_sr = 22.7e3/max(i_sr)*i_sr;
    % i_sr = -i_sr;
    
    
    pt_hit = [145 282];
    Ns_ch = 2000;
    h_ch = 2e3;
    flag_type = 2;
    %[Uind,Er_T,Ez_T] = sr_induced_v_num(pt_hit,h_ch,Ns_ch, p_sta_L,p_end_L, i_sr, ts,flag_type);
    
    %[Uind,Er_rs,Ez_rs] = sr_induced_v_anal(pt_hit,h_ch, p_sta_L,p_end_L, i_sr,tui);
    [Urind,Uzind,Er_T,Ez_T] = sr_induced_e_num_lossy(pt_hit,h_ch,Ns_ch, ...
        p_sta_L,p_end_L,pt_Pnew, i_sr,ts,soil_sig,soil_epr,flag_type);
    
    Nur = size(p_sta_L,1);
    Urname = char(Nur,16);
    for ik = 1:Nur
        Urname(ik,1:16) = ['U_IND_',num2str(ik,'%.10d')];
    end
    Nuz = size(pt_Pnew,1);
    Uzname = char(Nuz,16);
    for ik = 1:Nuz
        Uzname(ik,1:16) = ['U_IND_',num2str(ik+Nur,'%.10d')];
    end
    %     Cmtx= Cmtx*0;
    spice_air_self_vf_vi(Rmtx,Lmtx,Cmtx, Rfit,Lfit, bname,nd_Pnew, ...
        nd_start,nd_end, bran_out,nod_out, ts,Urind,Uzind, Urname,Uzname, fpath, fname)
end



end



