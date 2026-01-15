function Pcol = cal_P_plate(pt_mid_s, dv_s, d1_s,d2_s, pt_mid_f, dv_f, d1_f,d2_f)
%  Function:       cal_P_plate
%  Description:    Calculate L matrix of all tapes using exact tape intergral.
%                  source tape should be single and field tape can be many.
%  Calls:          int_tape_p_3d -- exact formula for paralell tapes
%                  int_tape_v_3d -- exact formula for perpendicular tapes
%
%  Input:          pt_mid    --  mid point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  dim1      --  x demension of the tape (N*1)
%                  dim2      --  y demension of the tape (N*1)
%  Output:         Pcol --  P a colume vector
%  Others:         1 source line and multi- field linesis are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-04-29

% ps should be the single conductor, pf should be vector
ep0 = 8.85*1e-12;

Nf = size(pt_mid_f,1);
Pcol = zeros(Nf,1);

if Nf > 0
    
    err = 1e-3;
    
    dvtmp = repmat(dv_s,Nf,1);
    idp = false(Nf,1);
    idv = false(Nf,1);
    ida = false(Nf,1);
    idp(1:Nf) = sum(abs(cross(dvtmp, dv_f,2)),2) <= err ;  % parallel
    idv(1:Nf) = dot(dvtmp,dv_f,2) <= err ;  % perpendicular
    ida(1:Nf) = ~(idp|idv);            % arbitrary angle
    
    Ss = d1_s.*d2_s;
    
    % for parallel conductors
    if sum(idp) > 0
        Sf = d1_f(idp).*d2_f(idp);
        
        Pcol(idp) = 1/(4*pi*ep0)./(Ss.*Sf) .* int_tape_p_3d(pt_mid_s, dv_s, d1_s,d2_s, ...
            pt_mid_f(idp,1:3), dv_f(idp,1:3), d1_f(idp),d2_f(idp));
    end
    
    if sum(idv) > 0 
        Sf = d1_f(idv).*d2_f(idv);
        
        Pcol(idv) = 1/(4*pi*ep0)./(Ss.*Sf) .* int_tape_v_3d(pt_mid_s, dv_s, d1_s,d2_s, ...
            pt_mid_f(idv,1:3), dv_f(idv,1:3), d1_f(idv),d2_f(idv));
    end
    
    % for arbitrary conductors
    if sum(ida) > 0
        % need to update in the furture
        % Sf = d1_f(ida).*d2_f(ida);
       % Lcol(ida) = mu0/(4*pi)*int_tape_a_2d();
       error('Incline tape-tape is not surpported.');
    end

end


end

