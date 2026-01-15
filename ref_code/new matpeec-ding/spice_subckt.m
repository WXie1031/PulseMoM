function spice_subckt(Rmtx,Lmtx,Cmtx, Rfit,Lfit, bran_name,nod_name, ...
    nod_start, nod_end, bran_out, nod_out, fpath, fname,...
    Rgrid,Lgrid,Cgrid,bran_name_grid,nod_name_grid, ...
    nod_start_grid, nod_end_grid, bran_out_grid, nod_out_grid,nod_gnd_grid, Nnod_gnd_grid,Rgrod,Rsoil,pt_start,pt_start_grid,pt_end,pt_end_grid,dv,dv_grid,len,len_grid,re,re_grid,frq)

% Rsoil=100;

Lmtx_grid=Lgrid;
Rmtx_grid=Rgrid;
Cmtx_grid=Cgrid;


[Rmtx0, Lmtx0, Cmtx0,Rg, Lg,Pg, Rgself, Lgself] = ground_cmplx_plane(Rmtx_grid, Lmtx_grid, Cmtx_grid,pt_start_grid, pt_end_grid, dv_grid, re_grid, len_grid,...
    0.01, frq, 0.1,ver);

Lmtx_grid = Lmtx_grid - 2*Lg;
    Pmtx=inv(Cmtx_grid)+2*Pg;
    Cmtx2=inv(Pmtx);

 [Rgrid, Lgrid, Cgrid,nod_name_grid] = para_main_fila_rlp(pt_start_grid, pt_end_grid, dv_grid, re_grid, len_grid, ...
    nod_start_grid,nod_end_grid, 1);
Rgrod=[];
Rgrod = grid_rod_resis_v_Ding(Rgrod, pt_start_grid,pt_end_grid, dv_grid, re_grid, len_grid, Rsoil);
Rgrod = grid_rod_resis_h_Ding(Rgrod, pt_start_grid,pt_end_grid, dv_grid, re_grid, len_grid, Rsoil);

Nc1 = size(pt_start,1);
Nc2 = size(pt_start_grid,1);
Lm=zeros(Nc1,Nc2);
for ik = 1:Nc1

    % calculate inductance using filament model
    Lm(ik,1:Nc2) = cal_L_fila ...
        (pt_start(ik,1:3), pt_end(ik,1:3), dv(ik,1:3), len(ik), re(ik), ...
        pt_start_grid(1:Nc2,1:3), pt_end_grid(1:Nc2,1:3), dv_grid(1:Nc2,1:3), len_grid(1:Nc2), re_grid(1:Nc2));

end

La=zeros(Nc1,Nc2);
for ia=1:Nc1
    for ib=1:Nc2
        La(ia,ib)=abs(sum(dv(ia,:).*dv_grid(ib,:).*[1 0 0]));
    end
end
type_mutual=1; % 1 for horizontal mutual coupling only; 2 for horizontal and vertical mutual coupling
if type_mutual==1
Lmtx=[Lmtx La.*Lm;La'.*Lm' Lgrid];
else
Lmtx=[Lmtx Lm;Lm' Lgrid];
end



Rm=zeros(Nc1,Nc2);
Rmtx=[Rmtx Rm;Rm' Rgrid];
if isempty(Cmtx)
else
    Cm=zeros(2*Nc1,2*Nc2);
Cmtx=[Cmtx Cm;Cm' Cgrid];
end

Lfit_grid=zeros(Nc2,12);
Rfit_grid=zeros(Nc2,12);
Lfit=[Lfit;Lfit_grid];
Rfit=[Rfit;Rfit_grid];

for ia=1:Nc2
     bran_name_grid2(ia,1:(length(deblank(bran_name_grid(ia,:)))+5))=[deblank(bran_name_grid(ia,:)),'_grid'];
%     nod_name_grid2(ia,:)=[deblank(nod_name_grid(ia,:)),'_grid'];
%     nod_start_grid2(ia,1:(length(deblank(nod_start_grid(ia,:)))+5))=[deblank(nod_start_grid(ia,:)),'_grid'];
%     nod_end_grid2(ia,1:(length(deblank(nod_end_grid(ia,:)))+5))=[deblank(nod_end_grid(ia,:)),'_grid'];
end

for ia=1:Nc1
    bran_name(ia,1:length(deblank(bran_name(ia,:))))=deblank(bran_name(ia,:));
    nod_start(ia,1:length(deblank(nod_start(ia,:))))=deblank(nod_start(ia,:));
    nod_end(ia,1:length(deblank(nod_end(ia,:))))=deblank(nod_end(ia,:)); 
    
end
nod_gnd_grid=['1'];
for ia=1:Nc2
    bran_name(ia+Nc1,1:length(deblank(bran_name_grid2(ia,:))))=deblank(bran_name_grid2(ia,:));
    nod_start(ia+Nc1,1:length(deblank(nod_start_grid(ia,:))))=deblank(nod_start_grid(ia,:));
    nod_end(ia+Nc1,1:length(deblank(nod_end_grid(ia,:))))=deblank(nod_end_grid(ia,:));
    nod_gnd_grid(ia,1:length(deblank(nod_end_grid(ia,:))))=deblank(nod_end_grid(ia,:));
end
Nnod_gnd_grid=size(nod_gnd_grid,1);

for ia=1:(2*Nc1)
    nod_name(ia,1:length(deblank(nod_name(ia,:))))=deblank(nod_name(ia,:));
end
for ia=1:(2*Nc2)
    nod_name(ia+2*Nc1,1:length(deblank(nod_name_grid(ia,:))))=deblank(nod_name_grid(ia,:));
end
% Na1=size(nod_out,1);
% Na2=size(nod_out_grid,1); 
% ic=0;
% for ia=1:Na1
%     if isempty(deblank(nod_out(ia,:)))==1;
%     else
%         ic=ic+1;
%         nod_out(ic,1:length(deblank(nod_out(ia,:))))=deblank(nod_out(ia,:));
%     end
% end
% 
% for ia=1:Na2
%         if isempty(deblank(nod_out(ia,:)))==1;
%         else
%         ic=ic+1;
%         nod_out(ic,1:length(deblank(nod_out_grid(ia,:))))=deblank(nod_out_grid(ia,:));
%         end
% end

Na1=size(nod_out,1);
Na2=size(nod_out_grid,1); 

for ia=1:Na1
    nod_out2(ia,1:length(deblank(nod_out(ia,:))))=deblank(nod_out(ia,:));
end

for ia=1:Na2
    nod_out2(ia+Na1,1:length(deblank(nod_out_grid(ia,:))))=deblank(nod_out_grid(ia,:));
end

Nbran1=Nc1;
Nbran2=Nc2;
re=[re;re_grid];
len=[len;len_grid];

nod_out3=unique(nod_out2,'rows');
nod_out=nod_out3;




spice_subckt_self_vf(Rmtx,Lmtx,Cmtx, Rfit,Lfit, bran_name,nod_name, ...
    nod_start, nod_end, nod_gnd_grid,Nnod_gnd_grid,bran_out, nod_out, fpath, fname,Rsoil,Rgrod,Nbran1,Nbran2,re,len);










