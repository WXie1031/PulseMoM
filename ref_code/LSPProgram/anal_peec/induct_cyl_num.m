function int = induct_cyl_num(r, len)
%  Function:       induct_bar_Grover
%  Description:    Calculate DC inductance of the cylinder conductor using
%                  numerical method. The thickness of clyinder is 0.
%
%  Calls:          
%
%  Input:          r       --  radius of conductors (N*1) (m)
%                  len     --  length of conductors (N*1)
%  Output:         Ls      --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-16

% level of numerical integral
Nint = 9;

Ns = length(r);

[T, A] = gauss_int_coef(Nint);
a = 2*pi/2*ones(1,Nint) + 2*pi/2*T;


% change to vectors for speeding
a = ones(Ns,1)*a;
A = ones(Ns,1)*A;

len = len*ones(Ns,Nint);
l2 = len.*len;
r = r*ones(1,Nint);
d = (2*r.*sin(a/2));
d2 = d.^2;

I2 = ( len.*log((len+sqrt(l2+d2))./d) - sqrt(l2+d2) + d );
        
I2int = 2*pi/2.*sum(A.*I2,2);
        
%int = 1e-7*4*pi*r2.*( I1 + I2int );
int =1e-7/pi.*( I2int );



end


