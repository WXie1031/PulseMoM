function [Rg,Lg, Rgself,Lgself] = grid_cmplx_plane(pt_start,pt_end, dv, re, len,...
    sig_soil, epr_soil, frq)
%  Function:       grid_cmplx_plane
%  Description:    Calculate ground grid matrix of all conductors using
%                  complex plane model.
%                  For horizontal conductors, complex imag under f0 is
%                  adopted.
%                  For vertical conductors, perfect imag is adopted.
%                  The project angles to the horizontal and vertical plane
%                  are calculated using direction vector.
%
%  Calls:          cal_L_fila
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  len       --  length of conductors (N*1)
%                  Ndat_3D   --  num. of conductors (N*1)
%                  cond_soil --  conductivity of the soil (1*1)
%                  f0        --  analysis frequency of soil (1*1)
%  Output:         Rg   --  imag R matrix
%                  Lg   --  imag L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-06-13


% general data
mu0 = 4*pi*1e-7;
ep0 = 8.85e-12;

epr_soil = max(1, epr_soil);
ep_soil = ep0*epr_soil;

fm = 10e3;
wm = 2*pi*fm;

w = frq*2*pi;
Nf = length(frq);
Nc = size(pt_start,1);

Rgself = zeros(Nc,Nf);
Lgself = zeros(Nc,Nf);

Lg_pec = zeros(Nc, Nc);
Lg_cmplx = complex(zeros(Nc, Nc));
Rg = zeros(Nc, Nc);
Lg = zeros(Nc, Nc);




% complex skin depth corresponding to Sunde ground return impedance
rg = sqrt( 1j*w*mu0.*(sig_soil + 1j*w*ep_soil) );
cplx_depth = 1./rg;  


%% 1. Calculate mutual matrix Rg, Lg with 10kHz complex depth
%% 1.1 vertical part using perfec image (mutual matrix)
pt_start_img = pt_start;
pt_end_img = pt_end;
pt_start_img(:,3) = -pt_start(:,3);
pt_end_img(:,3) = -pt_end(:,3);

dv_img = dv;
dv_img(:,3) = -dv(:,3);
re_img = re;
len_img = len;

% use filament model for perfect imag part (vertial part)
% calculate inductance
for ik = 1:Nc
    % calculate inductance using filament model
    Lg_pec(:,ik) =  cal_L_fila(pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start_img, pt_end_img, dv_img, len_img, re_img);
end
% project to the vertical plane
Lg_pec = -abs(dv(:,3)*(dv_img(:,3))').*Lg_pec;


%% 2.1 horizon part forming complex plane data (mutual matrix)
% get the angle for projection to the herizonal plane
dx = pt_end(:,1) - pt_start(:,1);            % X-increment of source line
dy = pt_end(:,2) - pt_start(:,2);            % Y-increment of source line

dx_img = pt_end_img(:,1) - pt_start_img(:,1);  % X-increment of field line
dy_img = pt_end_img(:,2) - pt_start_img(:,2);  % Y-increment of field line

KRS = sqrt(dv(:,1).^2 + dv(:,2).^2);
KRO = sqrt(dv_img(:,1).^2 + dv_img(:,2).^2);

R_s = max(sqrt(dx.^2 + dy.^2), re);
R_f = max(sqrt(dx_img.^2 + dy_img.^2), re_img);
KP = (dx_img*dx' + dy_img*dy')./(R_f*R_s');

COSP = KP.*(KRO*KRS');


% for mutual matrix, f0=10kHz is chosen.
rg_fm = sqrt( 1j*wm*mu0*(sig_soil + 1j*wm*ep_soil) );
cplx_depth_fm = 1./rg_fm;   % complex skin depth

pt_start_img = pt_start;
pt_end_img = pt_end;
    % In Du's formulation, 2x complex plane depth is used. However, 2x is
    % wrong according to the defination
pt_start_img(:,3) = -pt_start(:,3) - 2*cplx_depth_fm;
pt_end_img(:,3) = -pt_end(:,3) - 2*cplx_depth_fm;
[dv_img, len_img] = line_dv(pt_start_img, pt_end_img);

% use filament model for complex imag part (horizon part)
for ik = 1:Nc
    % calculate inductance using filament model
    dv_sign = ones(Nc,1);
    for ig = 1:Nc
        dv_sign(ig) = sign(dv_img(ig,1:3)*dv(ik,1:3)');
    end
    
    Lg_cmplx(:,ik) = COSP(:,ik).*dv_sign.*cal_L_fila( pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start_img, pt_end_img, dv_img, len_img, re_img);
    
    Rg(:,ik) = dv_sign.*imag(Lg_cmplx(:,ik))*wm;
    Lg(:,ik) = -(Lg_pec(:,ik)+dv_sign.*real(Lg_cmplx(:,ik)));
    
    Rg(ik,ik) = imag(Lg_cmplx(ik,ik))*wm;
    Lg(ik,ik) = -(Lg_pec(ik,ik)+real(Lg_cmplx(ik,ik)));
    
end



%% 2. Calculate self matrix Rgself,Lgself at various frequencies
COSPself = diag(COSP);
Lgv_tmp = diag(Lg_pec);

pt_start_img = pt_start;
pt_end_img = pt_end;
    
for ik = 1:Nf
    
    % In Du's formulation, 2x complex plane depth is used. 
    pt_start_img(:,3) = -pt_start(:,3) - 2*cplx_depth(ik);
    pt_end_img(:,3) = -pt_end(:,3) - 2*cplx_depth(ik);
    %[dv_img, len_img] = line_dv(pt_start_img, pt_end_img);
    
    Lgp_tmp = zeros(Nc,1);
    

    for ig = 1:Nc

        dv_sign = sign(dv_img(ig,1:3)*dv(ig,1:3)');

        
        Lgp_tmp(ig) = dv_sign.*cal_L_fila( pt_start(ig,1:3), pt_end(ig,1:3), dv(ig,1:3), len(ig), re(ig), ...
            pt_start_img(ig,1:3), pt_end_img(ig,1:3), dv_img(ig,1:3), len_img(ig), re_img(ig));
    end
    
    Lgp_tmp = COSPself.*Lgp_tmp;
    
    Rgself(:,ik) = imag(Lgp_tmp)*w(ik);
    %Rgself(:,ik) = imag(Lgp_tmp);
    Lgself(:,ik) = -(Lgv_tmp+real(Lgp_tmp));
    
end


end


