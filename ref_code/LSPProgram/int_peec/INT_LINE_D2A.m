% INT_LINE_D2A double-line integration along direction u and u'(free space)
%              (1) Close-form Formula for the integral of a function (1st)
%                        1
%                 F(z)= -------------------------------- over [u1 u2]
%                        sqrt(x^2+y^2+z^2)
%              (2) numerical integration using 1/2/3/5-point method(2nd)            
%
% DU(:,1)=[u1 u2]   vectors: lower and upper coodinates of u for interval
% DR(:,1)=[R1 R2]   vectors: relative dis. from obj. pt to two sour. pts
% r0(:)             radius of all segments in source wires
% num               [N ns no], N = # of pts on obs. segment for integratino 
% A0                coef. of Gauss–Legendre quadrature
%
% updated on April 2, 2013
%
function INT0=INT_LINE_D2A(num,DR,DU,Lo,A0)
% [t A]=GAUSS_LEGEN_CF(N0);  % get Gauss coef.
ELIM=1e-7;                  % limit of R-U (l=300,h=0.01)
No=num(1);  Ns=num(3); 
no=num(2);  ns=num(4);  nt=no*No;  

% (1) 1st integral result using the close-form formula (ns*no*N)
INT1=0;
lr0=1:nt;                  % range for all lower s. points
lr1=1:ns;
for ik=1:Ns
    lr2=lr1+ns;            % range for all upper s. points       
    R1=DR(lr0,lr1);
    R2=DR(lr0,lr2);
    U1=DU(lr0,lr1);        % for image under the ground
    U2=DU(lr0,lr2);        % for image under the ground
    tmp1=log((R2-U2)./(R1-U1));

    s=R1-U1;                % update NaN items in TMP with log(R1/R2)
    Itmp=s<ELIM;            % find the index of s<ELIM
    if sum(sum(Itmp))~=0
        tmp2=log((R1+U1)./(R2+U2));
        tmp1(Itmp)=tmp2(Itmp);  % update the NaN item
    end
    
    INT1=INT1+tmp1;
    lr1=lr2;
    
end
clear DR0 R1 R2 U1 U2;

% (2) (ns x no) 2nd integration: Gauss–Legendre quadrature
INT0=0;
lr1=1:no;
for ik=1:No
    INT0=INT0+A0(ik)*INT1(lr1,:);
    lr1=lr1+no;
end
LL=repmat(Lo,1,ns);
INT0=INT0.*LL/2;
end

