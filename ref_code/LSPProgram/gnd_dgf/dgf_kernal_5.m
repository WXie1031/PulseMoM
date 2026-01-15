function int = dgf_kernal_5(zf, zbdy,mm,nn, GRx_xx, kz)
%  Function:       dgf_kernal_3
%  Description:    
%                  ~~~ m>n case ~~~
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
%  Output:         int  --  reflection coefficients (N-1)x1
%
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-03-27



int = exp(-1j.*kz(mm).*(zf-zbdy(mm-1)))./(1+GRx_xx.*exp(-1j*2*kz(mm).*d_lyr(mm))) ...
    .* (1+GRx_xx.*exp(-1j*2*kz(mm).*(zbdy(mm)-zf))) ...
    .* prod( (1+GRx_xx(nn+1:mm-1)).*exp(-1j.*kz(nn+1:mm-1).*d_lyr(nn+1:mm-1)) ...
    ./ (1+GRx_xx(nn+1:mm-1).*exp(-1j*2.*kz(nn+1:mm-1).*d_lyr(nn+1:mm-1))) );

end

