function [Rg,Lg,Pg, Rgself,Lgself] = ground_pec(p_sta_L,p_end_L,dv_L,re_L,l_L, ...
    p_sta_P,p_end_P,dv_P,re_P,l_P, frq, offset, flag_p) 
%  Function:       para_ground_pec
%  Description:    Calculate imag effect matrix of all conductors using
%                  filament model. Perfect ground effect version.
%                  The project angles to the horizontal and vertical plane
%                  are calculated using direction vector.
%                  
%  Calls:          cal_L_fila
%                  cal_P_fila
%      
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  len       --  length of conductors (N*1)
%                  Ndat_3D   --  num. of conductors (N*1)
%  Output:         Rg   --  imag R matrix
%                  Lg   --  imag L matrix
%                  Pg   --  imag P matrix
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2014-12-13
%  Update:         add pec image of P matrix
%                  2018-06-04


if nargin < 7
    offset = 0;
end

Nf = length(frq);
Nc = size(p_sta_L,1);
Lg_scalar= zeros(Nc, Nc);
Rgself = zeros(Nc,Nf);
Lgself = zeros(Nc,Nf);

%% 1. forming perfec image data
pt_sta_L_img = p_sta_L;
pt_end_L_img = p_end_L;
pt_sta_L_img(:,3) = -(p_sta_L(:,3)+offset*2);
pt_end_L_img(:,3) = -(p_end_L(:,3)+offset*2);

dv_L_img = dv_L;
dv_L_img(:,3) = -dv_L(:,3);
re_L_img = re_L;
l_L_img = l_L;

%% 2. use filament model for perfect imag parameter calculation
% calculate inductance
for ik = 1:Nc
    % calculate inductance using filament model
    Lg_scalar(:,ik) =  cal_L_fila(p_sta_L(ik,1:3), p_end_L(ik,1:3), dv_L(ik,1:3), l_L(ik), re_L(ik), ...
        pt_sta_L_img, pt_end_L_img, dv_L_img, l_L_img, re_L_img);
    
end

%% 4. output the result
Rg = zeros(Nc, Nc);

Lg = -Lg_scalar;

for ik = 1:Nf
    Lgself(:,ik) = diag(Lg);
end

%% 5. calculate the image of the P matrix
if flag_p>0
    Nn = size(p_sta_P,1);
    Pg_scalar= zeros(Nn, Nn);
    
    p_sta_P_img = p_sta_P;
    p_end_P_img = p_end_P;
    p_sta_P_img(:,3) = -(p_sta_P(:,3)+offset*2);
    p_end_P_img(:,3) = -(p_end_P(:,3)+offset*2);
    
    dv_P_img = dv_P;
    dv_P_img(:,3) = -dv_P(:,3);
    re_P_img = re_P;
    l_P_img = l_P;
    
    for ik = 1:Nn
        Pg_scalar(:,ik) = cal_P_fila(p_sta_P(ik,1:3), p_end_P(ik,1:3), dv_P(ik,1:3), l_P(ik), re_P(ik), ...
            p_sta_P_img, p_end_P_img, dv_P_img, l_P_img, re_P_img);
    end
    
    Pg = -abs(Pg_scalar);
else
    Pg = [];
end


end


