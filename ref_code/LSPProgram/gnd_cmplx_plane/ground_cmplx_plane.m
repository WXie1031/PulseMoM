function [Rg, Lg, Pg, Rgself, Lgself, Pgself] = ground_cmplx_plane( ...
    p_sta_L, p_end_L, dv_L, re_L, l_L, ...
    p_sta_P, p_end_P, dv_P, re_P, l_P, ...
    sig_soil, frq, offset, flag_p)
%  Function:       ground_cmplx_plane
%  Description:    Calculate imag effect matrix of all conductors using
%                  filament model.
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
%  Date:           2014-12-13

if nargin < 8
    offset = 0;
end

% general data
mu0 = 4*pi*1e-7;

Nf = length(frq);
Nc = size(p_sta_L,1);

% for mutual matrix, f0=10kHz is chosen.
fm = 10e3;

w = frq*2*pi;
wm = 2*pi*fm;

cplx_depth = 1./sqrt(1i*w*mu0*sig_soil);   % complex skin depth
% for mutual matrix, f0=10kHz is chosen.
cplx_depth_fm = 1./sqrt(1i*wm*mu0*sig_soil);   % complex skin depth



Rgself = zeros(Nc,Nf);
Lgself = zeros(Nc,Nf);

Lg_pec = zeros(Nc, Nc);
Lg_cmplx = zeros(Nc, Nc);
Rg = zeros(Nc, Nc);
Lg = zeros(Nc, Nc);
%% 1. Calculate mutual matrix Rg, Lg with 10kHz complex depth
% 1.1 vertical part using perfec image (mutual matrix)
p_sta_L_img = p_sta_L;
p_end_L_img = p_end_L;
p_sta_L_img(:,3) = -(p_sta_L(:,3)+2*offset);
p_end_L_img(:,3) = -(p_end_L(:,3)+2*offset);

dv_L_img = dv_L;
dv_L_img(:,3) = -dv_L(:,3);
re_L_img = re_L;
l_L_img = l_L;

% use filament model for perfect imag part (vertial part)
% calculate inductance
for ik = 1:Nc
    % calculate inductance using filament model
    Lg_pec(:,ik) =  cal_L_fila(p_sta_L(ik,1:3), p_end_L(ik,1:3), dv_L(ik,1:3), l_L(ik), re_L(ik), ...
        p_sta_L_img, p_end_L_img, dv_L_img, l_L_img, re_L_img);
