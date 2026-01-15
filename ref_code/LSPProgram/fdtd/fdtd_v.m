function [vout, t] = fdtd_v(fname)

T = load(fname);
Nall = size(T,1);
Nt = T(1,1);

Npt = Nall/(Nt+1);

vout = reshape(T(:,2),Nt+1,Npt);
vout = vout(2:Nt+1,:);
t = T(2:Nt+1,1);

end

