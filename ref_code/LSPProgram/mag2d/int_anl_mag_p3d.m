function [Dxz,Dyz,Txx,Txy,Tyy] = int_anl_mag_p3d(a1, a2, r1, r2, r0, b0, l, Nint1,Nint2)
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
%                  Nint1 --  order of the guass integration of angle(usually n == 5 or 6)
%                  Nint2 --  order of the guass integration of radius(usually n == 5 or 6)
%  Output:         intD,intT  --  integral result

Ns = length(b0);


% dS should be multiplied in the last step.
% integral (s s' l l') -> (s l l') * ds'
% dS = (a2-a1).*(r2.*r2-r1.*r1)/2;


[TT1, AA1] = gauss_int_coef(Nint1);
[TT2, AA2] = gauss_int_coef(Nint2);

a = (a2+a1)/2*ones(1,Nint1) + (a2-a1)/2*TT1;
r = (r2+r1)/2*ones(1,Nint2) + (r2-r1)/2*TT2;


% change to vectors for speeding

r0 = r0*ones(1,Nint1);
r0d = r0.*r0;
a = ones(Ns,1)*a;
r = ones(Ns,1)*r;
l = l*ones(1,Nint1);
l2 = ones(Ns,1)*(l.*l);

AA1 = ones(Ns,1)*AA1;
AA2 = ones(Ns,1)*AA2;

b0 = b0*ones(1,Nint1);
% ang = b0-a;
cosd = cos(a-b0);
% sind = sin(a-b0);
cosa = cos(a);
cosb = cos(b0);
sina = sin(a);
sinb = sin(b0);

finDxz = zeros(Ns,Nint2);
finDyz = zeros(Ns,Nint2);
finTxx = zeros(Ns,Nint2);
finTyy = zeros(Ns,Nint2);
finTxy = zeros(Ns,Nint2);

for k = 1:Nint2
    
    rk = r(:,k)*ones(1,Nint1);
    % xn = x-x0,  yn=y-y0
    xn = rk.*cosa-r0.*cosb;
    yn = rk.*sina-r0.*sinb;
    Rxy2 = rk.*rk + r0d - 2*rk.*r0.*cosd;
    Rxy = sqrt(Rxy2);
    Rxyz = sqrt(Rxy2+l2);
    
    % matrix D: -y/R^3 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % I1 = 2*yn./x2ay2.^0.5.*rk;
    % I2 = -2*yn.*(x2ay2+l2).^0.5./x2ay2.*rk;
    I1sub = 1./Rxy;
    I2sub = -sqrt(Rxy2+l2)./Rxy2;
    
    faD1 = 2*yn .* (I1sub+I2sub).*rk ;
    
    % matrix D: x/R^3 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % I3 = -2*xn./x2ay2.^0.5.*rk;
    % I4 = 2*xn.*(x2ay2+l2).^0.5./x2ay2.*rk;
    I3sub = -1./Rxy;
    I4sub = Rxyz./Rxy2;
    faD2 = 2*xn .* (I3sub+I4sub).*rk;
    
    % matrix T: 3x^2/R^5 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % T1 = -2*xn.*xn.*rk./(x2ay2.^1.5);
    % T2 = 2*xn.*xn.*rk.*(2*l2+x2ay2)./(x2ay2.^2)./sqrt(l2+x2ay2);
    T1sub = -1./(Rxy2.^1.5);
    T2sub = (2*l2+Rxy2)./(Rxy2.^2)./Rxyz;
    
    faT1 = 2*xn.*xn .* (T1sub+T2sub) .*rk ;
    
    % matrix T: 3y^2/R^5 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % T3 = -2*yn.*yn.*rk./(x2ay2.^1.5);
    % T4 = 2*yn.*yn.*rk.*(2*l2+x2ay2)./(x2ay2.^2)./sqrt(l2+x2ay2);
    T3sub = -1./(Rxy2.^1.5);
    T4sub = (2*l2+Rxy2)./(Rxy2.^2)./Rxyz;
    
    faT2 = 2*yn.*yn .* (T3sub+T4sub) .*rk ;
    
    % matrix T: 1/R^3 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    T5 = -2*rk./Rxy+2*rk.*Rxyz./Rxy2;
    
    faT3 = T5;
    
    % matrix T: 3xy/R^5 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    % T6 = -2*xn.*yn.*rk./x2ay2.^1.5;
    % T7 = 2*xn.*yn.*rk.*(2*l2+x2ay2)./x2ay2.^2./sqrt(l2+x2ay2);
    T6sub = -1./Rxy2.^1.5;
    T7sub = (2*l2+Rxy2)./Rxy2.^2./Rxyz;
    
    faT4 = 2*xn.*yn .* (T6sub+T7sub) .* rk;
    
    
    % summation of numerical integal - level 1
    finDxz(:,k) = (a2-a1)/2.* sum(AA1.*(faD1),2);
    finDyz(:,k) = (a2-a1)/2.* sum(AA1.*(faD2),2);
    
    finTxx(:,k) = (a2-a1)/2.* sum(AA1.*(faT1-faT3),2);
    finTxy(:,k) = (a2-a1)/2.* sum(AA1.*(faT4),2);
    finTyy(:,k) = (a2-a1)/2.* sum(AA1.*(faT2-faT3),2);
    
end

% summation of numerical integal - level 2
Dxz = (r2-r1)/2.*sum(AA2.*finDxz,2);
Dyz = (r2-r1)/2.*sum(AA2.*finDyz,2);

Txx = (r2-r1)/2.*sum(AA2.*(finTxx),2);
Txy = (r2-r1)/2.*sum(AA2.*(finTxy),2);
Tyy = (r2-r1)/2.*sum(AA2.*(finTyy),2);



end

