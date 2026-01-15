function main_spice_subckt_tl(pt_hit,h_ch,Ns_ch,tl_start,tl_end, i_sr, t_sr,flag_type,epr_soil,sigma_soil,fname,fpath,tl_nod_start,tl_nod_end,r,group_num,tl_name)

%generate the netlist for DEPACT transmission lines model
% pt_hit=[0 0];
% h_ch=800;
% Ns_ch=400;
% tl_start=[-500 46.7 10;-500 50 10; -500 53.7 10];
% tl_end=[500 46.7 10;500 50 10; 500 53.7 10];
% r=[0.005;0.005;0.005];
% fname= 'test3'
% fpath= 'C:\Users\Ding\Desktop\test3\'
% ground_type=3; % 1 for no ground, 2 for PEC ground, 3 for lossy ground
% %%---------------------------- 0.25/100 us current source
% Imax=10e3;
% k0=1;
% tau1=0.45e-6;
% tau2=143e-6;
% n=10;
% Nt=8e-6/10*3e8*3;
% dt=10/3e8/3;
% i_sr=0;
% t_sr=0;
%     for i=1:Nt
%         I_temp(i)=(i*dt/tau1)^n/(1+(i*dt/tau1)^n);
%         i_sr(i)=Imax/k0*(I_temp(i))*exp(-i*dt/tau2);
%     end
% t_sr=(1:Nt)*dt*1e6;

% epr_soil=10;
% sigma_soil=0.001;
% %%------------------------------------------------------

sect_num=2;
ground_type=3;
% dz_tl=9.74;
dz_tl=10;
dt = (t_sr(2)-t_sr(1))*1e-6;
dis_tl=floor(dz_tl/3e8/dt);
% length_tl=floor(abs(tl_end(1,1)-tl_start(1,1))/dz_tl)*dz_tl;
length_tl=floor(abs(tl_end(1,1)-tl_start(1,1))/dz_tl)*dz_tl;


[Y, Rg0, Lg0, Rgn, Lgn]=TL_cal(tl_start,tl_end,epr_soil,sigma_soil,r,ground_type);


flag_load=2;
if flag_load==1
    load 'H:\test long line\jefimenko_Ding\Uind.mat'
%    Uout0=Uind;
    I_s1=zeros(50,Nt);
for ia=1:50
    I_s1(ia,(ia*3-2):Nt)=Uout0(ia,1:(Nt-3*ia+3));
end

I_s11=sum(I_s1(:,:),1)/Zm;

I_s2=zeros(50,Nt);
for ia=1:50
    I_s2(ia,((51-ia)*3-2):Nt)=Uout0(ia,1:(Nt-3*(51-ia)+3));
end

I_s22=sum(I_s2(:,:),1)/Zm;

I_s33=zeros(50,Nt);
for ia=1:50
    I_s3(ia,(ia*3-2):Nt)=Uout0(50+ia,1:(Nt-3*ia+3));
end

I_s33=sum(I_s3(:,:),1)/Zm;

I_s4=zeros(50,Nt);
for ia=1:50
    I_s4(ia,((51-ia)*3-2):Nt)=Uout0(50+ia,1:(Nt-3*(51-ia)+3));
end

I_s44=sum(I_s4(:,:),1)/Zm;

U_s1=Uout0(101,1:Nt);

U_s2=Uout0(101,1:Nt);

Is=zeros(4,Nt);
Us=zeros(2,Nt);
Is(1,:)=I_s11;
Is(2,:)=I_s22;
Is(3,:)=I_s33;
Is(4,:)=I_s44;
Us(1,:)=U_s1;
Us(2,:)=U_s2;
else 
    [pt_a0 pt_b0]=size(tl_start);

