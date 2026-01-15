function Rgrod = grid_rod_resis_h(Rgrod, pt_start,pt_end, dv, re, len, Rsoil)
%  Function:       grid_rod_resis_h
%  Description:    Calculate R of horizontal rod conductors using
%                  proposed model in paper "Modeling of Grounding
%                  Electrodes Under Lgihtning Currents"
%  Calls:          
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
%  Date:           2015-09-13

Nc = size(dv,1);

if isempty(Rgrod)
    Rgrod = zeros(Nc,1);
end


z_axis = [0 0 1];
z_axis = ones(Nc,1)*z_axis;
id_h = abs(dot(dv,z_axis,2))<1e-6;

depth = abs(pt_end(:,3));

%cplx_depth = 1./sqrt(1i*wm*mu0*sig_soil);   % complex skin depth

Rgrod(id_h) = Rsoil./(2*pi*len(id_h)) .* ...
    (log(len(id_h)./re(id_h)+sqrt(1+(len(id_h)./re(id_h)).^2)) ...
    + re(id_h)./len(id_h) - sqrt(1+(re(id_h)./len(id_h)).^2) ...
    + log(len(id_h)./(2*depth(id_h))+sqrt(1+(len(id_h)./depth(id_h)/2).^2)) ...
    + 2*depth(id_h)./len(id_h) - sqrt(1+(2*depth(id_h)./len(id_h)).^2) );

%Rgrod(id_h) = Rsoil./(2*pi*len(id_h)) .* log( len(id_h)./re(id_h)+1 );
    

end


