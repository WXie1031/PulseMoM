% INTE_LS       Perform 4-fold integration for Source Inductance 
%               with Gauss-L integration over z, and 
%               3-fold integration(x'/y'xy) with closed-form formula (T2SP)
% X=[x1 x2] Y=(y1 y2)  Z=(z1 z2) vectors -> L (matrix) (nf x ns) nf=ns
% INDEX         1='++', 2='-+' 3='+-' 4='--'
% xy_index      1= x source current, 2 = y source current
% Jan. 2016

function L=INTE_LS(X,Y,Z,Ps,alpha,T,A,INDEX,xy_index)
NG=length(A);
[ns m]=size(Ps);
[nf m]=size(X);
Xs=Ps(1:ns,1:3:4);
Ys=Ps(1:ns,2:3:5);
Zs=Ps(1:ns,3:3:6);

% (1) Level 1 integration (z)
switch INDEX
    case 1
        alph_f=+conj(alpha);  ulim=exp(alph_f*Z);            % field
    case 2
        alph_f=-conj(alpha);  ulim=exp(alph_f*Z);     
end    
du=ulim(1,2)-ulim(1,1);                                     % field
ui=0.5*du*T+0.5*(ulim(1,2)+ulim(1,1));

zf= log(ui)/alph_f;

switch xy_index 
    case 1
% (a) X-current
        f=G_T2SPx2(X,Y,zf,Xs,Ys,Zs);                        %field/source     

        tmp=0;
        for ik=1:NG
            tmp=tmp+f(1:nf,1:ns,ik)*A(ik);  %     tmp=tmp+1*A(ik);
        end
        L=0.5*du/alph_f*tmp;
    case 2
% (b) Y-current
        f=G_T2SPy2(X,Y,zf,Xs,Ys,Zs);                                  %field/source     

        tmp=0;
        for ik=1:NG
            tmp=tmp+f(1:nf,1:ns,ik)*A(ik);  %     tmp=tmp+1*A(ik);
        end
        L=0.5*du/alph_f*tmp;
end
end