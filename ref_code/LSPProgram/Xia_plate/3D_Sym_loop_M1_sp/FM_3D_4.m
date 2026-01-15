% Ly (Jy)
%
% I4=int(f4,y,x',y',z'),   f4=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% I4'=int(f4,x,y,x',y',z'),   f4=1/(dx*dx+dy*dy+dz*dz)^(1/2)
% Oxyz_Y, Sxyz_Y
%
% updated on 15/02/2011
%
function [I4]=FM_3D_4(Oxyz,Sxyz)   

% Obj are lines, Oxyz(xo,y1,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% Obj are planes, Oxyz(x1,y1,x2,y2,zo),Sxyz(x1,y1,x2,y2,zs)
% OBJ=1,"line"; OBJ=2,"plane"

[no mo]=size(Oxyz);
[ns ms]=size(Sxyz);
Io=ones(no,1);
Is=ones(1,ns);

xo=Oxyz(:,1)*Is;
yo1=Oxyz(:,2)*Is;
yo2=Oxyz(:,3)*Is;
zo=Oxyz(:,4)*Is;

xs1=Io*Sxyz(:,1)';
xs2=Io*Sxyz(:,2)';
ys1=Io*Sxyz(:,3)';
ys2=Io*Sxyz(:,4)';
zs=Io*Sxyz(:,5)';

x11=xo-xs1;
x12=xo-xs2;
y11=yo1-ys1;
y12=yo1-ys2;
y21=yo2-ys1;
y22=yo2-ys2;
dz=zo-zs;

clear Io Is xo yo1 yo2 zo xs1 xs2 ys1 ys2 zs;

%if OBJ==1                                     % Obj are lines
%    for i=1:ns                             
%        x11(1:no,i)=Oxyz(:,1)-Sxyz(i,1);
%        x12(1:no,i)=Oxyz(:,1)-Sxyz(i,3);
%        y11(1:no,i)=Oxyz(:,2)-Sxyz(i,2);
%        y12(1:no,i)=Oxyz(:,2)-Sxyz(i,4);
%        y21(1:no,i)=Oxyz(:,3)-Sxyz(i,2);
%        y22(1:no,i)=Oxyz(:,3)-Sxyz(i,4);          
%        z(1:no,i)=Oxyz(:,4)-Sxyz(i,5);
%    end
%elseif OBJ==2                                 % Obj are planes    
%    for i=1:ns    
%        x11(1:no,i)=Oxyz(:,1)-Sxyz(i,1);
%        x12(1:no,i)=Oxyz(:,1)-Sxyz(i,3);
%        x21(1:no,i)=Oxyz(:,3)-Sxyz(i,1);
%        x22(1:no,i)=Oxyz(:,3)-Sxyz(i,3);  
%        y11(1:no,i)=Oxyz(:,2)-Sxyz(i,2);
%        y12(1:no,i)=Oxyz(:,2)-Sxyz(i,4);
%        y21(1:no,i)=Oxyz(:,4)-Sxyz(i,2);
%        y22(1:no,i)=Oxyz(:,4)-Sxyz(i,4);
%        z(1:no,i)=Oxyz(:,5)-Sxyz(i,5);        
%    end
%end

I4=0;

% x11
[F4]=Formula_3D_4(x11,y11,dz);
I4=I4-F4;

[F4]=Formula_3D_4(x11,y12,dz);
I4=I4+F4;

[F4]=Formula_3D_4(x11,y21,dz);
I4=I4+F4;

[F4]=Formula_3D_4(x11,y22,dz);
I4=I4-F4;

% x12
[F4]=Formula_3D_4(x12,y11,dz);
I4=I4+F4;

[F4]=Formula_3D_4(x12,y12,dz);
I4=I4-F4;

[F4]=Formula_3D_4(x12,y21,dz);
I4=I4-F4;

[F4]=Formula_3D_4(x12,y22,dz);
I4=I4+F4;

clear x11 x12 y11 y12 y21 y22 dz F4;

end