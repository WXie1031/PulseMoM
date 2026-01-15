function [Rmtx, Lmtx, Pmtx] = para_main_plate_3d(pt_mid_L, dv_L, dim1_L,dim2_L,dim3_L, ...
    p_mid_P, dv_P, dim1_P,dim2_P, p_flag)
%  Function:       para_main_plate
%  Description:    Calculate L and P of all tapes in 2d plate using
%                  analytical model. 
%
%  Calls:          cal_L_plate
%                  cal_P_plate
%
%  Input:          pt_mid    --  mid point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  dim1      --  x demension of the tape (N*1)
%                  dim2      --  y demension of the tape (N*1)
%                  Rin_pul   --  resistance of conductors (N*1) (ohm/m)
%                  p_flag    --  calculate P or not
%  Output:         Rmtx  --  R matrix
%                  Lmtx  --  L matrix
%                  Pmtx  --  P matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-04-29

Nc = size(pt_mid_L,1);
Rmtx = zeros(Nc, Nc);
Lmtx = zeros(Nc, Nc);

if p_flag>0
    Np = size(p_mid_P,1);
    Pmtx = zeros(Np, Np);
else
    Pmtx = [];
end

%% use filament model to calculate mutual inductance
for ik = 1:Nc

    % calculate inductance using filament model
    Lmtx(1:ik,ik) = cal_L_plate_3d ...
        (pt_mid_L(ik,1:3), dv_L(ik,1:3), dim1_L(ik),dim2_L(ik),dim3_L(ik), ...
        pt_mid_L(1:ik,1:3), dv_L(1:ik,1:3), dim1_L(1:ik),dim2_L(1:ik),dim3_L(1:ik));

end

Ls = diag(Lmtx);
Lmtx = Lmtx+Lmtx'-diag(Ls);

%% use filament model to calculate mutual inductance
if p_flag>0
    for ik = 1:Np
        
        Pmtx(1:ik,ik) = cal_P_plate ...
            (p_mid_P(ik,1:3), dv_P(ik,1:3), dim1_P(ik),dim2_P(ik), ...
            p_mid_P(1:ik,1:3), dv_P(1:ik,1:3), dim1_P(1:ik),dim2_P(1:ik));
    end
    
    Ps = diag(Pmtx);
    Pmtx = Pmtx+Pmtx'-diag(Ps);
end



end


