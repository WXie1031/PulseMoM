function nlyr = dgf_locate_lyr(z, zbdy)
%  Function:       dgf_kernal_1
%  Description:    
%                  
%                  left medium | L1 | L2 | ... | LM | right medium 
%                    interface 1    2    3     M   M+1
%                  
%                  The left and the right medium is related with the
%                  direction of the waves. If the wave is in the right
%                  direction, 'left medium' is the first layer, and the
%                  'right medium' is the last medium. In this situation,
%                  the relection coefficient of the 'right medium' is 0.
%  Calls:          
%
%  Input:          para_lyr --  parameter of each layer (Nx1)
%                  kz   --  wave number
%
%  Output:         Rcof  --  reflection coefficients (N-1)x1
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-27

Nz = length(z);
Nlyr = length(zbdy)+1;

nlyr = zeros(Nz,1);
for ik = 1:Nz
    nlyr(ik) = Nlyr - sum((z(ik)-zbdy)>0); % index from upper to lower
    %nlyr(ik) = sum((z(ik)-zbdy)>0)+1; % index from lower to upper
end


end

