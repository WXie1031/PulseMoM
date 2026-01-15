function Rgrod = grid_rod_resis_v(Rgrod, pt_start,pt_end, dv, re, len, Rsoil)
%  Function:       grid_rod_resis_v
%  Description:    Calculate R of vertical rod conductors using
%                  proposed model in paper "Modeling of Grounding
%                  Electrodes Under Lgihtning Currents"
%  Calls:
%  Input:          pt_start  --  start point of conductors (N*3) (m)
%                  pt_end    --  end point of conductors (N*3) (m)
%                  dv        --  direction vector of conductors (N*3)
%                  re        --  equivalent radius (N*1)
%                  Rsoil     --  resistance of conductors (N*1) (ohm/m)
%                  len       --  length of conductors (N*1)
%  Output:         Rgrod   --  R matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-13

Nc = size(re,1);

if isempty(Rgrod)
    Rgrod = zeros(Nc,1);
end

z_axis = [0 0 1];
z_axis = ones(Nc,1)*z_axis;
id_v = abs(sum(cross(dv,z_axis),2))<1e-6;

Rgrod(id_v) = Rsoil./(2*pi*len(id_v)) .* ...
    (log(len(id_v)./re(id_v)+sqrt(1+(len(id_v)./re(id_v)).^2)) ...
    + re(id_v)./len(id_v) - sqrt(1+(re(id_v)./len(id_v)).^2) + log(2));

% simplified model
% Rgrod(id_rodv) = Rsoil./(2*pi*len(id_rodv)) .* ...
%     (log(4*len(id_rodv)./re(id_rodv))-1);


end


