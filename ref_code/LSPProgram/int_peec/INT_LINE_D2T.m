%INT_LINE_D2T   Neuuman formula for perpendicular wire-to-wire (s-to-o)
%               subscript "1" = observation (H), "2" = source (V)
%
%           Inegration of (1/sqrt((U-u)^2+(v-V)^2+(w-W^2) over dU & dv 
%           U and v = wire directions
%
% Q=[Q1(:);Q2(:)]   vectors: coef. for the GF from GF_TABLE
% DU(:,1)=[u1 u2]   vectors: lower and upper coodinates of u for interval
% DR(:,1)=[R1 R2]   vectors: relative dis. from obj. pt to two sour. pts
% r0(:)             radius of all segments in source wires
% num               [N ns no], N = # of pts on obs. segment for integratino 
% A0                coef. of Gauss–Legendre quadrature
%
% updated on April 2, 2013
function out=INT_LINE_D2T(U1a,U1b,V1,W1,U2,V2a,V2b,W2)
a0=1e-6;        % to avoid log(0) for negative uij
a2=a0*a0;

no=length(U1a);
ns=length(V2a);

u10=repmat(U1a,1,ns)-repmat(U2',no,1);
u20=repmat(U1b,1,ns)-repmat(U2',no,1);
v10=repmat(V1,1,ns)-repmat(V2a',no,1);
v20=repmat(V1,1,ns)-repmat(V2b',no,1);
dw=repmat(W1,1,ns)-repmat(W2',no,1);
dw0=dw+a0;

u1s=u10.*u10;
u2s=u20.*u20;
v1s=v10.*v10;
v2s=v20.*v20;
dws=dw.*dw;

R11=sqrt(u1s+v1s+dws+a2);
R22=sqrt(u2s+v2s+dws+a2);
R12=sqrt(u1s+v2s+dws+a2);
R21=sqrt(u2s+v1s+dws+a2);
% R11=max(sqrt(u1s+v1s+dws),a2);
% R22=max(sqrt(u2s+v2s+dws),a2);
% R12=max(sqrt(u1s+v2s+dws),a2);
% R21=max(sqrt(u2s+v1s+dws),a2);
clear a2 dws

I11=v10.*log(u10+R11)+u10.*log(v10+R11)+...
    -dw.*atan(u10.*v10./(dw0.*R11));
I22=v20.*log(u20+R22)+u20.*log(v20+R22)+...
    -dw.*atan(u20.*v20./(dw0.*R22));
I12=v20.*log(u10+R12)+u10.*log(v20+R12)+...
    -dw.*atan(u10.*v20./(dw0.*R12));
I21=v10.*log(u20+R21)+u20.*log(v10+R21)+...
    -dw.*atan(u20.*v10./(dw0.*R21));
out=-(I11+I22-I12-I21);         % add neg. sign
end