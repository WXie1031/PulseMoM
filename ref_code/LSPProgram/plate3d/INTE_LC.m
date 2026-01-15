% INTE_LC       Perform 6-fold integration for Cell Inductance 
%               with Gauss-L integration overe z' and z, and 
%               4-fold integration with closed-form formula (T2TP)
% X=[x1 x2] Y=(y1 y2)  Z=(z1 z2) vectors -> L (matrix) (nf x ns) nf=ns
% INDEX         1='++', 2='-+' 3='+-' 4='--'
% Jsan. 2016

function L=INTE_LC(X,Y,Z,alpha,T,A,INDEX)
NG=length(A);
[n m]=size(X);

% (1) Level 1 integration (z' z)switch INDEX
switch INDEX
    case 1
        alph_s=+alpha;          ulim_s=exp(alph_s*Z);        % source
        alph_f=+conj(alpha);    ulim_f=exp(alph_f*Z);        % field
    case 2
        alph_s=+alpha;          ulim_s=exp(alph_s*Z);        % source
        alph_f=-conj(alpha);    ulim_f=exp(alph_f*Z);        % field
    case 3
        alph_s=-alpha;          ulim_s=exp(alph_s*Z);        % source
        alph_f=+conj(alpha);    ulim_f=exp(alph_f*Z);        % field
    case 4
        alph_s=-alpha;          ulim_s=exp(alph_s*Z);        % source
        alph_f=-conj(alpha);    ulim_f=exp(alph_f*Z);        % field
end    
du_s=ulim_s(1,2)-ulim_s(1,1);                                % source
du_f=ulim_f(1,2)-ulim_f(1,1);                                % field
ui_s=0.5*du_s*T+0.5*(ulim_s(1,2)+ulim_s(1,1));
ui_f=0.5*du_f*T+0.5*(ulim_f(1,2)+ulim_f(1,1));

zs= log(ui_s)/alph_s;
zf= log(ui_f)/alph_f;

f=G_T2TP(X,Y,zf,X,Y,zs);                                    % field/source     

tmp=0;
for ik=1:NG
    for jk=1:NG
        tmp=tmp+f(1:n,1:n,ik,jk)*A(ik)*A(jk);   % tmp=tmp+1*A(ik)*A(jk);
    end
end
L=0.25/(alph_s*alph_f)*(du_f*du_s).*tmp;
end