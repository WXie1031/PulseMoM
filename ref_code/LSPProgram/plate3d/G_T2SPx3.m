% Tape_2_Sour_Px3  working horse for inte(1/R, x, y, z) -- (x')
%                  Level 1: z'
%                  Level 2: x'
%                  Level 3: x and y
% xf=[y1 y2]  yf=[y1 y2]  2-col vectors
% XS YS                   matrix
% dz                      matrix
% Dec. 2015
%
function f=G_T2SPx3(xf,yf,XS,YS,dz)
[nf ns]=size(XS);    % = size of ys

XF1=repmat(xf(1:nf,1),1,ns);
XF2=repmat(xf(1:nf,2),1,ns);

YF1=repmat(yf(1:nf,1),1,ns);
YF2=repmat(yf(1:nf,2),1,ns);

% f11  
dx=XF1-XS;   
dy=YF1-YS;   
R=sqrt(dx.*dx+dy.*dy+dz.*dz); 

tp1=0.5*(dx.*dx-dz.*dz).*log(dy+R);
tp2=dx.*dy.*log(dx+R);
tp3=-0.5*dy.*R;

Itmp=(dx==0);                           % find out dx=0 elements
tp2(Itmp)=0;                            % set these elements to be 0
if dz==0
    tp1(Itmp)=0;                        % set 0 for elements dx=dz=0
    tp4=0;
else
    tp4=-dx.*dz.*atan(dx.*dy./dz./R);    
end
f11=tp1+tp2+tp3+tp4;

% 12
dx=XF1-XS;   
dy=YF2-YS;   
R=sqrt(dx.*dx+dy.*dy+dz.*dz); 

tp1=0.5*(dx.*dx-dz.*dz).*log(dy+R);
tp2=dx.*dy.*log(dx+R);
tp3=-0.5*dy.*R;

Itmp=(dx==0);                           % find out dx=0 elements
tp2(Itmp)=0;                            % set these elements to be 0
if dz==0
    tp1(Itmp)=0;                        % set 0 for elements dx=dz=0
    tp4=0;
else
    tp4=-dx.*dz.*atan(dx.*dy./dz./R);    
end
f12=tp1+tp2+tp3+tp4;

% 21
dx=XF2-XS;   
dy=YF1-YS;   
R=sqrt(dx.*dx+dy.*dy+dz.*dz); 

tp1=0.5*(dx.*dx-dz.*dz).*log(dy+R);
tp2=dx.*dy.*log(dx+R);
tp3=-0.5*dy.*R;

Itmp=(dx==0);                           % find out dx=0 elements
tp2(Itmp)=0;                            % set these elements to be 0
if dz==0
    tp1(Itmp)=0;                        % set 0 for elements dx=dz=0
    tp4=0;
else
    tp4=-dx.*dz.*atan(dx.*dy./dz./R);    
end
f21=tp1+tp2+tp3+tp4;

% 22
dx=XF2-XS;   
dy=YF2-YS;   
R=sqrt(dx.*dx+dy.*dy+dz.*dz); 

tp1=0.5*(dx.*dx-dz.*dz).*log(dy+R);
tp2=dx.*dy.*log(dx+R);
tp3=-0.5*dy.*R;

Itmp=(dx==0);                           % find out dx=0 elements
tp2(Itmp)=0;                            % set these elements to be 0
if dz==0
    tp1(Itmp)=0;                        % set 0 for elements dx=dz=0
    tp4=0;
else
    tp4=-dx.*dz.*atan(dx.*dy./dz./R);    
end
f22=tp1+tp2+tp3+tp4;

f(1:nf,1:ns)=-(f11-f12-f21+f22);
end

