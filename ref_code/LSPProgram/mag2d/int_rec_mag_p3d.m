function [Dxz,Dyz,Txx,Txy,Tyy] = int_rec_mag_p3d(x1, x2, y1, y2, x0, y0, l, Nint1, Nint2)
%  Function:       int_anl_fila_p3d
%  Description:    calculate annulus segment-point integral
%                  (for 3D parallel aligned conductors)
%  Calls:
%  Input:          a1   --  start angle of the annulus segment
%                  a2   --  end angle of the annulus segment
%                  r1   --  inner radius of the annulus segment
%                  r2   --  outer radius of the annulus segment
%                  r0   --  distance between the point and the centre of
%                           the circle(which annulus segment belong to)
%                  b0   --  angle between the point and the centre of the
%                           circle (which annulus segment belong to)
%                  l    --  length of the conductor

%  Output:         intD,intT  --  integral result
[TT1, AA1] = gauss_int_coef(Nint1);
[TT2, AA2] = gauss_int_coef(Nint2);


Ns = length(x0);
x1 = x1*ones(Ns,1)-x0;
x2 = x2*ones(Ns,1)-x0;
y1 = y1*ones(Ns,1)-y0;
y2 = y2*ones(Ns,1)-y0;

xg = (x2+x1)/2*ones(1,Nint1) + (x2-x1)/2*TT1;
yg = (y2+y1)/2*ones(1,Nint2) + (y2-y1)/2*TT2;

xg = ones(Ns,1)*xg;
yg = ones(Ns,1)*yg;
lg = l*ones(1,Nint1);
AA1 = ones(Ns,1)*AA1;
AA2 = ones(Ns,1)*AA2;

finR3 = zeros(Ns,Nint2);
finTxx = zeros(Ns,Nint2);
finTyy = zeros(Ns,Nint2);
finTxy = zeros(Ns,Nint2);

R2d11 = sqrt(x1.^2+y1.^2);
R2d12 = sqrt(x1.^2+y2.^2);
R2d21 = sqrt(x2.^2+y1.^2);
R2d22 = sqrt(x2.^2+y2.^2);

R3d11 = sqrt(x1.^2+y1.^2+l.^2);
R3d12 = sqrt(x1.^2+y2.^2+l.^2);
R3d21 = sqrt(x2.^2+y1.^2+l.^2);
R3d22 = sqrt(x2.^2+y2.^2+l.^2);

%% matrix Dxz: -y/R^3 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
A1 = x2.*R2d22-x1.*R2d12+y2.^2.*log((x2+R2d22)./(x1+R2d12));
A2 = -x2.*(R2d21)+x1.*R2d11-y1.^2.*log((x2+R2d21)./(x1+R2d11));
B1 = x2.*R3d22+log(2.*x2+2.*R3d22).*R3d22.^2-x2.^2.*log(x2+R3d22)-(y2.^2)/2;
B2 = -x2.*R3d21-log(2.*x2+2.*R3d21).*R3d21.^2+x2.^2.*log(x2+R3d21)+(y1.^2)/2;
B3 = -x1.*R3d12-log(2.*x1+2.*R3d12).*R3d12.^2+x1.^2.*log(x1+R3d12)+(y2.^2)/2;
B4 = x1.*R3d11+log(2.*x1+2.*R3d11).*R3d11.^2-x1.^2.*log(x1+R3d11)-(y1.^2)/2;
C1 = 2.*l.*y2.*atan(l.*x2./y2./R3d22)-2.*l.*x2.*atanh(R3d22./l)-2.*l.^2.*atanh(R3d22./x2);
C2 = -2.*l.*y1.*atan(l.*x2./y1./R3d21)+2.*l.*x2.*atanh(R3d21./l)+2.*l.^2.*atanh(R3d21./x2);
C3 = -2.*l.*y2.*atan(l.*x1./y2./R3d12)+2.*l.*x1.*atanh(R3d12./l)+2.*l.^2.*atanh(R3d12./x1);
C4 = 2.*l.*y1.*atan(l.*x1./y1./R3d11)-2.*l.*x1.*atanh(R3d11./l)-2.*l.^2.*atanh(R3d11./x1);

