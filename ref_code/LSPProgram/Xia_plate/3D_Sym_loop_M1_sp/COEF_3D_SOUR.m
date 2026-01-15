%
% Oxy(xo,yo),Oxy_X(xo1,xo2,yo),Oxy_Y(xo,yo1,yo2),zo(zo1,zo2)
% Qs=[x1 x2 y z;
%     x y1 y2 z;
%     x y z1 z2]
%
function [Sx,Sy,Bxs,Bys,Bzs]=COEF_3D_SOUR(Oxy,Oxy_X,Oxy_Y,zo,Qs,Is,Ns)

zo1=zo(1); zo2=zo(2);

[no mo]=size(Oxy);
[nox mox]=size(Oxy_X); 
[noy moy]=size(Oxy_Y);
Io=ones(no,1);
Iox=ones(nox,1);
Ioy=ones(noy,1);

Oxyz=[Oxy Io*zo1;
      Oxy Io*zo2];
XOxyz=[Oxy_X Iox*zo1;
       Oxy_X Iox*zo2];
YOxyz=[Oxy_Y Ioy*zo1;
       Oxy_Y Ioy*zo2];
   
[Bxs,Bys,Bzs]=Cal_3D_Bs(Oxyz,Qs,Is,Ns);

[Sx,Sy,Sz]=Cal_3D_Js(XOxyz,YOxyz,[],Qs,Is,Ns);

end
