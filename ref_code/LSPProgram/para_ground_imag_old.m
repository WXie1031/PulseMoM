% PEEC_M3_M_EXT Based on PEEC_M3_M, only external inductance is calculated
%            The self resistance and inductance are calculated outside
% PEEC_M3_M  Based on Dr.Du's prog 'PEEC_M3', changed for calculating
%            multiple wires' inductance and potential parameters
% PEEC_M3
%            Calculate s-domain parial inductance and potential of arbitray 
%            lines in air over the ground at a freq. using the image method
%            *--low frequency, low-height structure, without retadation--*
%
%            * Kernal--Scalar/vector potential from a hozrizontal section
%            (1) Gtt: complex image  (- negative contribution)
%            (2) Khp: perfect ground (- negative contribution)
%                    --Scalar/vector potential from a hozrizontal section
%            (3) Gzz: perfect ground (+ positive contribution)
%            (4) Kvp: perfect ground (- negative contribution)
%
%            * Partial inductance and coefficient
%            (1) length=m0/4/pi*int(int(Guv(u,v),u1,u2),v1,v2) (inner product)
%            (2) P=1/4/pi/e0*int(int(Kuv(u,v),u1,u2),v1,v2)/(s1*s2)  
% CALL -->   INT_LINE_M2A(N0,num,DD,Lo,Lss,Kxss,Kyss,Kzss,r0,A0);
%
% Po(:,6)  Cooridnates of observation lines (above the ground)
% Ps(:,6)  Cooridnates of source lines      (above the ground)
% r0, rs0  wire radus of field and source wires: max{r0, rs0} is used
% No Ns    N points (N-1 segments) in observation/source lines (2 3 5)
% R length P    Matrix for resistance, inductance and coef of potential(no x ns) 
% Lg,Pg    Matrix of image inductance and coef of potential (no x ns)
% mat      [FMHZ EPSG SIGG MURW];
% The o. line is divided into (N-1) segments or No points (No=1,2,3,5)
% The source line remains unchanged (integration by a close-form formula)
%
% Structure of field and source pts for integrtation
%       --------> source pts (starting to ending)
%       |  all sourc seg. pt 1 ... all sourc seg. pt 5     
%       |  all field seg. pt 1 
%       |  ...
%       V  all field seg. pt 5 
%
% Dec. 15  2011
% Po=Ps=[0 0 1 0 0 2; 0 0 2 0 0 3; 0 0 3 0 0 5];
% r0        radius vector
% Modified on 14 Apr,2012 by Michael Wang

function [Rg, Lg] = para_ground_imag(IBCdat_3D, ICBdat_3D, TCGdat_3D, GND) 

condG = GND.condG;
% general data
w0 = GND.freq*2*pi; 
mu0 = 4*pi*1e-7; 
cof0 = mu0/(4*pi); 


pt_end = vertcat(IBCdat_3D.pt_end, ICBdat_3D.pt_end, TCGdat_3D.pt_end);
pt_start = vertcat(IBCdat_3D.pt_start, ICBdat_3D.pt_start, TCGdat_3D.pt_start);
length = vertcat(IBCdat_3D.length, ICBdat_3D.length, TCGdat_3D.length);
%r_equivalent = vertcat(IBCdat_3D.r_equivalent, ICBdat_3D.r_equivalent, TCGdat_3D.r_equivalent);
direct_vector = vertcat(IBCdat_3D.direct_vector, ICBdat_3D.direct_vector, TCGdat_3D.direct_vector);

r0  = vertcat(IBCdat_3D.r_equivalent, ICBdat_3D.r_equivalent, TCGdat_3D.r_equivalent);
rs0 = vertcat(IBCdat_3D.r_equivalent, ICBdat_3D.r_equivalent, TCGdat_3D.r_equivalent);


Nsgs = 5; 
Nfgs = 5; % default Ns=2 No=3

TT=[-0.5773502692 0.5773502692 0 0 0;
    -0.7745966692 0 0.7745966692 0 0; 
    -0.8611363116 -0.3399810436 0.3399810436 0.8611363116 0; 
    -0.9061798459 -0.5384693101 0 0.5384693101 0.9061798459]; 
AA=[1 1 0 0 0;
    0.5555555556 0.8888888889 0.5555555556 0 0;
    0.3478548451 0.6521451549 0.6521451549 0.3478548451 0;
    0.2369263351 0.4786286705 0.5688888889 0.4786286705 0.2369263351];

T0 = TT(Nfgs-1,1:Nfgs);
A0 = AA(Nfgs-1,1:Nfgs);

Nf = size(pt_start,1);            % number of observation lines
Ns = size(pt_start,1);            % number of source lines        

num=[Nsgs Ns Nfgs Nf];
tn=Ns*Nf*Nsgs*Nfgs;             % total line segments including image segments
tnn=1:tn;


% (2) determine length of source and field lines
IXS = pt_end(1:Ns,1) - pt_start(1:Ns,1);            % X-increment of source line
IYS = pt_end(1:Ns,2) - pt_start(1:Ns,2);            % Y-increment of source line
IZS = pt_end(1:Ns,3) - pt_start(1:Ns,3);            % Z-increment of source line
IXO = pt_end(1:Nf,1) - pt_start(1:Nf,1);            % X-increment of field line
IYO = pt_end(1:Nf,2) - pt_start(1:Nf,2);            % Y-increment of field line
IZO = pt_end(1:Nf,3) - pt_start(1:Nf,3);            % Z-increment of field line

