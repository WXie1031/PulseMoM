% BFSCAL    Calculate the contribution of source lines (Bsx Bsy)
%
%
% June 15 2010
% 
function [Bxs Bys]=BFSCAL(Q0,Qs,Icu)
% initilization
[nu nn]=size(Q0);               % no. of obj. points
[ns nn]=size(Qs);               % no. of source lines

for i=1:ns
    dx(1:nu,i)=Q0(1:nu,1)-Qs(i,1);
    dy(1:nu,i)=Q0(1:nu,2)-Qs(i,2);
end

tmp=-dy./(dx.*dx+dy.*dy);
Bxs=tmp*Icu;      

tmp=+dx./(dx.*dx+dy.*dy);
Bys=tmp*Icu; 
end