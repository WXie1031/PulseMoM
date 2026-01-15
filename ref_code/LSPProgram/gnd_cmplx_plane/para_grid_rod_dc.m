function [Rrod, Crod] = para_grid_rod_dc(pt_start,pt_end, dv, re, len, Rsoil,epr_soil)
%  Function:       main_grid_rod_resis
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

ERR = 1e-3;
ep0 = 8.85*1e-12;

if isempty(pt_start)
    Rrod = [];
    Crod = [];
    return;
end

Nc = size(pt_start,1);
Rrod = zeros(Nc,1);

z_axis = [0 0 1];
ztmp = repmat(z_axis,Nc,1);
idp = false(Nc,1);
idv = false(Nc,1);
ida = false(Nc,1);
idp(1:Nc) = sum(abs(ztmp.*dv),2) <= ERR ;  % parallel
idv(1:Nc) = sum(abs(cross(ztmp, dv,2)),2) <= ERR ; % vertical
ida(1:Nc) = ~(idp+idv);            % arbitrary angle

% for horizontal conductors
if sum(idp) > 0
    Rrod(idp) = grid_rod_resis_h(pt_start(idp,:),pt_end(idp,:), dv(idp,:), re(idp), len(idp), Rsoil);
end
% for vectical conductors
if sum(idv)>0
    Rrod(idv) = grid_rod_resis_v(pt_start(idv,:),pt_end(idv,:), dv(idv,:), re(idv), len(idv), Rsoil);
end
% for arbitrary conductors
if sum(ida) > 0
    Na = sum(ida);
    pt_tmp = (pt_end(ida,:)+pt_start(ida,:))/2;
    
    ltmp1 = sqrt( (pt_end(ida,1)-pt_start(ida,1)).^2 + (pt_end(ida,2) - pt_start(ida,2)).^2 ); 
    dvh = ones(Na,1)*[1 0 0];
    Rtmph = grid_rod_resis_h(pt_tmp,pt_tmp, dvh, re(ida,:), ltmp1, Rsoil);

    ltmp2 = abs(pt_end(ida,3) - pt_start(ida,3));
    dvv = ones(Na,1)*[0 0 1];
    Rtmpv = grid_rod_resis_v(pt_tmp,pt_tmp, dvv, re(ida,:), ltmp2, Rsoil);
    
    Rrod(ida) = Rtmph + Rtmpv;
end


Crod = 1./Rrod * Rsoil * epr_soil*ep0;

% Ctmp1 = 2*pi*ep0*epr_soil*len./ ...
%     ( re./len + log((len+sqrt(len.^2+re.^2))./re) - sqrt(1-(re./len).^2) );

% dep = abs(pt_start(:,3));
% Ctmp2 = 2*pi*ep0*epr_soil*len./ ...
%     ( (2*dep-re)./len + log((len+sqrt(len.^2+(2*dep-re).^2))./(2*dep-re)) - sqrt(1-((2*dep-re)./len).^2) );
% 
% C = Ctmp1+Ctmp2;

end


