function Pcol = cal_P_fila(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2)
%  Function:       cal_L_filament
%  Description:    Calculate L matrix of all conductors using filament model.
%                  1 : parallel conductors
%                  2 : perpendicular conductors ( =0 )
%                  3 : arbitrary angle conductors
%  Calls:          int_line_p -- exact formula
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
ep0 = 8.85*1e-12;

Nf = size(pf1,1);
Pcol = zeros(Nf,1);

if Nf > 0
    
    ERR = 1e-3;
    
    % calculate the distance between ps2 and pf1
    R = sqrt( sum(((ps1+ps2)/2-(pf1+pf2)/2).^2,2) );
    id_near = ( R <= 5*l1/2 );
    id_far  = ( R >  5*l1/2 );
    
    % for test
%     id_near = 1;
%     id_far = 0;

    dvtmp = repmat(dv1,Nf,1);
    idp = false(Nf,1);
    idv = false(Nf,1);
    ida = false(Nf,1);
    idp(1:Nf) = (sum(abs(cross(dvtmp, dv2,2)),2) <= ERR) & id_near ;  % parallel
    idv(1:Nf) = (abs(dot(dvtmp,dv2,2)) <= ERR) & id_near ;  % perpendicular
    ida(1:Nf) = ~(idp+idv) & id_near;            % arbitrary angle
    
    if sum(id_far) > 0
        Pcol(id_far) = 1/(4*pi*ep0)./(l1*l2(id_far)) .* int_fmm_far(dv1,l1,dv2(id_far,1:3),l2(id_far), R(id_far));
    end
    
    % for parallel conductors
    if sum(idp) > 0
        Nftmp = sum(idp);
        dv_sign = ones(Nftmp,1);
        
        ind = sum( abs(ones(Nftmp,1)*dv1 + dv2(idp,1:3)),2  )<1e-3 ;
        dv_sign(ind) = -1;

        Pcol(idp) = dv_sign.*1/(4*pi*ep0)./(l1*l2(idp)) .* int_fila_p(ps1,ps2,dv1,r1, ...
            pf1(idp,1:3),pf2(idp,1:3),dv2(idp,1:3),r2(idp));
    end
    
    % for vectical conductors
    if sum(idv)>0
%         Pcol(idv) = 1/(4*pi*ep0)./(l1*l2(idv)) .* int_fila_v(ps1,ps2,dv1, ...
%             pf1(idv,1:3),pf2(idv,1:3),dv2(idv,1:3));
%         dv_sign = sign(real(Pcol(idv)));
%         Pcol(idv) = dv_sign.*Pcol(idv);

        Pcol(idv) = 1/(4*pi*ep0)./(l1*l2(idv)) .* int_fila_v_anal(ps1,ps2,l1, ...
            pf1(idv,1:3), pf2(idv,1:3), l2(idv));
    end
    
    % for arbitrary conductors
    if sum(ida) > 0
        % numerical integration by Du
        %cosa = dv1(:,1)'*dv2(ida,1) + dv1(:,2)'*dv2(ida,2) + dv1(:,3)'*dv2(ida,3);
%         Pcol(ida) = 1/(4*pi*ep0)./(l1*l2(ida)) .* int_fila_a_num(ps1,ps2,dv1,l1,r1, ...
%             pf1(ida,1:3),pf2(ida,1:3),dv2(ida,1:3),l2(ida), r2(ida));
        % analytical formula
        Nftmp = sum(ida);
        dv_sign = ones(Nftmp,1);
        dv_sign(1:Nftmp) = sign( sum(ones(Nftmp,1)*dv1.*dv2(ida,:),2) );
        
        Pcol(ida) = dv_sign.*1/(4*pi*ep0)./(l1*l2(ida)) .* int_fila_a_anal_c( ...
            ps1,ps2,l1, pf1(ida,1:3),pf2(ida,1:3),l2(ida));
        %Pcol(ida)  = abs(Pcol(ida));
    end

end

Pcol = abs(Pcol);

end