end
% project to the vertical plane
Lg_pec = -abs(dv_L(:,3)*(dv_L(:,3))').*Lg_pec;


% 1.2 horizon part forming complex plane data (mutual matrix)
p_sta_L_img = complex(p_sta_L);
p_end_L_img = complex(p_end_L);
% In Deri's formulation, 2x complex plane depth is used.
p_sta_L_img(:,3) = -(p_sta_L(:,3)+2*offset)-2*cplx_depth_fm;
p_end_L_img(:,3) = -(p_end_L(:,3)+2*offset)-2*cplx_depth_fm;


% get the angle for projection to the herizonal plane
dx = p_end_L(:,1) - p_sta_L(:,1);            % X-increment of source line
dy = p_end_L(:,2) - p_sta_L(:,2);            % Y-increment of source line
dx_img = p_end_L_img(:,1) - p_sta_L_img(:,1);  % X-increment of field line
dy_img = p_end_L_img(:,2) - p_sta_L_img(:,2);  % Y-increment of field line

KRS = sqrt(dv_L(:,1).^2 + dv_L(:,2).^2);
KRO = sqrt(dv_L_img(:,1).^2 + dv_L_img(:,2).^2);
R_s = max(sqrt(dx.^2 + dy.^2), re_L);
R_f = max(sqrt(dx_img.^2 + dy_img.^2), re_L_img);
KP = (dx_img*dx' + dy_img*dy')./(R_f*R_s');

COSP = KP.*(KRO*KRS');

% use filament model for complex imag part (horizon part)
for ik = 1:Nc
    % calculate inductance using filament model
    dv_sign = ones(Nc,1);
    for ig = 1:Nc
        dv_sign(ig) = sign(dv_img(ig,1:3)*dv(ik,1:3)');
    end
    
    Lg_cmplx(:,ik) = COSP(:,ik).*dv_sign.*cal_L_fila( p_sta_L(ik,1:3), p_end_L(ik,1:3), dv_L(ik,1:3), l_L(ik), re_L(ik), ...
        p_sta_L_img, p_end_L_img, dv_L_img, l_L_img, re_L_img);
    
    Rg(:,ik) = dv_sign.*imag(Lg_cmplx(:,ik))*wm;
    Lg(:,ik) = -(Lg_pec(:,ik)+dv_sign.*real(Lg_cmplx(:,ik)));
    
    Rg(ik,ik) = imag(Lg_cmplx(ik,ik))*wm;
    Lg(ik,ik) = -(Lg_pec(ik,ik)+real(Lg_cmplx(ik,ik)));
end



%% 2. Calculate self matrix Rgself,Lgself at various frequencies
COSPself = diag(COSP);
Lgv_tmp = diag(Lg_pec);

for ik = 1:Nf
    p_sta_L_img = p_sta_L;
    p_end_L_img = p_end_L;
    % In Deri's formulation, 2x complex plane depth is used.
    p_sta_L_img(:,3) = -(p_sta_L(:,3)+2*offset) - 2*cplx_depth(ik);
    p_end_L_img(:,3) = -(p_end_L(:,3)+2*offset) - 2*cplx_depth(ik);
    
    Lgp_tmp = zeros(Nc,1);
    for ig = 1:Nc
        % calculate inductance using filament model
        dv_sign = sign(dv_img(ig,1:3)*dv(ig,1:3)');
        
        Lgp_tmp(ig) = dv_sign.*cal_L_fila( p_sta_L(ig,1:3), p_end_L(ig,1:3), dv_L(ig,1:3), l_L(ig), re_L(ig), ...
            p_sta_L_img(ig,1:3), p_end_L_img(ig,1:3), dv_L_img(ig,1:3), l_L_img(ig), re_L_img(ig));
    end
    
    Lgp_tmp = COSPself.*Lgp_tmp;
    
    Rgself(:,ik) = imag(Lgp_tmp)*w(ik);
    %Rgself(:,ik) = imag(Lgp_tmp);
    Lgself(:,ik) = -(Lgv_tmp+real(Lgp_tmp));
    
end


%% 3. Calculate P matrix Rg with 10kHz complex depth
if flag_p>0
    Nn = size(p_sta_P,1);
    
    Pgself = zeros(Nn,Nf);
    Ggself = zeros(Nn,Nf);
    Pg_pec = zeros(Nn, Nn);
    Pg_cmplx = zeros(Nn, Nn);
    
    % 3.1 vertical part using perfec image (mutual matrix)
    p_sta_P_img = p_sta_P;
    p_end_P_img = p_end_P;
    p_sta_P_img(:,3) = -(p_sta_P(:,3)+2*offset);
    p_end_P_img(:,3) = -(p_end_P(:,3)+2*offset);
    
    dv_P_img = dv_P;
    dv_P_img(:,3) = -dv_P(:,3);
    re_P_img = re_P;
    l_P_img = l_P;
    
    % use filament model for perfect imag part (vertial part)
    % calculate inductance
    for ik = 1:Nn
        % calculate inductance using filament model
        Pg_pec(:,ik) =  cal_P_fila(p_sta_P(ik,1:3), p_end_P(ik,1:3), dv_P(ik,1:3), l_P(ik), re_P(ik), ...
            p_sta_P_img, p_end_P_img, dv_P_img, l_P_img, re_P_img);
    end
    % project to the vertical plane
    Pg_pec = -abs(dv_P(:,3)*dv_P(:,3)').*Pg_pec;
    
    
    % 3.2 horizon part forming complex plane data (mutual matrix)
    p_sta_P_img = p_sta_P;
    p_end_P_img = p_end_P;
    % In Deri's formulation, 2x complex plane depth is used.
    p_sta_P_img(:,3) = -(p_sta_P(:,3)+2*offset)-2*cplx_depth_fm;
    p_end_P_img(:,3) = -(p_end_P(:,3)+2*offset)-2*cplx_depth_fm;
    
    % use filament model for complex imag part (horizon part)
    for ik = 1:Nn
        % calculate inductance using filament model
        Pg_cmplx(:,ik) = cal_P_fila(p_sta_P(ik,1:3), p_end_P(ik,1:3), dv_P(ik,1:3), l_P(ik), re_P(ik), ...
            p_sta_P_img, p_end_P_img, dv_P_img, l_P_img, re_P_img);
    end
    
    % get the angle for projection to the herizonal plane
    dx = p_end_P(:,1) - p_sta_P(:,1);            % X-increment of source line
    dy = p_end_P(:,2) - p_sta_P(:,2);            % Y-increment of source line
    
    dx_img = p_end_P_img(:,1) - p_sta_P_img(:,1);  % X-increment of field line
    dy_img = p_end_P_img(:,2) - p_sta_P_img(:,2);  % Y-increment of field line
    
    KRS = sqrt(dv_P(:,1).^2 + dv_P(:,2).^2);
    KRO = sqrt(dv_P_img(:,1).^2 + dv_P_img(:,2).^2);
    
    R_s = max(sqrt(dx.^2 + dy.^2), re_P);
    R_f = max(sqrt(dx_img.^2 + dy_img.^2), re_P_img);
    KP = (dx_img*dx' + dy_img*dy')./(R_f*R_s');
    
    COSP = KP.*(KRO*KRS');
    
    Pg_cmplx = COSP.*Pg_cmplx;
    
    % 3.3 output the mutual part
    Gg = imag(Pg_cmplx)*wm;
    Pg = -(Pg_pec+real(Pg_cmplx));
    
    
    %% 4. Calculate self matrix Ggself,Pgself at various frequencies
    COSPself = diag(COSP);
    Pgv_tmp = diag(Pg_pec);
    
    for ik = 1:Nf
        p_sta_P_img = p_sta_P;
        p_end_P_img = p_end_P;
        % In Deri's formulation, 2x complex plane depth is used.
        p_sta_P_img(:,3) = -(p_sta_P(:,3)+2*offset) - 2*cplx_depth(ik);
        p_end_P_img(:,3) = -(p_end_P(:,3)+2*offset) - 2*cplx_depth(ik);
        
        Pgp_tmp = zeros(Nc,1);
        for ig = 1:Nc
            % calculate inductance using filament model
            Pgp_tmp(ig) = cal_P_fila(p_sta_P(ig,1:3), p_end_P(ig,1:3), dv_P(ig,1:3), l_P(ig), re_P(ig), ...
                p_sta_P_img(ig,1:3), p_end_P_img(ig,1:3), dv_P_img(ig,1:3), l_P_img(ig), re_P_img(ig));
        end
        
        Pgp_tmp = COSPself.*Pgp_tmp;
        
        Ggself(:,ik) = imag(Pgp_tmp)*w(ik);
        Pgself(:,ik) = -(Pgv_tmp+real(Pgp_tmp));
    end
    
else
    Pg = [];
    Pgself = [];
end



end