Dxz = A1+A2-B1-B2-B3-B4-C1-C2-C3-C4;


%% matrix Dxz: x/R^3 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
D1 = y1.*R2d21-y2.*R2d22+x2.^2.*log((y1+R2d21)./(y2+R2d22));
D2 = -y1.*R2d11+y2.*R2d12-x1.^2.*log((y1+R2d11)./(y2+R2d12));
E1 = y2.*R3d22+log(2.*y2+2.*R3d22).*R3d22.^2-y2.^2.*log(y2+R3d22)-(x2.^2)/2;
E2 = -y2.*R3d12-log(2.*y2+2.*R3d12).*R3d12.^2+y2.^2.*log(y2+R3d12)+(x1.^2)/2;
E3 = -y1.*R3d21-log(2.*y1+2.*R3d21).*R3d21.^2+y1.^2.*log(y1+R3d21)+(x2.^2)/2;
E4 = y1.*R3d11+log(2.*y1+2.*R3d11).*R3d11.^2-y1.^2.*log(y1+R3d11)-(x1.^2)/2;
F1 = 2.*l.*x2.*atan(l.*y2./x2./R3d22)-2.*l.*y2.*atanh(R3d22./l)-2.*l.^2.*atanh(R3d22./y2);
F2 = -2.*l.*x1.*atan(l.*y2./x1./R3d12)+2.*l.*y2.*atanh(R3d12./l)+2.*l.^2.*atanh(R3d12./y2);
F3 = -2.*l.*x2.*atan(l.*y1./x2./R3d21)+2.*l.*y1.*atanh(R3d21./l)+2.*l.^2.*atanh(R3d21./y1);
F4 = 2.*l.*x1.*atan(l.*y1./x1./R3d11)-2.*l.*y1.*atanh(R3d11./l)-2.*l.^2.*atanh(R3d11./y1);
Dyz =  D1+D2+E1+E2+E3+E4+F1+F2+F3+F4;
%% 1/R^3
P1 = 2.*x2.*log((y2+R2d22)./(y1+R2d21));
P2 = 2.*x1.*log((y2+R2d12)./(y1+R2d11));
P3 = 2.*y2.*log((x2+R2d22)./(x1+R2d12));
P4 = 2.*y1.*log((x2+R2d21)./(x1+R2d11));
P = P1-P2+P3-P4;
Q = zeros(Ns,1);
% for i1 = 1:Ns
%     Qfun = @(x,y)  (2.*sqrt(x.^2+y.^2+l(i1).^2)./(x.^2+y.^2));
%     Q(i1) = TwoD(Qfun,x1(i1),x2(i1),y1(i1),y2(i1));
% end
% % Gaussian Integral
for k1 = 1:Nint2
    yk1 = yg(:,k1)*ones(1,Nint1);
    fR3 = 2.*sqrt(xg.^2+yk1.^2+lg.^2)./(xg.^2+yk1.^2);
    finR3(:,k1) = (x2-x1)/2.* sum(AA1.*(fR3),2);
end
Q = (y2-y1)/2.*sum(AA2.*finR3,2);

R3 = Q-P;
%% Txx = 3x^2/R^5-1/R^3
% -2x^2/(x^2+y^2)^1.5
G1 = 2.*x2.*log(y2+R2d22)-2.*x2.*log(y2+R2d22)-2.*y2.*log(x2+R2d22)+2*y2;
G2 = -2.*x2.*log(y1+R2d21)+2.*x2.*log(y1+R2d21)+2.*y1.*log(x2+R2d21)-2*y1;
G3 = -2.*x1.*log(y2+R2d12)+2.*x1.*log(y2+R2d12)+2.*y2.*log(x1+R2d12)-2*y2;
G4 = 2.*x1.*log(y1+R2d11)-2.*x1.*log(y1+R2d11)-2.*y1.*log(x1+R2d11)+2*y1;

