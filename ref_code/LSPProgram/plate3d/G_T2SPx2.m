% TAPE_2_Sour_Px2  working horse for inte(1/R, x, y, z) -- (x')
%                  Level 1: z
%                  Level 2: x'
%                  Level 3: x and y
% xf yf  xs ys     2-col vectors
% zf zs            Gaussian coef.
% Dec. 2015
%
function f=G_T2SPx2(xf,yf,zf,xs,ys,zs)
[nf m]=size(xf);
[ns m]=size(xs);    % = size of ys
[nzf m]=size(zf);   % integration variable

XS1=repmat(xs(1:ns,1)',nf,1);
XS2=repmat(xs(1:ns,2)',nf,1);
YS=repmat(ys(1:ns,1)',nf,1);
ZS=zs(1:ns,1);

% 2nd level integration (y')
for ik=1:nzf
    dz=repmat(zf(ik),nf,ns)-repmat(ZS(1:ns,1)',nf,1);  

% f1  
f1=G_T2SPx3(xf,yf,XS1,YS,dz);

% f2
f2=G_T2SPx3(xf,yf,XS2,YS,dz);

f(1:nf,1:ns,ik)=f2-f1;
end
end
