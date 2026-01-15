% INT_LINE_M2A_M Based on Dr.Du's program INT_LINE_M2A
%               modified by change radius r0 from scalar to vector
% INT_LINE_M2A   double-line integration along direction w1 and w2
%               in free air with arbitray directions
%              (1) Close-form Formula for the integral of a function (1st)
%                        1
%                 F(z)= -------------------------------- over [w1 w2]
%                        sqrt(x^2+y^2+z^2)
%              (2) numerical integration using 1/2/3/5-formula (2nd)            
%
% num               [N, ns no]
% DD(:)             vectors of relative distance from obj. to sour.
% Lo(:)             interval of integration variable (u'2-u'1)
% Lss               length of source lines
% Kxss Kyss Kzss    directional numbers
% N0=1 or 0         1=yes (division Ns-1), 0=no (1 segment only)
%
% April 15 2012
% r1                radius vector with max values
% Modified on 16 Mar, 2012
function INT0=int_fila_a_num2(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2)

% order of numerical integration
NsIntOrder=2;
NfIntOrder=5; 

Ns = size(ps1,1);
Nf = size(pf1,1);

Nsall = Ns*NsIntOrder;   
Nfall = Nf*NfIntOrder;  

Nmtx = Ns*Nf;

%% 1. Guass numerical integration cofficients
[T0, A0] = gauss_int_coef(NfIntOrder);  % get Gauss coef.

%% 2. segment the source and field line
% 2.a mesh source line
Ps = zeros(Nsall*Nfall,3);
drx = zeros(Nsall*Nfall,1);
dry = zeros(Nsall*Nfall,1);
drz = zeros(Nsall*Nfall,1);
lss = zeros(Nsall*Nfall,1);
r0 = zeros(Nsall*Nfall,1);
Qtmp = ps1;

if NsIntOrder == 1
    sls = l1;
else
    sls = l1/(NsIntOrder-1);
end

cnt = 0;
for ik=1:NsIntOrder
    for ig = 1:Ns
        ind = (1:Nfall)+(ig-1)*Nfall+(ik-1)*Ns*Nfall;
        
        Ps(ind,1) = Qtmp(ig,1);     % coor of field lines after meshing
        Ps(ind,2) = Qtmp(ig,2);
        Ps(ind,3) = Qtmp(ig,3);
        drx(ind) = dv1(ig,1);
        dry(ind) = dv1(ig,2);
        drz(ind) = dv1(ig,3);
        
        rtmp = max(r2,r1(ig));
        for ih = 1:NfIntOrder
            r0((1:Nf)+cnt*Nf) = rtmp;
            cnt = cnt+1;
        end
        
        lss(ind) = l1(ig);
    end
    
    Qtmp(:,1) = Qtmp(:,1)+sls.*dv1(:,1);
    Qtmp(:,2) = Qtmp(:,2)+sls.*dv1(:,2);
    Qtmp(:,3) = Qtmp(:,3)+sls.*dv1(:,3);
end

% 2.b mesh field line
Po = zeros(Nfall*Nsall,3);
slo = zeros(Nf,1);
for ik=1:NfIntOrder
	slo(1:Nf) = 0.5*l2*(T0(ik)+1);    
  	% coor of field lines after meshing
  	Po((1:Nf)+(ik-1)*Nf,1) = pf1(:,1) + slo.*dv2(:,1);   
  	Po((1:Nf)+(ik-1)*Nf,2) = pf1(:,2) + slo.*dv2(:,2); 
  	Po((1:Nf)+(ik-1)*Nf,3) = pf1(:,3) + slo.*dv2(:,3);
end

for ik = 2:Nsall
   Po((1:Nfall)+(ik-1)*Nfall,:) = Po(1:Nfall,:); 
end

%% 3. calculation 
id0 = 1:(NsIntOrder-1)*Ns*Nfall;

dx = Po(id0,1)-Ps(id0,1);
dy = Po(id0,2)-Ps(id0,2);
dz = Po(id0,3)-Ps(id0,3);
DU = dx.*drx(id0)+dy.*dry(id0)+dz.*drz(id0);
DU2 = DU.*DU;
DR2 = max((dx.*dx+dy.*dy+dz.*dz-DU2), r0(id0).*r0(id0));


%lr1=1:(NsIntOrder-1)*Ns*NfIntOrder*Nf;   % range for all lower s. points
l1s=lss(id0)/(NsIntOrder-1);


% (1) 1st integral result using the close-form formula (ns*no*N)
c=l1s-DU;

INT1a=log((c+sqrt(c.*c+DR2))./(-DU+sqrt(DU2+DR2)));
INT1 = zeros(Nf*NfIntOrder*Ns,1);
for ik=1:NsIntOrder-1
    idx=(1:Nf*NfIntOrder*Ns)+(ik-1)*(Nf*NfIntOrder*Ns);
    INT1=INT1+INT1a(idx);
end

% (2) 2nd integral using N-point formulas (ns x no)
% (a) formulating f-matrix (no x ns, N)
f = zeros(Nmtx, NfIntOrder);

off=0;
for jk=1:Ns
    for ik=1:Nf
        idx=ik-1+(1:Nf:Nf*NfIntOrder)+off;
        f(ik+(jk-1)*Nf,1:NfIntOrder)=INT1(idx);
    end
    off=off+Nf*NfIntOrder;
end

len_all=[];
for ik=1:Ns
      len_all=[len_all;l2];            % h=Z2-Z1 (o. lines)
end


% (b) 2nd integration
Atmp = ones(Nmtx,1)*A0;
INT2 = sum(Atmp.*f,2).*len_all/2;


% (3) formulatign the output matrix (no x ns)

INT0 = zeros(Nf,Ns); 
for ik=1:Ns
    idx=Nf*(ik-1)+(1:Nf);
    INT0(1:Nf,ik)=INT2(idx);
end

cosa = dv1(:,1)'*dv2(:,1) + dv1(:,2)'*dv2(:,2) + dv1(:,3)'*dv2(:,3);
INT0 = INT0.*cosa;

end