% H = zeros(Ns,1);
% for i2 = 1:Ns
%     Hfun = @(x,y)  (2.*x.^2.*(2.*l(i2).^2+x.^2+y.^2)./(x.^2+y.^2).^2./sqrt(l(i2).^2+x.^2+y.^2));
%     H(i2) = TwoD(Hfun,x1(i2),x2(i2),y1(i2),y2(i2));
% end
% gaussian integral
for k2 = 1:Nint2
    yk2 = yg(:,k2)*ones(1,Nint1);
    fH = (2.*xg.^2.*(2.*l.^2+xg.^2+yk2.^2)./(xg.^2+yk2.^2).^2./sqrt(l.^2+xg.^2+yk2.^2));
    finTxx(:,k2) = (x2-x1)/2.* sum(AA1.*(fH),2);
end
H = (y2-y1)/2.*sum(AA2.*finTxx,2);

Txx = G1+G2+G3+G4+H-R3;
%% Tyy = 3y^2/R^5-1/R^3
% -2y^2/(x^2+y^2)^1.5
J1 = 2.*y2.*log(x2+R2d22)-2.*y2.*log(x2+R2d22)-2.*x2.*log(y2+R2d22)+2.*x2;
J2 = -2.*y2.*log(x1+R2d12)+2.*y2.*log(x1+R2d12)+2.*x1.*log(y2+R2d12)-2.*x1;
J3 = -2.*y1.*log(x2+R2d21)+2.*y1.*log(x2+R2d21)+2.*x2.*log(y1+R2d21)-2.*x2;
J4 = 2.*y1.*log(x1+R2d11)-2.*y1.*log(x1+R2d11)-2.*x1.*log(y1+R2d11)+2.*x1;

K = zeros(Ns,1);
% for i3 = 1:Ns
%     Kfun = @(x,y)  (2.*y.^2.*(2.*l(i3).^2+x.^2+y.^2)./(x.^2+y.^2).^2./sqrt(l(i3).^2+x.^2+y.^2));
%     K(i1) = TwoD(Kfun,x1(i3),x2(i3),y1(i3),y2(i3));
% end
% Gaussian integral
for k3 = 1:Nint2
    yk3 = yg(:,k3)*ones(1,Nint1);
    fK =  (2.*yk3.^2.*(2.*l.^2+xg.^2+yk3.^2)./(xg.^2+yk3.^2).^2./sqrt(l.^2+xg.^2+yk3.^2));
    finTyy(:,k3) = (x2-x1)/2.* sum(AA1.*(fK),2);
end
K = (y2-y1)/2.*sum(AA2.*finTyy,2);

Tyy = J1+J2+J3+J4+K-R3;

%% Txy = 3y^2/R^5
% -2xy/(x^2+y^2)^1.5
M1 = 2.*R2d22-2.*R2d21-2.*R2d12-2.*R2d11;
N = zeros(Ns,1);
% for i4 = 1:Ns
%     Nfun = @(x,y)  (2.*y.*x.*(2.*l(i4).^2+x.^2+y.^2)./(x.^2+y.^2).^2./sqrt(l(i4).^2+x.^2+y.^2));
%     N(i4) = TwoD(Nfun,x1(i4),x2(i4),y1(i4),y2(i4));
% end

% Gaussian integral
for k4 = 1:Nint2
    yk4 = yg(:,k4)*ones(1,Nint1);
    fN =  (2.*yk4.*xg.*(2.*l.^2+xg.^2+yk4.^2)./(xg.^2+yk4.^2).^2./sqrt(l.^2+xg.^2+yk4.^2));
    finTxy(:,k4) = (x2-x1)/2.* sum(AA1.*(fN),2);
end
N = (y2-y1)/2.*sum(AA2.*finTxy,2);
Txy = M1 + N;

end