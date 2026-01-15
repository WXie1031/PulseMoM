% COEF_3D_PLAT      Return resistance and inductance matrix of cell/source

function [Rx Ry Lxc Lyc Lxs Lys]=COEF_3D_PLAT(Freq,Plat,Sour)
Rx=0; Ry=0; Lxc=0; Lyc=0; Lxs=0; Lys=0;

% (1) General constant (f and mu0)
mu0=4*pi*1e-7;
NF=length(Freq);

if Freq(1)<0
    for ik=1:NF
        f=-Freq(ik);
        Plat.alph=-(1+1i)*sqrt(pi*f*mu0*Plat.sigm); % update alpha
        [Rxp Rxn Ryp Ryn]=T_RES_CELL(Plat);
        n=length(Rxp);
        Rx(1:n,ik)=0.5*(Rxp+Rxn);
        Ry(1:n,ik)=0.5*(Ryp+Ryn);
    end
    return;
end
    
for ik=1:NF
    f=Freq(ik);
    Plat.alph=-(1+1i)*sqrt(pi*f*mu0*Plat.sigm); % update alpha
   
    % [R S L]=test(Plat,Sour);

% (5) Cal circuit parameters
[Rxp Rxn Ryp Ryn]=T_RES_CELL(Plat);
ns=length(Rxp);  ND=1:ns;                    % size of the impedance matrix
[Lxp Lxn Lyp Lyn]=T_IND_CELL(Plat);
[Lxps Lxns Lyps Lyns]=T_IND_SOUR(Sour,Plat);

    if NF==1
        Rx=diag(0.5*(Rxp+Rxn));
        Ry=diag(0.5*(Ryp+Ryn));
     
        Lxc=real(0.5*(Lxp+Lxn));
        Lyc=real(0.5*(Lyp+Lyn));
    
        Lxs=0.5*real(Lxps+Lxns);    
        Lys=0.5*real(Lyps+Lyns);  
    else
        Rx(ND,ND,ik)=diag(0.5*(Rxp+Rxn));
        Ry(ND,ND,ik)=diag(0.5*(Ryp+Ryn));
    
        Lxc(ND,ND,ik)=real(0.5*(Lxp+Lxn));
        Lyc(ND,ND,ik)=real(0.5*(Lyp+Lyn));
    
        Lxs(ND,ND,ik)=0.5*real(Lxps+Lxns);    
        Lys(ND,ND,ik)=0.5*real(Lyps+Lyns);  
    end
end
end