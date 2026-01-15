
function [Bxs Bys Bzs]=BFSCAL(Oxyz,Qs,Is)

Qsx1=[Qs(1,:); Qs(3,:)];        % start point of source x 
Qsx2=[Qs(2,:); Qs(4,:)];        % end point of source x
Qsy1=[Qs(2,:); Qs(4,:)];        % start point of source y
Qsy2=[Qs(3,:); Qs(1,:)];        % end point of source y

% initilization
%[no mo]=size(Oxyz);
[no mo]=size(Oxyz); 
[ns ms]=size(Qs);
Iq=ones(1,ns/2);
Io=ones(no,1);

xo=Oxyz(:,1)*Iq; 
yo=Oxyz(:,2)*Iq; 
zo=Oxyz(:,3)*Iq; 
ux1=Io*Qsx1(:,1)';
uy=Io*Qsx1(:,2)';
uz=Io*Qsx1(:,3)';
ux2=Io*Qsx2(:,1)';  
vx=Io*Qsy1(:,1)';
vy1=Io*Qsy1(:,2)';
vz=Io*Qsy1(:,3)';
vy2=Io*Qsy2(:,2)';  

uux=xo-ux1;
uuy=yo-uy;
uuz=zo-uz;
vvx=xo-vx;
vvy=yo-vy1;
vvz=zo-vz;
[Fsy_X]=Formula_3D_Bsy(uux,uuy,uuz);
By_X=Fsy_X;
[Fsx_Y]=Formula_3D_Bsx(vvx,vvy,vvz);
Bx_Y=Fsx_Y;
[Fsz_X,Fsz_Y]=Formula_3D_Bsz(uux,uuy,uuz,vvx,vvy,vvz);
Bz_X=Fsz_X;
Bz_Y=Fsz_Y;

uux=xo-ux2;
vvy=yo-vy2;
[Fsy_X]=Formula_3D_Bsy(uux,uuy,uuz);
By_X=-By_X+Fsy_X;
[Fsx_Y]=Formula_3D_Bsx(vvx,vvy,vvz);
Bx_Y=-Bx_Y+Fsx_Y;
[Fsz_X,Fsz_Y]=Formula_3D_Bsz(uux,uuy,uuz,vvx,vvy,vvz);
Bz_X=-Bz_X+Fsz_X;
Bz_Y=-Bz_Y+Fsz_Y;

Bxs=Bx_Y*[Is;Is];
Bys=By_X*[Is;Is];
Bzs=Bz_X+Bz_Y;
Bzs=Bzs*[Is;Is];

clear xo yo zo;
clear ux1 uy uz ux2 vx vy1 vz vy2; 
clear uux uuy uuz vvx vvy vvz Fsy_X Fsz_X Fsx_Y Fsz_Y By_X Bz_X Bx_Y Bz_Y;
end
