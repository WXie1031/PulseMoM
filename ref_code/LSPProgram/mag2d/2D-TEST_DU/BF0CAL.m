% BF0CAL  Calculating the B-field from Is, Jc, Mx and My
%
%
function [Bxc Byc Bxm Bym]=BF0CAL(B0c,P0c,J0c,M0x,M0y)
% (0) initilization
% (0a) obsewrvation points
[nb nn]=size(B0c); mb=1:nb;
[np nn]=size(P0c); mp=1:np;
    
Sxy(mp,1:2)=P0c(mp,1:2:3);
Sxy(mp,3:4)=P0c(mp,2:2:4);
r0=1e-6;

        [Pa,Pb,Pc,Pd,Pe]=COEF_2D_CELL_M0(Sxy,B0c,r0);
        
        Bx=-Pb;
        Pxx=Pd; 
        Pxy=Pc;
        
        By=Pa;
        Pyx=Pc; 
        Pyy=-Pd;

Bxc=Bx*J0c;
Byc=By*J0c;

Bxm=Pxx*M0x+Pxy*M0y;
Bym=Pyx*M0x+Pyy*M0y;
end

           
            
