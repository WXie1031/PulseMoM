function [Ls, Lin, Lext] = induct_bar_ac(wid, hig, sig, mur, len, f0)
%  Function:       induct_bar_ac
%  Description:    Calculate AC inductance of the rectangle conductor using
%                  fitting method.
%
%  Calls:          
%
%  Input:          wid       --  width of conductors (N*1) (m)
%                  hig       --  thick of conductors (N*1) (m)
%                  sig       --  conductivity of conductors (N*1) (S/m)
%                  len       --  length of conductors (N*1)
%                  f0        --  frequency
%  Output:         Ls    --  Ls vector (have multiplied length)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-06-16

mu0 = 4*pi*1e-7;


Lr = induct_bar_Grover(wid, hig, len);

%% method-1  external L is rectangle shell
Lext = induct_bar_ext(wid, hig, len);
Lin_dc = Lr-Lext;

%% method-2 fit expression for the dc internal inductance of rectangle
% modi_cof = 2.5;  % modification to internal inductance.
% from - Addition to "DC Internal Inductance for a Conductor of Rectangular Cross Section"
% Lin_dc = induct_bar_in_dc_appr(wid, hig, len);
% Lext = Lr - modi_cof*Lin_dc;

% re = 0.2235*(wid+hig)./0.7788;
% Lin_dc = induct_gmd(gmd_cir_self(re),len) ...
%     -induct_gmd(gmd_cyl_self(re),len);
%% fit expression for the AC inductance of rectangle
% from - Modeling of impedance of rectangular cross-section conductors
cof = 2.8*sqrt(mur);
w0 = 8./(mu0*sqrt(mur)*sig).*((wid+hig)./(wid.*hig)).^2;

% internal inductance has relation with mur
%Lin =  mur * modi_cof .* Lin_dc./sqrt(1+cof*2*pi*f0./w0); 
Lin = mur * Lin_dc./sqrt(1+cof*2*pi*f0./w0); 

Ls = Lext + Lin;


end
    
