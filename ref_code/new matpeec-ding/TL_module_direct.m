function [pt_start_2, pt_end_2, dv_2, re_2, len_2, nod_start_2,nod_end_2,shape_2, dim1_2, dim2_2, ...
    R_pul_2,Rmtx_2,Lmtx_2,Cmtx_2,nod_name_2,Rfit,Lfit]=TL_module_direct(Rmtx,Lmtx,Cmtx,Rself,Lself,pt_start, pt_end, dv, re, len, nod_start,nod_end,bran_name,shape, dim1, dim2, ...
    R_pul,fname,fpath,grp_id,grp_type,pt_2d,epr_soil,sigma_soil,p_flag,m_TypeGround,frq)

% [pt_start_2, pt_end_2, dv_2, re_2, len_2, nod_start_2,nod_end_2,shape_2, dim1_2, dim2_2, ...
%     R_pul_2,Rmtx_2,Lmtx_2,Cmtx_2,nod_name_2,Rfit,Lfit]=TL_module_direct(Rmtx,Lmtx,Cmtx,Rself,Lself,pt_start, pt_end, dv, re, len, nod_start,nod_end,bran_name,shape, dim1, dim2, ...
%     R_pul,fname,fpath,grp_id,grp_type,pt_2d,10,0.01,1,2,frq);


Nfit=3;
offset=0.1;
grp_num=max(grp_id);
Nc1=size(pt_start,1);
tl_num=0;
short_num=0;
tl_num_gp=0;
ib=1;
ic=1;
for ia=1:Nc1
    if len(ia)>=50 && sum(dv(ia,:).*[1 0 0])>=0.9397
        tl_nod_start_temp(ia,1:length([deblank(nod_start(ia,:)),'_tl']))=[deblank(nod_start(ia,:)),'_tl'];
        tl_nod_end_temp(ia,1:length([deblank(nod_end(ia,:)),'_tl']))=[deblank(nod_end(ia,:)),'_tl'];
        tl_num(ib)=ia;
        tl_num_gp(ib)=grp_id(ia);
        ib=ib+1;
        else
        short_num(ic)=ia;
        ic=ic+1;
    end
end

bran_name_2=bran_name([short_num]',:);
nod_start_2=nod_start([short_num]',:);
nod_end_2=nod_end([short_num]',:);
pt_start_2=pt_start([short_num]',:);
pt_end_2=pt_end([short_num]',:);
dv_2=dv([short_num]',:);
re_2=re([short_num]);
shape_2=shape([short_num]);
dim1_2=dim1([short_num]);
dim2_2=dim2([short_num]);
len_2=len([short_num]);
R_pul_2=R_pul([short_num]);
Rself_2=Rself([short_num],:);
Lself_2=Lself([short_num],:);

Rmtx_2=[];
Lmtx_2=[];
Cmtx_2=[];
nod_name_2=[];
[Rmtx_2, Lmtx_2, Cmtx_2,nod_name_2] = para_main_fila_rlp(pt_start_2, pt_end_2, dv_2, re_2, len_2, ...
    nod_start_2,nod_end_2, p_flag);

if m_TypeGround==0
    
elseif m_TypeGround==1
    
    [Rmtx_2, Lmtx_2, Cmtx_2,Rg, Lg,Cg, Rgself, Lgself] = ground_pec(Rmtx_2, Lmtx_2, Cmtx_2,pt_start_2, pt_end_2, dv_2, ...
    re_2, len_2, frq, offset,ver) ;
%     Lmtx_2=1e-9*ones(Nc1,Nc1);
% Cmtx_2=Cmtx_2.*1e-3;

elseif m_TypeGround==2
    [Rmtx_2, Lmtx_2, Cmtx_2,Rg, Lg,Cg, Rgself, Lgself] = ground_cmplx_plane(Rmtx_2, Lmtx_2, Cmtx_2,pt_start_2, pt_end_2, dv_2, re_2, len_2,...
    sigma_soil, frq, offset,ver);
%     Lmtx_2=2e-9*ones(Nc1,Nc1);
% Cmtx_2=Cmtx_2.*1e-3;
else
end


% nod_name_2=nod_P;


% if isempty(Rfit)==1
%     Rfit_2=[];
%     Lfit_2=[];
% else
%      Rfit_2=Rfit([short_num],:);
%      Lfit_2=Lfit([short_num],:);
% end
[R0fit,L0fit,Rfit,Lfit,Zfit] = vectfit_main_Z(Rself_2,Lself_2, frq, Nfit, Lmtx_2);
% [Rmtx_2, Lmtx_2] = update_vectfit_to_main(Rmtx_2, Lmtx_2, R0fit, L0fit);


group_num_max=length(unique(tl_num_gp));
tl_num_gp2=unique(tl_num_gp);

% for ia=tl_num(1):tl_num(end)
i_sr=zeros(1,10);
t_sr=(1:10)*1e-2;
flag_type=1;
sigma_soil=0.001;
epr_soil=10;
pt_hit=[0 0];
h_ch=1;
Ns_ch=1;
    tl_start=0;
    tl_end=0;
    

    if tl_num_gp==0
    else
for group_num=1:group_num_max
    [a]=find(tl_num_gp==tl_num_gp2(group_num));
    [b]=tl_num(a);
    tl_start=pt_start(b,:);
    tl_end=pt_end(b,:);
    tl_nod_start=tl_nod_start_temp(b,:);
    tl_nod_end=tl_nod_end_temp(b,:);
    fname_temp=[fname,'_gp',num2str(group_num,'%.2d')];
    tl_r=re(b,:);
    tl_name=['tl_gp',num2str(group_num,'%.2d')];

      main_spice_subckt_tl(pt_hit,h_ch,Ns_ch,tl_start,tl_end, i_sr, t_sr,flag_type,epr_soil,sigma_soil,fname_temp,fpath,tl_nod_start,tl_nod_end,tl_r,group_num,tl_name);
end


    end
    
    
        