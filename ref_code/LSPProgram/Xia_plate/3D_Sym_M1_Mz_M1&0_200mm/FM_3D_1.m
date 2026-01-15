% Lx (Jx)
%
% I1=int(f1,x,x',y',z'),   f1=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% I1'=int(f1,y,x,x',y',z'),   f1=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% Oxyz_X, Sxyz_X
%
% updated on 15/02/2011
%
function [I1]=FM_3D_1(Oxyz,Sxyz)   

% Obj are lines, Oxyz(x1,x2,yo,zo),Sxyz(x1,y1,x2,y2,zs)
% Obj are planes, Oxyz(x1,y1,x2,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% OBJ=1,"line"; OBJ=2,"plane"

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);
Io=ones(no,1);
Is=ones(1,ns);

xo1=Oxyz(:,1)*Is;
xo2=Oxyz(:,2)*Is;
yo=Oxyz(:,3)*Is;
zo=Oxyz(:,4)*Is;

xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,3)';
ys1=Io*Sxyz(:,2)';
ys2=Io*Sxyz(:,4)';
zs=Io*Sxyz(:,5)';

x11=xo1-xs1;
x12=xo1-xs2;
x21=xo2-xs1;
x22=xo2-xs2;
y11=yo-ys1;
y12=yo-ys2;
dz=zo-zs;

clear Io Is xo1 xo2 yo zo xs1 xs2 ys1 ys2 zs;

%if OBJ==1                                     % Obj are lines
%    for i=1:ns                             
%        x11(1:no,i)=Oxyz(:,1)-Sxyz(i,1);
%        x12(1:no,i)=Oxyz(:,1)-Sxyz(i,3);
%        x21(1:no,i)=Oxyz(:,2)-Sxyz(i,1);
%        x22(1:no,i)=Oxyz(:,2)-Sxyz(i,3);  
%        y11(1:no,i)=Oxyz(:,3)-Sxyz(i,2);
%        y12(1:no,i)=Oxyz(:,3)-Sxyz(i,4);
%        z(1:no,i)=Oxyz(:,4)-Sxyz(i,5);
%    end
%elseif OBJ==2                                 % Obj are planes    
%    for i=1:ns    
%        x11(1:no,i)=Oxyz(:,1)-Sxyz(i,1);
%        x12(1:no,i)=Oxyz(:,1)-Sxyz(i,3);
%        x21(1:no,i)=Oxyz(:,3)-Sxyz(i,1);
%        x22(1:no,i)=Oxyz(:,3)-Sxyz(i,3);  
%        y11(1:no,i)=Oxyz(:,2)-Sxyz(i,2);
%       y12(1:no,i)=Oxyz(:,2)-Sxyz(i,4);
%        y21(1:no,i)=Oxyz(:,4)-Sxyz(i,2);
%        y22(1:no,i)=Oxyz(:,4)-Sxyz(i,4);
%        z(1:no,i)=Oxyz(:,5)-Sxyz(i,5);        
%    end
%end

I1=0;

% y11
[F1]=Formula_3D_1(x11,y11,dz);
I1=I1-F1;

[F1]=Formula_3D_1(x12,y11,dz);
I1=I1+F1;

[F1]=Formula_3D_1(x21,y11,dz);
I1=I1+F1;

[F1]=Formula_3D_1(x22,y11,dz);
I1=I1-F1;

% y12
[F1]=Formula_3D_1(x11,y12,dz);
I1=I1+F1;

[F1]=Formula_3D_1(x12,y12,dz);
I1=I1-F1;

[F1]=Formula_3D_1(x21,y12,dz);
I1=I1-F1;

[F1]=Formula_3D_1(x22,y12,dz);
I1=I1+F1;

clear x11 x12 x21 x22 y11 y12 dz F1; 
end