%   abc=100;
abc=round(length_tl/dz_tl);
nk=abc;
Point=zeros((nk+1)*(pt_a0-1)+nk+1,3);
for i=1:pt_a0
Kx=(tl_end(i,1)-tl_start(i,1))/nk;
Ky=(tl_end(i,2)-tl_start(i,2))/nk;
Kz=(tl_end(i,3)-tl_start(i,3))/nk;
Point((nk+1)*(i-1)+1,:)=tl_start(i,:);
for ik=1:1*nk
    Point((nk+1)*(i-1)+ik+1,1)=Point((nk+1)*(i-1)+ik,1)+Kx;
    Point((nk+1)*(i-1)+ik+1,2)=Point((nk+1)*(i-1)+ik,2)+Ky;
    Point((nk+1)*(i-1)+ik+1,3)=Point((nk+1)*(i-1)+ik,3)+Kz;
end
end
pt_start=zeros(pt_a0*abc,pt_b0);
pt_end=zeros(pt_a0*abc,pt_b0);
for i=1:pt_a0
pt_start((abc*(i-1)+1):(abc*i),:)=Point(((abc+1)*(i-1)+1):((abc+1)*i-1),:);
pt_end((abc*(i-1)+1):(abc*i),:)=Point(((abc+1)*(i-1)+2):((abc+1)*i),:);
end

for ia=1:pt_a0
    pt_start(pt_a0*abc+ia,:)=tl_start(ia,:);
     pt_start(pt_a0*abc+ia+pt_a0,:)=tl_end(ia,:);
end
%------------------------------------------------------------------------------
% for ia=1:pt_a0
%     pt_end(pt_a0*abc+ia,1:2)=tl_start(ia,1:2);
%      pt_end(pt_a0*abc+ia+pt_a0,1:2)=tl_end(ia,1:2);
%      pt_end(pt_a0*abc+ia,3)=0;
%      pt_end(pt_a0*abc+ia+pt_a0,3)=0;
% end

%------------------------------------------------------------------------------
for ia=1:pt_a0
    pt_end(pt_a0*abc+ia,1:2)=tl_start(ia,1:2);
     pt_end(pt_a0*abc+ia+pt_a0,1:2)=tl_end(ia,1:2);
     pt_end(pt_a0*abc+ia,3)=0;
     pt_end(pt_a0*abc+ia+pt_a0,3)=0;
end
pt_end(pt_a0*abc+1,3)=5.7;
pt_end(pt_a0*abc+1+pt_a0,3)=5.7;
%------------------------------------------------------------------------------

    pt_start_grid=[];
    pt_end_grid=[];
    [Uout0,E_all_r,E_all_z] = sr_induced_v_num(pt_hit,h_ch,Ns_ch, pt_start,pt_end,pt_start_grid,pt_end_grid, i_sr, t_sr);
%        Uou0=Uind;
% Uout0(1:200,1:Nt)=0;
% Uout0(401:600,1:Nt)=0;
% Uout0(603:606,1:Nt)=0;

line_num=size(tl_start,1);

% abc=100;
abc2=abc/sect_num;
Nt=size(t_sr,2);
dt = (t_sr(2)-t_sr(1))*1e-6;

% dis_tl=floor(dz_tl/3e8/dt);
I_s0=zeros(line_num*sect_num,abc2,Nt);

for ia=1:line_num
    for ib=1:sect_num
        for ic=1:abc2
            
            I_s0((ia-1)*sect_num*2+2*ib,ic,((abc2-ic)*dis_tl+1):Nt)=-Uout0(((ia-1)*sect_num+ib-1)*abc2+ic,1:(Nt-dis_tl*(abc2-ic)));
            
                     
            I_s0((ia-1)*sect_num*2+2*ib-1,ic,((ic-1)*dis_tl+1):Nt)=Uout0(((ia-1)*sect_num+ib-1)*abc2+ic,1:(Nt-dis_tl*ic+dis_tl));
                    end
    end
