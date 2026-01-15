function [Rmtx, Lmtx] = para_main_fila(pt_start, pt_end, dv, re, len)
%  Function:       para_main_fila
%  Description:    Calculate L and P of all conductors using
%                  filament model. Cable group which calculated using
%                  meshing method with update the matrix outside this
%                  function.
%
%  Calls:          cal_L_filament
%
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  Rin_pul   --  resistance of conductors (N*1) (ohm/m)
%                  Lin_pul   --  internal L of conductors (N*1) (H/m)
%                  len       --  length of conductors (N*1)
%                  Ndat_3D   --  num. of conductors (N*1)
%  Output:         Rmtx --  R matrix
%                  Lmtx --  L matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-12-13

Nc = size(pt_start,1);
Rmtx = sparse(zeros(Nc, Nc));
Lmtx = sparse(zeros(Nc, Nc));


%% use DC model to calculate resistance
%Rmtx = sparse(diag());

%% use filament model to calculate mutual inductance
for ik = 1:Nc

    % calculate inductance using filament model
    Lmtx(1:ik,ik) = cal_L_fila ...
        (pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start(1:ik,1:3), pt_end(1:ik,1:3), dv(1:ik,1:3), len(1:ik), re(1:ik));

end

Ls = diag(Lmtx);
Lmtx = (Lmtx+Lmtx'-diag(Ls));



end


