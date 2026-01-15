function [pt_start,pt_end, shp_id, dim1,dim2,re, R_pul, sig, mur] = ...
    mesh_grid(pt_mid,W,L,Nw,Nl, shp_in, d1_in,d2_in,re_in, Rpul_in, sig_in, mur_in)

Nsw = Nw*(Nl+1);
Nsl = (Nw+1)*Nl;
Ns = Nsw+Nsl;

pt_start = zeros(Ns,3);
pt_end = zeros(Ns,3);

dlx_tmp = W./Nw;
dly_tmp = L./Nl;

xs = pt_mid(1)-W/2 + dlx_tmp*((0:Nw)');
ys = pt_mid(2)-L/2 + dly_tmp*((0:Nl)');

[Xxgrd, Yxgrd] = meshgrid(xs, ys);

pt_start(1:Nsw,1) = reshape(Xxgrd(:,1:Nw)',Nsw,1);
pt_start(1:Nsw,2) = reshape(Yxgrd(:,1:Nw)',Nsw,1);

pt_end(1:Nsw,1) = reshape(Xxgrd(:,2:Nw+1)',Nsw,1);
pt_end(1:Nsw,2) = reshape(Yxgrd(:,2:Nw+1)',Nsw,1);

pt_start(Nsw+(1:Nsl),1) = reshape(Xxgrd(1:Nl,:)',Nsl,1);
pt_start(Nsw+(1:Nsl),2) = reshape(Yxgrd(1:Nl,:)',Nsl,1);

pt_end(Nsw+(1:Nsl),1) = reshape(Xxgrd(2:Nl+1,:)',Nsl,1);
pt_end(Nsw+(1:Nsl),2) = reshape(Yxgrd(2:Nl+1,:)',Nsl,1);

pt_start(:,3) = pt_mid(3);
pt_end(:,3) = pt_mid(3);

shp_id = shp_in*ones(Ns,1);
dim1 = d1_in*ones(Ns,1);
dim2 = d2_in*ones(Ns,1);
R_pul = Rpul_in*ones(Ns,1);
sig = sig_in*ones(Ns,1);
mur = mur_in*ones(Ns,1);
re = re_in*ones(Ns,1);
% figure(56)
% hold on
% for ik=1:Ns
%     plot3( [pt_start(ik,1); pt_end(ik,1)], [pt_start(ik,2); pt_end(ik,2)],[pt_start(ik,3); pt_end(ik,3)])
% end
% hold off
% axis equal



end

