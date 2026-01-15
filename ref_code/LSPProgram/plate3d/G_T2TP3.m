% TAPE_2_TAPE_P3    working horse for inte(1/R, x, y,  x' y') -- (x' y')
%                   parallel tapes
% dx                matrix
% yf=[y1 y2]  ys=[y'1 y'2]   2-col vectors
% dz                         single value
% Dec. 2015
%
function f=G_T2TP3(xf,xs,dy,dz)
[nf ns]=size(dy);

XF1=repmat(xf(1:nf,1),1,ns);
XF2=repmat(xf(1:nf,2),1,ns);
XS1=repmat(xs(1:ns,1)',nf,1);
XS2=repmat(xs(1:ns,2)',nf,1);

dy2=dy.*dy;
dz2=dz.*dz;

% 1st level integration (y y')
% 11  
dx=XF1-XS1;   
dx2=dx.*dx;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp3=-(R.*R-3*dz2).*R/6;
if dz==0  
    tp1(dx==0)=0;
    tp2(dy==0)=0;
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f11=tp1+tp2+tp3+tp4;

% 12
dx=XF1-XS2;   
dx2=dx.*dx;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp3=-(R.*R-3*dz2).*R/6;
if dz==0  
    tp1(dx==0)=0;
    tp2(dy==0)=0;
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f12=tp1+tp2+tp3+tp4;

% 21
dx=XF2-XS1;   
dx2=dx.*dx;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp3=-(R.*R-3*dz2).*R/6;
if dz==0  
    tp1(dx==0)=0;
    tp2(dy==0)=0;
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f21=tp1+tp2+tp3+tp4;

% 22
dx=XF2-XS2;   
dx2=dx.*dx;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp3=-(R.*R-3*dz2).*R/6;
if dz==0  
    tp1(dx==0)=0;
    tp2(dy==0)=0;
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f22=tp1+tp2+tp3+tp4;

f=f22-f21-f12+f11;
end

