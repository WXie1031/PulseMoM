% TAPE_2_TAPE_Pax    working horse for inte(1/R, x, y,  x' y') -- (x' y')
%                   parallel tapes
% dx                matrix
% yf=[y1 y2]  ys=[y'1 y'2]   2-col vectors
% dz                         single value
% Dec. 2015
%
function f=G_T2TPa(dx,yf,ys,dz)
[nf ns]=size(dx);

YF1=repmat(yf(1:nf,1),1,ns);
YF2=repmat(yf(1:nf,2),1,ns);
YS1=repmat(ys(1:ns,1)',nf,1);
YS2=repmat(ys(1:ns,2)',nf,1);

dx2=dx.*dx;
dz2=dz.*dz;

% 1st level integration (y y')
% 11  
dy=YF1-YS1;   
dy2=dy.*dy;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp1(dx==0)=0;
tp2(dy==0)=0;
tp3=-(R.*R-3*dz2).*R;
if dz==0  
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f11=tp1+tp2+tp3+tp4;

% 12
dy=YF1-YS2;   
dy2=dy.*dy;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp1(dx==0)=0;
tp2(dy==0)=0;
tp3=-(R.*R-3*dz2).*R;
if dz==0  
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f12=tp1+tp2+tp3+tp4;

% 21
dy=YF2-YS1;   
dy2=dy.*dy;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp1(dx==0)=0;
tp2(dy==0)=0;
tp3=-(R.*R-3*dz2).*R;
if dz==0  
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f21=tp1+tp2+tp3+tp4;

% 22
dy=YF2-YS1;   
dy2=dy.*dy;
R=sqrt(dx2+dy2+dz2); 
tp1=0.5*(dx2-dz2).*dy.*log(dy+R);
tp2=0.5*(dy2-dz2).*dx.*log(dx+R);
tp1(dx==0)=0;
tp2(dy==0)=0;
tp3=-(R.*R-3*dz2).*R;
if dz==0  
    tp4=0;
else
    tp4=-dx.*dy.*dz.*atan(dx.*dy./dz./R); 
end
f22=tp1+tp2+tp3+tp4;

f=f22-f21-f12+f11;
end

