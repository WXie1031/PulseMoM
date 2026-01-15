function INT0 = int_fila_v_num(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2, Nint)
% INT_LINE_D2A double-line integration along direction u and u'(free space)
%              (1) Close-form Formula for the integral of a function (1st)
%                        1
%                 F(z)= -------------------------------- over [u1 u2]
%                        sqrt(x^2+y^2+z^2)
%              (2) numerical integration using 1/2/3/5-point method(2nd)
%
% DU(:,1)=[u1 u2]   vectors: lower and upper coodinates of u for interval
% DR(:,1)=[R1 R2]   vectors: relative dis. from obj. pt to two sour. pts
% r0(:)             radius of all segments in source wires
% num               [N ns no], N = # of pts on obs. segment for integratino
% A0                coef. of Gauss–Legendre quadrature
%
% updated on April 2, 2013
%
% Warning:  1. nss must be 1 -- source line can not be segmented in this
%           version.
%           2. for almost aligned but offset lines, the result is not
%           accurate.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%  Function:       int_line_a
%  Description:    Calculate intergral between two arbitrary lines.
%                  Modified based on Prof. Du's program.
%
%  Calls:          gauss_int_coef
%
%  Input:          ps1  --  coordinate of start point of source line
%                  ps2  --  coordinate of end point of source line
%                  dv1  --  direction vector of source line
%                  l1   --  length of source line
%                  pf1  --  coordinate of start point of field line
%                  pf2  --  coordinate of end point of field line
%                  dv2  --  direction vector of field line
%                  l2   --  length of field line
%  Output:         int  --  integral result
%  Others:         1 source line and multi- field linesis are supported
%  Author:         Du Ya-ping
%  Email :         hc.chen@live.com
%  Date:           Modified on 2014-12-13

ELIM = 1e-7;                  % limit of R-U (l=300,h=0.01)

Ns = size(ps1,1);
Nf = size(pf1,1);

% order of numerical integration
if nargin < 11
    Nint = 6;
end


Nsall = 2*Ns;
Nfall = Nf*Nint;

%% 1. Guass numerical integration cofficients
[T0, A0] = gauss_int_coef(Nint);  % get Gauss coef.

%% 2. segment the source and field line
% 2.a mesh source line (the source line does not segmented)
Ps = zeros(Ns*2,3);
Ds = zeros(Ns*2,3);

Ps((1:Ns),:) = ps1;     % coor of field lines after meshing
Ps((1:Ns)+Ns,:) = ps2;
Ds((1:Ns),:) = dv1;
Ds((1:Ns)+Ns,:) = dv1;

% 2.b mesh field line
Po = zeros(Nfall,3);
slo = zeros(Nf,1);
for ik=1:Nint
    slo(1:Nf) = 0.5*l2*(T0(ik)+1);
    %ind = (1:Nf)+(ik-1)*Nf;
    Po((1:Nf)+(ik-1)*Nf,1) = pf1(:,1) + slo.*dv2(:,1);     % coor of field lines after meshing
    Po((1:Nf)+(ik-1)*Nf,2) = pf1(:,2) + slo.*dv2(:,2);
    Po((1:Nf)+(ik-1)*Nf,3) = pf1(:,3) + slo.*dv2(:,3);
end

%% 3. prapare the data for calculation
r1tmp = repmat(r1',Nfall,Nsall);
r2tmp = repmat(r2, Nint, Nsall);
r0 = max(r1tmp,r2tmp);

dx = repmat(Po(:,1),1,Nsall)-repmat(Ps(:,1)',Nfall,1);
dy = repmat(Po(:,2),1,Nsall)-repmat(Ps(:,2)',Nfall,1);
dz = repmat(Po(:,3),1,Nsall)-repmat(Ps(:,3)',Nfall,1);
DR = max(sqrt(dx.*dx+dy.*dy+dz.*dz), r0);

drx = repmat(Ds(:,1)',Nfall,1);
dry = repmat(Ds(:,2)',Nfall,1);
drz = repmat(Ds(:,3)',Nfall,1);
DU = dx.*drx+dy.*dry+dz.*drz;


%% integration calculation
% (1) 1st integral result using the close-form formula (ns*no*N)
INT1 = zeros(Nfall,1);
R1 = zeros(Nfall,1);
R2 = zeros(Nfall,1);
U1 = zeros(Nfall,1);
U2 = zeros(Nfall,1);
tmp = zeros(Nfall,1);
dif = zeros(Nfall,1);


R1(1:Nfall) = DR(1:Nfall, 1:Ns);     % range for all upper s. points
R2(1:Nfall) = DR(1:Nfall, Ns+(1:Ns));
U1(1:Nfall) = DU(1:Nfall, 1:Ns);        % for image under the ground
U2(1:Nfall) = DU(1:Nfall, Ns+(1:Ns));        % for image under the ground
tmp(1:Nfall) = log((R2-U2)./(R1-U1));

dif(1:Nfall) = R1-U1;     % update NaN items in TMP with log(R1/R2)
for ig = 1:Nfall
    if dif(ig) < ELIM
        tmp(ig) = log((R1(ig)+U1(ig))./(R2(ig)+U2(ig)));
    end
end

INT1 = INT1+tmp;

% (2) (ns x no) 2nd integration: Gauss egendre quadrature
INT0 = zeros(Nf,1);
for ik = 1:Nint
    INT0 = INT0 + A0(ik)*INT1( (ik-1)*Nf+(1:Nf),1);
end

if nargin < 11
    id_inf = find(isinf(INT0)|isnan(INT0));
    Ntmp = Nint;
    while ~isempty(id_inf)
        if Ntmp>15
            Ntmp = 1;
        end
        Ntmp = Ntmp+1;
        INT0(id_inf) = int_fila_v_num(ps1,ps2,dv1,l1,r1, ...
            pf1(id_inf,:),pf2(id_inf,:),dv2(id_inf,:),l2(id_inf),r2(id_inf), Ntmp);
        
        id_inf = find(isinf(INT0) |isnan(INT0), 1);
        if Ntmp==Nint
            break;
        end
    end
end

% if ~isempty(id_inf)
%     disp('Inf or NaN exsit in int_v calculation.')
% end

%cosa = dv1(:,1)'*dv2(:,1) + dv1(:,2)'*dv2(:,2) + dv1(:,3)'*dv2(:,3);
%INT0 = INT0.*LL/2.*cosa;

LL = repmat(l2,1,Ns);
INT0 = INT0.*LL/2;


end

