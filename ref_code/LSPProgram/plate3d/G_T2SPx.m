% TAPE_2_Sour_Px  working horse for inte(1/R, x, y, z) -- (x, 'y')
%                  Level 1: z and x' or y'
%                  Level 2: x and y
% xf yf  xs ys     2-col vectors
% dz=zf-zs         single value
% Dec. 2015
%
function f=G_T2SPx(xf,yf,zf,xs,ys,zs)
[nf m]=size(xf);
[ns m]=size(xs);    % = size of ys
[nzf m]=size(zf);   % integration variable

XS1=repmat(xs(1:ns,1)',nf,1);
XS2=repmat(xs(1:ns,2)',nf,1);
YS=ys(1:ns,1);
ZS=zs(1:ns,1);

% 1st level integration (z and x')
for ik=1:nzf
    dz=repmat(zf(ik),nf,ns)-repmat(ZS(1:ns,1)',nf,1);  

% f1  
f1=G_P2LPxa(xf,yf,XS1,YS,dz);

% f2
f2=G_P2LPxa(xf,yf,XS2,YS,dz);

f(1:nf,1:ns,ik)=f2-f1;
end
end