Ls = length(1:Ns,:);  % length of source line
Lf = length(1:Nf,:); % length of field line

KY = (IXO*IXS'+IYO*IYS'+IZO*IZS')./(Lf*Ls');  % theta: 3D angle (Matrix)

% direction number of line segments for source and field lines
% KXS=IXS./Ls;
% KYS=IYS./Ls;
% KZS=IZS./Ls;
% KXO=IXO./Lf;
% KYO=IYO./Lf;
% KZO=IZO./Lf;
KXS = direct_vector(1:Ns,1);
KYS = direct_vector(1:Ns,2);
KZS = direct_vector(1:Ns,3);
KXO = direct_vector(1:Nf,1);
KYO = direct_vector(1:Nf,2);
KZO = direct_vector(1:Nf,3);

KRS=sqrt(KXS.*KXS+KYS.*KYS);
KRO=sqrt(KXO.*KXO+KYO.*KYO);
% phi: angle between projection lines of source and field on xoy (tt) 
RS=max( sqrt(IXS.*IXS+IYS.*IYS), rs0.*rs0 );
RO=max( sqrt(IXO.*IXO+IYO.*IYO), r0.*r0 );
KP=(IXO*IXS'+IYO*IYS')./(RO*RS');
% Change the infinite small factor r0 into rs0, to avoid dimension mismatch
% of Matrix
% RS=sqrt(IXS.*IXS+IYS.*IYS+r0.*r0);

% TSIN=KZS;       TCOS=sqrt(1-TSIN.*TSIN);
% KT=(TCOS-TSIN)./(TCOS+TSIN);
% for ik=1:no
%     KT1(ik,1:ns)=KT;
% end
%clear IXS IYS IZS IXO IYO IZO RS RO TSIN TCOS KT

% (2a) formulating coordinates Qa of source segments & Qb of image under
% the perfect ground 
clear Q1 Q2 Qsa Qsb;
Q1 = pt_start(1:Ns,:);  
sls=Ls; 
if Nsgs~=1
    sls=Ls/(Nsgs-1); 
end            % length of s. segments

rr=[];
for ik=1:(Nsgs)
    for jk=1:Ns
        idx=(1:Nf*Nfgs)+(jk-1)*(Nf*Nfgs)+(ik-1)*(Nf*Nfgs*Ns);
        Qsa(idx,1)=Q1(jk,1);
        Qsa(idx,2)=Q1(jk,2);
        Qsa(idx,3)=Q1(jk,3);
        Kxss(idx)=KXS(jk);          % direction number of source image
        Kyss(idx)=KYS(jk);          % direction number of source image
        Kzss(idx)=KZS(jk);          % direction number of source image 
        Lss(idx)=Ls(jk);            % length of the whole wire for segment
        
        dis=abs(KY(1:Nf,jk)-1);
        rtmp0=max(r0,rs0(jk));       % radius of wires, max(rs0,r0)
        rtmp1=r0*0+rs0(jk);
        rtmp1(dis<1e-6)=rtmp0(dis<1e-6);
        for kk=1:Nfgs
            rr=[rr;rtmp1];           % radius for all field segments
        end
    end
    Q1(1:Ns,1)=Q1(1:Ns,1)+sls.*KXS; % linear division for integration
    Q1(1:Ns,2)=Q1(1:Ns,2)+sls.*KYS;
    Q1(1:Ns,3)=Q1(1:Ns,3)+sls.*KZS;
end
Qsb=Qsa;                            % Qsa source seg. in air (1->2)
Qsb(tnn,3)=-Qsa(tnn,3);             % Qsb imag seg. under ground (1->2)
clear rtmp0 rtmp1 rtmp2;

% (2b) formulating cooridnates of observation segments (in air)
Q1 = pt_start(1:Nf,:); 
Q2=[];
for ik=1:Nfgs
        slo=0.5*Lf*(T0(ik)+1);      % Gaussion devision for integration
        Q0(1:Nf,1)=Q1(1:Nf,1)+slo.*KXO; 
        Q0(1:Nf,2)=Q1(1:Nf,2)+slo.*KYO;
        Q0(1:Nf,3)=Q1(1:Nf,3)+slo.*KZO;
        Q2=[Q2;Q0]; 
end
Qo=[];
for ik=1:Ns*(Nsgs)
    Qo=[Qo;Q2];                     % repeat for sourc pts. (imag)
end
clear Q0 Q1 Q2;

% (3) determine the contribution from image
DD = Qo-Qsb;
N1=1;
G00=INT_LINE_M2A_M(N1,num,DD,Lf,Lss,Kxss,Kyss,-Kzss,rr,A0);    % image (Vert)

Lzz=KZO*(-KZS)'.*G00;              

CID=0;
if condG~=0
    CID=2/sqrt(1i*w0*mu0*condG);   % complex skin depth
    DD(tnn,3)=DD(tnn,3)+CID;
    N0=1;
    G00=INT_LINE_M2A_M(N0,num,DD,Lf,Lss,Kxss,Kyss,-Kzss,rr,A0); % image (Hori)
end

Ltt=KP.*(KRO*KRS').*G00;

Lt=real(Ltt);
Rb=imag(Ltt)*(w0);

Lb=(Lt+Lzz);                  % Ind. by imag
        
% (4) overall partial inductance and potnetial coeffecient
Rg = cof0*diag(diag(Rb));
Lg = cof0*(-Lb);


end



