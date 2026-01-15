function Lcol = cal_L_plate_2d(pt_mid_s, dv_s, d1_s,d2_s, pt_mid_f, dv_f, d1_f,d2_f)
%  Function:       cal_L_plate_2d
%  Description:    Calculate L matrix of all tapes using exact tape intergral.
%                  source tape should be single and field tape can be many.
%  Calls:          int_tape_p_2d -- exact formula for tapes in the same
%                  plate
%
%  Input:          pt_mid    --  mid point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  dim1      --  x demension of the tape (N*1)
%                  dim2      --  y demension of the tape (N*1)
%  Output:         Lcol --  L a colume vector
%  Others:         1 source line and multi- field linesis are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-04-29

% ps should be the single conductor, pf should be vector
mu0 = 4*pi*1e-7;

Nf = size(pt_mid_f,1);
Lcol = zeros(Nf,1);

if Nf > 0
    
    ERR = 1e-2;
    
    % Ws is the cross section area
    if sum(abs(dv_s-[1 0 0]))<=ERR   % x cell  (x y)
        Ws = d2_s;
        Wf = d2_f;
    elseif sum(abs(dv_s-[0 1 0]))<=ERR  % y cell  (x y)
        Ws = d1_s;
        Wf = d1_f;
    end
    
    dvtmp = repmat(dv_s,Nf,1);
    idp = false(Nf,1);
    idv = false(Nf,1);
    ida = false(Nf,1);
    idp(1:Nf) = sum(abs(cross(dvtmp, dv_f,2)),2) <= ERR ;  % parallel
    idv(1:Nf) = dot(dvtmp,dv_f,2) <= ERR ;  % perpendicular
    ida(1:Nf) = ~(idp+idv);            % arbitrary angle
    
    % for parallel conductors
    if sum(idp) > 0
        Lcol(idp) = mu0/(4*pi)/Ws*int_tape_fila_p_h2d(pt_mid_s, dv_s, d1_s,d2_s, ...
            pt_mid_f(idp,1:3), dv_f(idp,1:3), d1_f(idp),d2_f(idp));
        
%         Lcol(idp) = mu0/(4*pi)./(Ws.*Wf(idp)).*int_tape_p_h2d(pt_mid_s, d1_s,d2_s, ...
%             pt_mid_f(idp,1:3), d1_f(idp),d2_f(idp));
    end
    % for arbitrary conductors
    if sum(ida) > 0
        % need to update in the furture
        
        % Lcol(ida) = mu0/(4*pi)*int_tape_a_2d();
        
    end
    
end


end

