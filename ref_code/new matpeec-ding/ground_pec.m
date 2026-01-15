function [Rmtx, Lmtx2, Cmtx2,Rg, Lg,Pg, Rgself, Lgself] = ground_pec(Rmtx, Lmtx, Cmtx,pt_start, pt_end, dv, ...
    re, len, frq, offset,ver) 
%  Function:       para_ground_pec
%  Description:    Calculate imag effect matrix of all conductors using
%                  filament model. Perfect ground effect version.
%                  The project angles to the horizontal and vertical plane
%                  are calculated using direction vector.
%                  
%  Calls:          cal_L_filament
%      
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  len       --  length of conductors (N*1)
%                  Ndat_3D   --  num. of conductors (N*1)
%  Output:         Rg   --  imag R matrix
%                  Lg   --  imag L matrix
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2014-12-13

    
if nargin < 7
    offset = 0;
end

Nf = length(frq);
Nc = size(pt_start,1);
Lg_scalar= zeros(Nc, Nc);
Rgself = zeros(Nc,Nf);
Lgself = zeros(Nc,Nf);

%% 1. forming perfec image data
pt_start_img = pt_start;
pt_end_img = pt_end;
pt_start_img(:,3) = -(pt_start(:,3)+offset*2);
pt_end_img(:,3) = -(pt_end(:,3)+offset*2);

dv_img = dv;
dv_img(:,3) = -dv(:,3);
re_img = re;
len_img = len;


    pt_mid = (pt_start+pt_end)/2;
    p_sta_P= [pt_start;pt_mid];
    p_end_P= [pt_mid;pt_end];
    dv_P   = [dv;dv];
    len_P  = [len;len]/2;
    re_P   = [re;re]; 
    
    p_sta_P_img = p_sta_P;
p_end_P_img = p_end_P;
    p_sta_P_img(:,3) = -(p_sta_P(:,3)+offset*2);
    p_end_P_img(:,3) = -(p_end_P(:,3)+offset*2);
    dv_P_img = dv_P;
    re_P_img = re_P;
    len_P_img = len_P;

%% 2. use filament model for perfect imag parameter calculation
% calculate inductance
for ik = 1:Nc
    % calculate inductance using filament model
    Lg_scalar(:,ik) =  cal_L_fila(pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start_img, pt_end_img, dv_img, len_img, re_img);
%     Pg_scalar(:,ik) =  cal_P_fila(pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
%         pt_start_img, pt_end_img, dv_img, len_img, re_img);
%     Pg_scalar(1:ik,ik) = cal_P_fila(p_sta_P(ik,1:3), p_end_P(ik,1:3), dv_P(ik,1:3), len_P(ik), re_P(ik),p_sta_P_img, p_end_P_img, dv_P_img, len_P_img, re_P_img);

end

for ik = 1:2*Nc
    % calculate inductance using filament model
%     Lg_scalar(:,ik) =  cal_L_fila(pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
%         pt_start_img, pt_end_img, dv_img, len_img, re_img);
%     Pg_scalar(:,ik) =  cal_P_fila(pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
%         pt_start_img, pt_end_img, dv_img, len_img, re_img);
    Pg_scalar(:,ik) = cal_P_fila(p_sta_P(ik,1:3), p_end_P(ik,1:3), dv_P(ik,1:3), len_P(ik), re_P(ik),p_sta_P_img, p_end_P_img, dv_P_img, len_P_img, re_P_img);

end

%% 3. project the matrix to the herizonal and vertical plane

% project to the vertical plane
Lg_vert = dv(:,3)*(-dv(:,3))'.*Lg_scalar;

% project to the herizonal plane
IXS = pt_end(:,1) - pt_start(:,1);            % X-increment of source line
IYS = pt_end(:,2) - pt_start(:,2);            % Y-increment of source line

IXO = IXS;            % X-increment of field line
IYO = IYS;            % Y-increment of field line

KRS = sqrt(dv(:,1).*dv(:,1) + dv(:,2).*dv(:,2));
KRO = KRS;

RS = max(sqrt(IXS.*IXS+IYS.*IYS), re);
RO = max(sqrt(IXO.*IXO+IYO.*IYO), re_img);
KP = (IXO*IXS'+IYO*IYS')./(RO*RS');

Lg_horiz = KP.*(KRO*KRS').*Lg_scalar;

%% 4. output the result
Rg = zeros(Nc, Nc);
% Lg_horiz=zeros(Nc, Nc);
Lg = (Lg_vert-Lg_horiz);


% Lg=abs(Lg_scalar);
Pg=abs(Pg_scalar);
% Cg=inv(Pg);
% Cg=1./3e8./3e8./Lg;
for ik = 1:Nf
    Lgself(:,ik) = diag(Lg);
end


Lmtx2 = Lmtx + Lg;
% Lg_scalar2 =  cal_L_fila(pt_start(1,1:3), pt_end(1,1:3), dv(1,1:3), len(1), re(1), ...
% pt_start_img(187,:), pt_end_img(187,:), dv_img(187,:), len_img(187,:), re_img(187,:))
% Lg_scalar3 =  cal_L_fila(pt_start(187,1:3), pt_end(187,1:3), dv(187,1:3), len(187), re(187), ...
% pt_start_img(1,:), pt_end_img(1,:), dv_img(1,:), len_img(1,:), re_img(1,:))


    Pmtx=inv(Cmtx)-Pg;
    Cmtx2=inv(Pmtx);
%
% end


