function [Y, Rg0, Lg0, Rgn, Lgn]=TL_cal(tl_start,tl_end,epr_soil,sigma_soil,r,ground_type)

% Function:   calculate the characteristic impedance and ground impedance
% of transmission lines

% input: tl_start  --  the start point of transmission lines
%        tl_end    --  the end point of transmission lines
%        epr_soil, sigma_soil  --  the permittivity and conductance of soil
%        r         --   the radius of transmission lines
% output:Zm        -- the characteristic impedance of transmission lines
%        Rg0, Lg0, Rgn, Zgn  --  the vector fitting result

% The vector fitting frequency w=[5e1 1e2 5e2 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6];    
% Nd=3 is the vector fitting rank

%------------------------------------input
ground_type=3;  % 1 for no ground; 2 for perfect ground; 3 for complex ground;
sigma_soil=0.001;
epr_soil=10;
% tl_start=[-500 46.3 10;-500 50 10; -500 53.7 10];
% tl_end=[500 46.3 10;500 50 10; 500 53.7 10];
% tl_start=[-500 46.3 10;-500 50 10;];
% tl_end=[500 46.3 10;500 50 10;];
% % tl_start=[-500 50 10;-500 50 13.7; -500 50 17.4];
% % tl_end=[500 50 10;500 50 13.7; 500 50 17.4];
% r=[9.14e-3;9.14e-3;9.14e-3];
% r=[9.14e-3;9.14e-3];
%------------------------------------------
% ground_type=3;  % 1 for no ground; 2 for perfect ground; 3 for complex ground;
% sigma_soil=0.001;
% epr_soil=10;
% tl_start=[-500 46.7 10;-500 50 10];
% tl_end=[500 45 10;500 50 10];
% r=[5e-3;5e-3;5e-3];






sigma_wire=1/59e6; % const
 ep0=8.85e-12;% const
line_num=size(tl_start,1);

Nd=3;
Rg0=zeros(line_num,line_num);
Lg0=zeros(line_num,line_num);
Rgn=zeros(line_num,line_num,Nd);
Lgn=zeros(line_num,line_num,Nd);

[a00 b00]=size(tl_start);
Rc=1/5e7;
for i1=1:a00
    tl_length(i1)=[sqrt((tl_start(i1,1)-tl_end(i1,1))^2+(tl_start(i1,2)-tl_end(i1,2))^2)];
    h(i1)=tl_start(i1,3);
end


Ns_tl=10;
u0=1e-7*4*pi;
vc=3e8;
% function 
L_self=zeros(a00,a00);
L_m=zeros(a00,a00);
Lmtx=zeros(a00,a00);
if ground_type==1;
%     for i=1:a00
%     L_self(i,i)=u0/2/pi*log(2*tl_length(i)/r(i)-1)/tl_length(i);
%     end
    for i1=1:a00
        for j1=1:a00 
            if i1==j1
                Lmtx(i1,j1)=u0/2/pi*log(2*Ns_tl/r(i1)-1);
            else
%             D=sqrt((tl_start(i,1)-tl_start(j,i))^2+(tl_start(i,2)-tl_start(j,2))^2+(tl_start(i,3)-tl_start(j,3))^2);
              D=1;
            Lmtx(i1,j1)=u0/2/pi*log(D/r(i1));
            end
        end
            
    end

    
else if ground_type==2;
            for i1=1:a00
        for j1=1:a00 
            if i1==j1
                Lmtx(i1,j1)=u0/2/pi*log(2*h(i1)/r(i1));
            else
%             D=sqrt((tl_start(i,1)-tl_start(j,i))^2+(tl_start(i,2)-tl_start(j,2))^2+(tl_start(i,3)-tl_start(j,3))^2);
              D=1;
              d=sqrt((tl_start(i1,1)-tl_start(j1,1))^2+(tl_start(i1,2)-tl_start(j1,2))^2);
            Lmtx(i1,j1)=u0/4/pi*log((d^2+(h(i1)+h(j1))^2)/(d^2+(h(i1)-h(j1))^2));
            end
        end
            
    end
    else if ground_type==3;
                        for i1=1:a00
                            for j1=1:a00 
                                if i1==j1
                                    Lmtx(i1,j1)=u0/2/pi*log(2*h(i1)/r(i1));
                                else
                                    %             D=sqrt((tl_start(i,1)-tl_start(j,i))^2+(tl_start(i,2)-tl_start(j,2))^2+(tl_start(i,3)-tl_start(j,3))^2);
                                    D=1;
                                    d=sqrt((tl_start(i1,1)-tl_start(j1,1))^2+(tl_start(i1,2)-tl_start(j1,2))^2);
                                    Lmtx(i1,j1)=u0/4/pi*log((d^2+(h(i1)+h(j1))^2)/(d^2+(h(i1)-h(j1))^2));
                                end
                            end
                        end
                        w=[5e1 1e2 5e2 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6];    
                        for i1=1:a00
                            for j1=1:a00
                                for ii=1:length(w)
                                    d=sqrt((tl_start(i1,1)-tl_start(j1,1))^2+(tl_start(i1,2)-tl_start(j1,2))^2);
                                    rg(ii)=j*w(ii)*sqrt(u0*(epr_soil*ep0+sigma_soil/(j*w(ii))));
                                    rw(ii)=j*w(ii)*sqrt(u0*ep0);
                                    Zw_v(i1,j1,ii)=(rw(ii)*besseli(0,rw(ii)*r(i1)))/(2*pi*r(i1)*sigma_wire*besseli(1,rw(ii)*r(i1)));
                                    Zg_v(i1,j1,ii)=j*w(ii)*u0/2/pi*log((1+rg(ii)*h(i1))/(rg(ii)*h(i1)));
                                    Zgm_v(i1,j1,ii)=j*w(ii)*u0/4/pi*log(((1+rg(ii)*(h(i1)/2+h(j1)/2))^2+(rg(ii)*d/2)^2)/((rg(ii)*(h(i1)/2+h(j1)/2))^2+(rg(ii)*d/2)^2));
                                end
                            end
                        end
