function [Rmtx, Lmtx2, Cmtx2,Rg, Lg,Pg, Rgself, Lgself] = ground_cmplx_plane(Rmtx, Lmtx, Cmtx,pt_start, pt_end, dv, re, len,...
    sig_soil, frq, offset,ver)

%  Function:       para_ground_imag
%  Description:    Calculate imag effect matrix of all conductors using
%                  filament model.
%                  For horizontal conductors, complex imag under f0 is
% %                  adopted.
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


u0=4*pi*1e-7;
epr_soil=10;
ep0=8.85e-12;
sigma_soil=0.001;
r=re;
sigma_wire=5.9e7;
offset=0.1;


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

%% 2. use filament model for perfect imag parameter calculation
% calculate inductance
for ik = 1:Nc
    % calculate inductance using filament model
    Lg_scalar(:,ik) =  cal_L_fila(pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start_img, pt_end_img, dv_img, len_img, re_img);

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

Lg = -(Lg_vert+Lg_horiz);

for ik = 1:Nf
    Lgself(:,ik) = diag(Lg);
end


solution_flag=2;  % 1 for single frequency; 2 for vector fitting
a00=size(pt_start,1);




Lg=zeros(Nc,Nc);
Rg=zeros(Nc,Nc);
for ja=1:Nc
    Rg(ja,ja)=0;
    Lg(ja,ja)=0;
end

[Rmtx, Lmtx, Cmtx,Rg, Lg,Pg, Rgself, Lgself] = ground_pec(Rmtx, Lmtx, Cmtx,pt_start, pt_end, dv, ...
    re, len, frq, offset,ver) ;

Lmtx=Lmtx-Lg;
% Pmtx=inv(Cmtx)+inv(Cg);
% Cmtx2=inv(Pmtx);

% Cmtx2=Cmtx;
    
La=zeros(Nc,Nc);
for ia=1:Nc
    for ib=1:Nc
        La(ia,ib)=abs(sum(dv(ia,:).*[0 0 1]));
    end
end
Lg=(Lg.*La);


Lmtx2 = Lmtx +Lg;
    Pmtx=inv(Cmtx)-Pg;
    Cmtx2=inv(Pmtx);


