function Lcol = cal_L_fila(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2)
%  Function:       cal_L_filament
%  Description:    Calculate L matrix of all conductors using filament model.
%                  1 : parallel conductors
%                  2 : perpendicular conductors ( =0 )
%                  3 : arbitrary angle conductors
%  Calls:          int_fila_p -- exact formula
%                  int_line_a -- 1st. exact formula; 2nd. numerical method
%
%  Input:          ps1  --  coordinate of start point of source line
%                  ps2  --  coordinate of end point of source line
%                  dv1  --  direction vector of source line
%                  l1   --  length of source line
%                  r1   --  equivalent radius of the source conductor
%                  pf1  --  coordinate of start point of field line
%                  pf2  --  coordinate of end point of field line
%                  dv2  --  direction vector of field line
%                  l2   --  length of field line
%                  r2   --  equivalent radius of the field conductor
%  Output:         Lcol --  L a colume vector
%  Others:         1 source line and multi- field linesis are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-12-24

% ps should be the single conductor, pf should be vector
mu0 = 4*pi*1e-7;

Nf = size(pf1,1);
Lcol = zeros(Nf,1);

if Nf > 0
    
    ERR = 1e-2;
    
    dvtmp = repmat(dv1,Nf,1);
    idp = false(Nf,1);
    idv = false(Nf,1);
    ida = false(Nf,1);
    idp(1:Nf) = sum(abs(cross(dvtmp, dv2,2)),2) <= ERR ;  % parallel
    idv(1:Nf) = abs(dot(dvtmp,dv2,2)) <= ERR ;  % perpendicular
    ida(1:Nf) = ~(idp+idv);            % arbitrary angle
    
    % for parallel conductors
    if sum(idp) > 0
        Lcol(idp) = mu0/(4*pi)*int_fila_p_2(ps1,ps2,dv1,r1, ...
            pf1(idp,1:3),pf2(idp,1:3),dv2(idp,1:3),r2(idp));
    end
    
    % for arbitrary conductors
    if sum(ida) > 0
        % numerical integration by Du
%         Lcol(ida) = mu0/(4*pi)*int_fila_a_num(ps1,ps2,dv1,l1,r1, ...
%             pf1(ida,1:3),pf2(ida,1:3),dv2(ida,1:3),l2(ida), r2(ida));
        % analytical formula
        Lcol(ida) = mu0/(4*pi)*int_fila_a_anal(ps1,ps2,l1, ...
            pf1(ida,1:3),pf2(ida,1:3),l2(ida));
    end

end


end

