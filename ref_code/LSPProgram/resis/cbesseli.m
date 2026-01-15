function Inu = cbesseli(nu,x)
%  -----   Calculate gamma function with a complex order   ----

if isreal(nu)
    Inu = besseli(nu,x);
else    
    Inu = (1i)^(-nu)*cbesselj(nu,1i*x);
end
Inu =Inu(:);