%                         w=2*pi*1e4;    
%                         for i1=1:a00
%                             for j1=1:a00
% 
%                                     d=sqrt((tl_start(i1,1)-tl_start(j1,1))^2+(tl_start(i1,2)-tl_start(j1,2))^2);
%                                     rg=j*w*sqrt(u0*(epr_soil*ep0+sigma_soil/(j*w)));
%                                     rw=j*w*sqrt(u0*ep0);
%                                     Zw_v(i1,j1)=(rw*besseli(0,rw*r(i1)))/(2*pi*r(i1)*sigma_wire*besseli(1,rw*r(i1)));
%                                     Zg_v(i1,j1)=j*w*u0/2/pi*log((1+rg*h(i1))/(rg*h(i1)));
%                                     Zgm_v(i1,j1)=j*w*u0/4/pi*log(((1+rg*(h(i1)/2+h(j1)/2))^2+(rg*d/2)^2)/((rg*(h(i1)/2+h(j1)/2))^2+(rg*d/2)^2));
% 
%                             end
%                         end
%                         w=[5e1 1e2 5e2 1e3 5e3 1e4 5e4 1e5 5e5 1e6 5e6];              
                        Nd=3;
                        R0m=zeros(line_num,line_num);
                        L0m=zeros(line_num,line_num);
                        Rnm=zeros(line_num,line_num,Nd);
                        Lnm=zeros(line_num,line_num,Nd);
                        for ia=1:line_num
                            for ib=1:line_num
                                [Rg0(ia,ib),Lg0(ia,ib),Rgn(ia,ib,:),Lgn(ia,ib,:),Zfitm(ia,ib,:)]=vecfit_kernel_Z_Ding((Zgm_v(ia,ib,:)), w/2/pi, Nd);
                            end
                        end
                                
%                         [R0, L0, Rn, Ln, Zfit] = vecfit_kernel_Z_Ding((Zg_v), w/2/pi, Nd);
%                         [R0m, L0m, Rnm, Lnm, Zfitm] = vecfit_kernel_Z_Ding((Zgm_v), w/2/pi, Nd);
                        
                   
                        test1=zeros(11,1);
                        test2=zeros(11,1);
                        test1(1:11,1)=Zfitm(1,1,1:11);
                        test2(1:11,1)=Zg_v(1,1,1:11);
    
    
                        end
        
    end
    
end
% Lmtx=L_self+L_m;
% Cmtx=1./vc^2./Lmtx;
Cmtx=inv(Lmtx*vc^2);


Rmtx=zeros(a00,a00);
for i1=1:a00
    Rmtx(i1,i1)=Rc/pi/(r(i1))^2;
end
Cmtx0=Cmtx*Ns_tl;
Lmtx0=Lmtx*Ns_tl;
Rmtx0=Rmtx*Ns_tl;
% Lmtx=zeros(102,102);
% Rmtx=zeros(102,102);
% Cmtx=zeros(103,103);
[Tv Lm]=eig(Lmtx);
% [Ti Cm]=eig(Cmtx);
Cm=Tv'*Cmtx*Tv;
Zm=zeros(a00,a00);
for ia=1:a00
    for ib=1:a00
        if ia==ib
            Zm(ia,ib)=sqrt(Lm(ia,ib)./Cm(ia,ib));
        else
        end
    end
end

Y=Tv*inv(Zm)*inv(Tv);
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


% Zm=sqrt(Lmtx./Cmtx);

% [La Lb]=eig(Lmtx0*Cmtx0);
% [Ca Cb]=eig(Cmtx0*Lmtx0);

% Rg0=R0m;
% Lg0=L0m;
% Rgn=Rnm;
% Lgn=Lnm;


% for i=1:102
%     for j=1:102
%         if i==j
%             Lmtx(i,j)=Lmtx0;
%             end
%     end
% end
% 
% for i=1:103
%     for j=1:103
%         if i==j
%             Cmtx(i,j)=Cmtx0;
%             end
%     end
% end
            
% Z=sqrt(Lmtx/Cmtx)
% [k_a k_b]=size(nod_start);
% for ia=1:k_b
% nod_name(1:2:(2*k_a-1),ia)=nod_start(1:k_a,ia);
% nod_name(2:2:(2*k_a),ia)=nod_end(1:k_a,ia);
% end

% [nod_a nod_b]=size(nod_start);
% nod_name=nod_start;
% nod_name(nod_a+1,:)=nod_start(1,:);