end
% testa11=zeros(100,Nt);
% for ic=1:100
%     testa11(ic,(ic*3):Nt)=testa1(ic,1:(Nt-(ic*3)+1));
% end
% testa22=zeros(100,Nt);
% for ic=1:100
%     testa22(ic,(ic*3):Nt)=testa2(ic,1:(Nt-(ic*3)+1));
% end
% testa33=zeros(100,Nt);
% for ic=1:100
%     testa33(ic,(ic*3):Nt)=testa3(ic,1:(Nt-(ic*3)+1));
% end


I_s01=sum(I_s0,2);
Is0=zeros(line_num*sect_num*2,Nt);
Is0(:,:)=I_s01(:,1,:);
I_s02=zeros(line_num,sect_num*2,Nt);
for ia=1:line_num
    for ib=1:sect_num*2
        I_s02(ia,ib,:)=I_s01((ia-1)*sect_num*2+ib,1,:);
    end
end
I_s03=zeros(line_num,sect_num*2,Nt);
for it=1:Nt
    I_s03(:,:,it)=Y*I_s02(:,:,it);
% I_s03(:,:,it)=I_s02(:,:,it)/500;
end
Is=zeros(line_num*sect_num*2,Nt);
for ia=1:line_num
    for ib=1:sect_num*2
        Is((ia-1)*sect_num*2+ib,:)=I_s03(ia,ib,:);
    end
end


Us=zeros(line_num*2,Nt);
Us(1:(line_num*2),:)=Uout0(line_num*abc+(1:line_num*2),:);
       
% test1=zeros(1,Nt);
% test2=zeros(1,Nt);
% test3=zeros(1,Nt);
% 
% test1(1,36:end)=Is(2,1:(end-35));
% test2(1,36:end)=Is(3,1:(end-35));
% test3(1,71:end)=Us(2,1:(end-70));
%  
% test4=zeros(1,Nt);
% test5=zeros(1,Nt);
% test6=zeros(1,Nt);
% 
% test4(1,36:end)=Is(6,1:(end-35));
% test5(1,36:end)=Is(7,1:(end-35));
% test6(1,71:end)=Us(4,1:(end-70));


% 
% test5=zeros(1,Nt);
% test6=zeros(1,Nt);

% test5(1,301:end)=Is(10,1:(end-300));
% test6(1,601:end)=Us(6,1:(end-600));

% test7=zeros(1,Nt);
% test8=zeros(1,Nt);
% test9=zeros(1,Nt);
% test7(1,601:end)=Is(4,1:(end-600));
% test8(1,601:end)=Is(8,1:(end-600));
% test9(1,601:end)=Is(12,1:(end-600));

% test11=zeros(1,Nt);
% test12=zeros(1,Nt);
% test13=zeros(1,Nt);

% test11(1,301:end)=Is6(2,1:(end-300));
% test12(1,301:end)=Is0(6,1:(end-300));

% Z1=[461.4496 0 0;0 461.4496 0; 0 0 461.4496];
% Z2=inv(Y);
% Ztemp=Z1*inv(Z1+Z2);
% Ztemp2=Z2*inv(Z1+Z2);
% alpha=(Z1-Z2)*inv(Z1+Z2);

% It1=[Is(1,1:Nt);Is(5,1:Nt);Is(9,1:Nt)];
% It2=[test1;test3;test5];
% It3=[test7;test8;test9];
% It4=[test2;test4;test6];


% I_s11=sum(I_s1(:,:),1)/Zm;
% 
% I_s2=zeros(50,Nt);
% for ia=1:50
%     I_s2(ia,((51-ia)*3-2):Nt)=Uout0(ia,1:(Nt-3*(51-ia)+3));
% end
% 
% I_s22=sum(I_s2(:,:),1)/Zm;
% 
% I_s33=zeros(50,Nt);
% for ia=1a:50
%     I_s3(ia,(ia*3-2):Nt)=Uout0(50+ia,1:(Nt-3*ia+3));
% end
% 
% I_s33=sum(I_s3(:,:),1)/Zm;
% 
% I_s4=zeros(50,Nt);
% for ia=1:50
%     I_s4(ia,((51-ia)*3-2):Nt)=Uout0(50+ia,1:(Nt-3*(51-ia)+3));
% end
% 
% I_s44=sum(I_s4(:,:),1)/Zm;

