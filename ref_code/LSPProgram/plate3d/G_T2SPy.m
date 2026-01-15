% TAPE_2_Sour_Py  working horse for inte(1/R, x, y, z) -- (x, 'y')
%                  Level 1: z and x' or y'
%                  Level 2: x and y
% xf yf  xs ys     2-col vectors
% dz=zf-zs         single value
% Dec. 2015
%
function f=G_T2SPy(xf,yf,zf,xs,ys,zs)
[nf m]=size(xf);
[ns m]=size(xs);    % = size of ys
[nzf m]=size(zf);   % integration variable

XS=xs(1:ns,1);
YS1=repmat(ys(1:ns,1)',nf,1);
YS2=repmat(ys(1:ns,2)',nf,1);
ZS=zs(1:ns,1);

% 1st level integration (z and x')
for ik=1:nzf
    dz=repmat(zf(ik),nf,ns)-repmat(ZS(1:ns,1)',nf,1);  

% f1  
f1=G_P2LPya(xf,yf,XS,YS1,dz);

% f2
f2=G_P2LPya(xf,yf,XS,YS2,dz);

f(1:nf,1:ns,ik)=f2-f1;
end
end
