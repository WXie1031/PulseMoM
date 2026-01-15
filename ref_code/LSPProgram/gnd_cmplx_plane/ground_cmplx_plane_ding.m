function [Rmtx, Lmtx2, Cmtx2,Rg, Lg,Pg, Rgself, Lgself] = ground_cmplx_plane_ding(Rmtx, Lmtx, Cmtx,pt_start, pt_end, dv, re, len,...
    sig_soil, frq, offset,ver)

Nc = size(pt_start,1);
[Rmtx, Lmtx, Cmtx,Rg, Lg,Pg, Rgself, Lgself] = ground_pec(Rmtx, Lmtx, Cmtx,pt_start, pt_end, dv, ...
    re, len, frq, offset,ver) ;

Lmtx=Lmtx-Lg;
La=zeros(Nc,Nc);
for ia=1:Nc
    for ib=1:Nc
        La(ia,ib)=abs(sum(dv(ia,:).*[0 0 1]));
    end
end
Lg=(Lg.*La);

Lmtx2 = Lmtx +Lg;
    Cmtx2=Cmtx;


