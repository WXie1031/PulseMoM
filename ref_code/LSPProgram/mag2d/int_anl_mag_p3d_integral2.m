function [Dxz,Dyz,Txx,Txy,Tyy] = int_anl_mag_p3d_integral2(a1, a2, r1, r2, r0, b0, l)
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

Ns = length(b0);
Dxz = zeros(Ns,1);
Dyz = zeros(Ns,1);
Txx = zeros(Ns,1);
Txy = zeros(Ns,1);
Tyy = zeros(Ns,1);
for i = 1:Ns

    rp = r0(i);
    beta = b0(i);
    x0 = rp*cos(beta);
    y0 = rp*sin(beta);
     % matrix Dxz: -y/R^3 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    fundxz = @(x,y) (2.*(y-y0)./sqrt((x-x0).^2+(y-y0).^2)-2.*(y-y0).*sqrt((x-x0).^2+(y-y0).^2+l.^2)./((x-x0).^2+(y-y0).^2));
%     polarfundxz = @(theta,r) fundxz(r.*cos(theta),r.*sin(theta)).*r;
    Dxztmp = int_quad_num(fundxz,a1,a2,r1,r2,'Sector',true);
     % matrix Dyz: x/R^3 dv   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    fundyz = @(x,y) (-2.*(x-x0)./sqrt((x-x0).^2+(y-y0).^2)+2.*(x-x0).*sqrt((x-x0).^2+(y-y0).^2+l.^2)./((x-x0).^2+(y-y0).^2));
%     polarfundyz = @(theta,r) fundyz(r.*cos(theta),r.*sin(theta)).*r;
    Dyztmp = int_quad_num(fundyz,a1,a2,r1,r2,'Sector',true);
    % matrix Txx: 3x^2/R^5 - 1/R^3   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    funtxx1 = @(x,y) (-2.*(x-x0).^2./((x-x0).^2+(y-y0).^2).^1.5);
%     polarfuntxx1 = @(theta,r) funtxx1(r.*cos(theta),r.*sin(theta)).*r;
    Txxtmp1 = int_quad_num(funtxx1,a1,a2,r1,r2,'Sector',true);
    
    funtxx2 = @(x,y) (2*(x-x0).^2.*(2*l.^2+(x-x0).^2+(y-y0).^2)./...
        (((x-x0).^2+(y-y0).^2).^2.*sqrt(l.^2+(x-x0).^2+(y-y0).^2)));
%     polarfuntxx2 = @(theta,r) funtxx2(r.*cos(theta),r.*sin(theta)).*r;
    Txxtmp2 = int_quad_num(funtxx2,a1,a2,r1,r2,'Sector',true);
    
    funtxx3 = @(x,y) (2./sqrt((x-x0).^2+(y-y0).^2)-2.*sqrt((x-x0).^2+(y-y0).^2+l^2)./((x-x0).^2+(y-y0).^2));
%     polarfuntxx3 = @(theta,r) funtxx3(r.*cos(theta),r.*sin(theta)).*r;
    Txxtmp3 = int_quad_num(funtxx3,a1,a2,r1,r2,'Sector',true);
    Txxtmp = Txxtmp1 + Txxtmp2 + Txxtmp3;
    % matrix Txy: 3xy/R^5   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    funtxy = @(x,y) (-2.*(x-x0).*(y-y0)./((x-x0).^2+(y-y0).^2).^1.5+2.*(x-x0).*(y-y0).*(2.*l.^2+(x-x0).^2+(y-y0).^2)./...
        ((x-x0).^2+(y-y0).^2).^2./sqrt(l.^2+(x-x0).^2+(y-y0).^2));
%     polarfuntxy = @(theta,r) funtxy(r.*cos(theta),r.*sin(theta)).*r;
    Txytmp = int_quad_num(funtxy,a1,a2,r1,r2,'Sector',true);
    % matrix Tyy: 3y^2/R^5 - 1/R^3   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    funtyy = @(x,y) (-2.*(y-y0).^2./((x-x0).^2+(y-y0).^2).^1.5+2*(y-y0).^2.*(2*l.^2+(x-x0).^2+(y-y0).^2)./...
        ((x-x0).^2+(y-y0).^2).^2./sqrt(l.^2+(x-x0).^2+(y-y0).^2)+2./sqrt((x-x0).^2+(y-y0).^2)-...
        2.*sqrt((x-x0).^2+(y-y0).^2+l^2)./((x-x0).^2+(y-y0).^2));
%     polarfuntyy = @(theta,r) funtyy(r.*cos(theta),r.*sin(theta)).*r;
    Tyytmp = int_quad_num(funtyy,a1,a2,r1,r2,'Sector',true);
    
    Dxz(i,1) = Dxztmp;
    Dyz(i,1) = Dyztmp;
    Txx(i,1) = Txxtmp;
    Txy(i,1) = Txytmp;
    Tyy(i,1) = Tyytmp;
    
end

end
