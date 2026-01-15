function [Rg,Lg, Zg, Rsg,Lsg] = zg_cmplx_plane_b1(pt_start,pt_end, ...
    dv, re, len, sig_soil, epr_soil, frq)
%  Function:       zg_cmplx_plane_b1
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


w = frq*2*pi;
Nf = length(frq);
Nc = size(pt_start,1);

Rsg = zeros(Nc,Nf);
Lsg = zeros(Nc,Nf);
Rg = zeros(Nc,Nc,Nf);
Lg = zeros(Nc,Nc,Nf);
Zg = zeros(Nc,Nc,Nf);

Lg_pec = zeros(Nc, Nc);

rg = sqrt( 1j*w*mu0.*(sig_soil + 1j*w*ep_soil) );

% complex skin depth corresponding to Sunde ground return impedance
cplx_depth = 1./rg;  


%% 1. Calculate mutual matrix Rg, Lg with 10kHz complex depth
%% 1.1 vertical part using perfec image (mutual matrix)
pt_start_img = pt_start;
pt_end_img = pt_end;
pt_start_img(:,3) = pt_start(:,3);
pt_end_img(:,3) = pt_end(:,3);

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
Lg_pec = dv(:,3)*(-dv(:,3))'.*Lg_pec;


%% get the angle for projection to the herizonal plane
IXS = pt_end(:,1) - pt_start(:,1);            % X-increment of source line
IYS = pt_end(:,2) - pt_start(:,2);            % Y-increment of source line

IXO = IXS;            % X-increment of field line
IYO = IYS;            % Y-increment of field line

KRS = sqrt(dv(:,1).*dv(:,1) + dv(:,2).*dv(:,2));
KRO = KRS;

RS = max(sqrt(IXS.*IXS+IYS.*IYS), re);
RO = max(sqrt(IXO.*IXO+IYO.*IYO), re_img);
KP = (IXO*IXS'+IYO*IYS')./(RO*RS');

COSP = KP.*(KRO*KRS');


%% 2. Calculate self matrix Rgself,Lgself at various frequencies
for ik = 1:Nf
    pt_start_img = pt_start;
    pt_end_img = pt_end;
    % In Du's formulation, 2x complex plane depth is used. However, 2x is
    % wrong according to the defination
    pt_start_img(:,3) = -pt_start(:,3) - 2*cplx_depth(ik);
    pt_end_img(:,3) = -pt_end(:,3) - 2*cplx_depth(ik);
    
    Lgp_tmp = zeros(Nc,Nc);
    for ig = 1:Nc
        % calculate inductance using filament model
        Lgp_tmp(ig,ig:end) = cal_L_fila( pt_start(ig,1:3), pt_end(ig,1:3), dv(ig,1:3), len(ig), re(ig), ...
            pt_start_img(ig:Nc,1:3), pt_end_img(ig:Nc,1:3), dv_img(ig:Nc,1:3), len_img(ig:Nc), re_img(ig:Nc));
    end
    
    Lgp_tmp = COSP.*Lgp_tmp;
    Lgp_tmp = Lgp_tmp'+Lgp_tmp-diag(diag(Lgp_tmp));
    
    Rg(:,:,ik) = imag(Lgp_tmp)*w(ik);
    Lg(:,:,ik) = (Lg_pec+real(Lgp_tmp));
    Zg(:,:,ik) = Rg(:,:,ik)+1j*w(ik)*Lg(:,:,ik);

    Rsg(:,ik) = diag(Rg(:,:,ik));
    Lsg(:,ik) = diag(Lg(:,:,ik));
    
end


end