% U_s1=Uout0(101,1:Nt);
% 
% U_s2=Uout0(101,1:Nt);
% 
% Is=zeros(4,Nt);
% Us=zeros(2,Nt);
% Is(1,:)=I_s11;
% Is(2,:)=-I_s22;
% Is(3,:)=I_s33;
% Is(4,:)=-I_s44;
% Us(1,:)=U_s1;
% Us(2,:)=U_s2;
    

end
Nbran=size(Is,1);
tui=t_sr*1e-6;
line_num=size(tl_start,1);

fid = fopen([fpath, fname,'.inc'],'w+');
fprintf(fid,'* Add induced voltage files. \n');

for ia=1:line_num
for ib=1:sect_num
for ik = 1:2
    Uname = ['U_IND_L',num2str(ia,'%.1d'),num2str(ib,'%.1d'),num2str(ik,'%.4d'),'_G',num2str(group_num,'%.1d')];
    fprintf(fid,'.STMLIB "../../../%s.stl" \n', deblank(Uname));    
end
end
end
for ia=1:(line_num*2)
    Uname1=['U_IND_END000',num2str(ia,'%.1d'),'_G',num2str(group_num,'%.1d')];
    fprintf(fid,'.STMLIB "../../../U_IND_END000%d_G%d.stl" \n',ia,group_num);
    spice_induced_v_num(tui, Us(ia,:), fpath, deblank(Uname1));
end
% Uname1= 'U_IND_END0001';
% Uname2= 'U_IND_END0002';

% fprintf(fid,'.STMLIB "../../../U_IND_END0001.stl" \n');
% fprintf(fid,'.STMLIB "../../../U_IND_END0002.stl" \n');
% spice_induced_v_num(tui, Us, fpath, deblank(Uname1));
% spice_induced_v_num(tui, Us(2,:), fpath, deblank(Uname2));

fclose(fid);

temp1=1;
for ia=1:line_num
for ib=1:sect_num
    for ik = 1:2
        Uname = ['U_IND_L',num2str(ia,'%.1d'),num2str(ib,'%.1d'),num2str(ik,'%.4d'),'_G',num2str(group_num,'%.1d')];
    spice_induced_v_num(tui, Is(temp1,:), fpath, deblank(Uname))
    temp1=temp1+1;
end
end
end
Yt=zeros(line_num,line_num);
for ia=1:line_num
    for ib=1:line_num
        if ia==ib
            Yt(ia,ib)=sum(Y(ia,:));
        else
            Yt(ia,ib)=-Y(ia,ib);
        end
    end
end

Zmt=1./Yt;
Zm=inv(Y);
tl_length=abs(tl_start(1,1,1)-tl_end(1,1,1));
sect_length=tl_length/sect_num;
% Zm=[460 1e6 1e6;1e6 460 1e6;1e6 1e6 460];

PEC_flag=1;
if PEC_flag==1

spice_subckt_tl(fname,fpath,line_num,sect_num,Zmt,Zm,Y, Rg0/1e6, Lg0/1e6, Rgn/1e6, Lgn/1e6,tl_nod_start,tl_nod_end,tl_name,sect_length,group_num)

else
    spice_subckt_tl(fname,fpath,line_num,sect_num,Zmt,Zm,Y, Rg0, Lg0, Rgn, Lgn,tl_nod_start,tl_nod_end,tl_name,sect_length,group_num)
end


% Uss2=spline(Usout(:,1),Usout(:,2),(1:Nt)*dt);

% plot((1:Nt)*dt,Uss2-Us(1,:))

% plot(Usout3h(:,1),Usout3h(:,2)+Usout3h(:,3))



end






