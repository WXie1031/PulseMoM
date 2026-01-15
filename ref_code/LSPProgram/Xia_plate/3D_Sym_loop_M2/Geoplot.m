function [c]=Geoplot(u0,v0,ux,vx,uy,vy,Oxy_Z,Oxyz_Z0,zo,ez,c)

Nx0=length(u0);
Ny0=length(v0);
NxX=length(ux);
NyX=length(vx);
NxY=length(uy);
NyY=length(vy);
Nez=length(ez);

Ix=ones(1,NxX);
vx=0.5*(vx(1:NyX-1)+vx(2:NyX));
Iy=ones(1,NyY);
uy=0.5*(uy(1:NxY-1)+uy(2:NxY));

[No Mo]=size(Oxy_Z);
Io=ones(No,1);

c=c+1;
figure(c);
% plot the total box
for i=1:Nez
    mesh(u0,v0,ones(Ny0,Nx0)*ez(i));
    hold on;
end
for i=1:Ny0
    for j=1:Nx0
        plot3([u0(j),u0(j)],[v0(i),v0(i)],[ez(1),ez(end)],'r');
        hold on;
    end
end

% plot the objective points for Mz applying M0 in edge blocks
plot3(Oxyz_Z0(:,1),Oxyz_Z0(:,2),Oxyz_Z0(:,3),'r*');           
hold on;

% plot the segments for Jx and Jy (kvl)
for i=1:2
    for j=1:NyX-1
        plot3(ux,vx(j)*Ix,zo(i)*Ix,'r-o','linewidth',1);
        hold on;
    end
    for j=1:NxY-1
        plot3(uy(j)*Iy,vy,zo(i)*Iy,'b-o','linewidth',1);
        hold on;
    end
end

% plot the objective points for Mx and My
for i=1:2
    plot3(Oxy_Z(:,1),Oxy_Z(:,2),Io*zo(i),'kx');
    hold on;
end
hold off;

end



