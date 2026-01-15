function int = dgf_tail_F_sub(p, dz, D)
%  Function:       dgf_tail_F_sub
%  Description:    calculate the F tail term in DGF.
%                  kz and rr must have the same demension.
%  Calls:          
%
%  Input:          p   --  rho in x-y plane
%                  dz  --  input distance in z direction
%                  D   --  is the distance in 3D spatial
%
%  Output:         int  --  result
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-05-31

if nargin < 3
    D = sqrt(dz.^2+p.^2);
end

int = dz ./ (D.^3);



end

