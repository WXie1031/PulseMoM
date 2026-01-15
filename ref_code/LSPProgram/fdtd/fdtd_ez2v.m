function [vout, t] = fdtd_ez2v(fname, dl, Nobs_line)

if nargin < 3
    Nobs_line = 1;
end

T = load(fname);
Nall = size(T,1);
Nt = T(1,1);

Npt = Nall/(Nt+1);
Npt_line = Npt/Nobs_line;

Ez = reshape(T(:,2),Nt+1,Npt);

vout = zeros(Nobs_line, Nt-1);
for ik = 1:Nobs_line
    vout(ik,1:Nt-1) = sum(Ez(2:Nt,Npt_line*(ik-1)+(2:Npt_line)),2)*dl;
end

t = T(2:Nt,1);

end

