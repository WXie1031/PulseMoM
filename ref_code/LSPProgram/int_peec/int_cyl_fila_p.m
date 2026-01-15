function int = int_cyl_fila_p(ps1, ps2, dv1, r1, pf1, pf2, dv2, Nint)
%  Function:       int_cyl_fila_p
%  Description:    Calculate intergral between a cylinder and a filament in
%                  parallel lines according to Zhou's formulas
%  Calls:
%  Input:          ps1  --  coordinate of start point of source line
%                  ps2  --  coordinate of end point of source line
%                  dv1  --  direction vector of source line
%                  l1   --  length of source line
%                  pf1  --  coordinate of start point of field line
%                  pf2  --  coordinate of end point of field line
%                  dv2  --  direction vector of field line
%                  l2   --  length of field line
%                  n    --  order of the guass integration
%  Output:         int  --  integral result
%  Others:         1 source line and multi- field linesis are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2018-03-22

Nf = size(pf1,1);

[TT, AA] = gauss_int_coef(Nint);
a = (2*pi)/2*ones(1,Nint) + (2*pi)/2*TT;


% change to vectors for speeding
a = ones(Nf,1)*a;
AA = ones(Nf,1)*AA;


%% 1.check if the direction is the same or oppsite
% dv_sign = ones(Nf,1);
% for ik = 1:Nf
%     if sum( abs(dv2(ik,1:3) + dv1) )<1e-3
%         dv_sign(ik) = -1;
%     end
% end

%% 2.rotate to parallel to z-axis.
% zdv = [0 0 1];
% wv = cross(dv1,zdv);
% sina = norm(wv)/(norm(dv1)*norm(zdv));
% cosa = dot(dv1,zdv)/(norm(dv1)*norm(zdv));
% W = [0  -wv(3)  wv(2);  wv(3)  0  -wv(1);  -wv(2)  wv(1)  0;];
% M = cosa*eye(3) + (1-cosa)*kron(wv',wv) + sina*W;
% 
% poffset = ps1;
% 
% ps1 = ps1-poffset;
% ps2 = ps2-poffset;
% 
% for ik=1:Nf
%     pf1(ik,1:3) = pf1(ik,1:3)-poffset;
%     pf2(ik,1:3) = pf2(ik,1:3)-poffset;
% end
% 
% ps1 = ps1*M;
% ps2 = ps2*M;
% pf1 = pf1*M;
% pf2 = pf2*M;


ps10 = ps1;
ps20 = ps2;
pf11 = zeros(Nf,3);
pf22 = zeros(Nf,3);
for ik = 1:Nf

    err1=dot(dv1,[0 0 1]);
    
    err2=dot(dv2(ik,:),[0 0 1]);
    if abs(err1)==1 || abs(err2)==1
        
    else
        rota=cross(dv1,[0 0 1]);
        t=acos(dot(dv1,[0 0 1]));
        M = makehgtform('axisrotate',rota,t);

        pf11(ik,:)=(M*[pf1(ik,:) 0]')';
        pf22(ik,:)=(M*[pf2(ik,:) 0]')';

        pf1(ik,:)=pf11(ik,1:3);
        pf2(ik,:)=pf22(ik,1:3);
        ps1=(M*[ps10(1,:) 0]')';
        ps2=(M*[ps20(1,:) 0]')';
    end
    
end



%% 3. calculation
s13 = zeros(Nf,Nint);
s14 = zeros(Nf,Nint);
s23 = zeros(Nf,Nint);
s24 = zeros(Nf,Nint);
s13(1:Nf,:) = (ps1(1,3)-pf1(:,3))*ones(1,Nint);
s14(1:Nf,:) = (ps1(1,3)-pf2(:,3))*ones(1,Nint);
s23(1:Nf,:) = (ps2(1,3)-pf1(:,3))*ones(1,Nint);
s24(1:Nf,:) = (ps2(1,3)-pf2(:,3))*ones(1,Nint);


r12 = r1.*r1;

d2 = (ps1(:,1)-pf1(:,1)).^2+(ps1(:,2)-pf1(:,2)).^2;
d2 = d2*ones(1,Nint);
d = sqrt(d2);

R13 = zeros(Nf,Nint);
R23 = zeros(Nf,Nint);
R14 = zeros(Nf,Nint);
R24 = zeros(Nf,Nint);
D2 = r12 + d2 - 2*d.*r1.*cos(a);
R13(1:Nf,:) = sqrt(s13.*s13 + D2);
R23(1:Nf,:) = sqrt(s23.*s23 + D2);
R14(1:Nf,:) = sqrt(s14.*s14 + D2);
R24(1:Nf,:) = sqrt(s24.*s24 + D2);

% using the exact formulas for calculation
I1 = zeros(Nf,Nint);
I2 = zeros(Nf,Nint);
I1(1:Nf,:) = - s13.*log(s13+R13) + s23.*log(s23+R23) ...
    + s14.*log(s14+R14) - s24.*log(s24+R24);
I2(1:Nf,:) = R13 - R23 - R14 + R24;

int = zeros(Nf,1);
% int(1:Nf) = dv_sign.*(I1 + I2);
% numerical integration -> 2*pi/2 .* sum(AA.*(I1+I2),2)
int(1:Nf) = 2*pi/2.*sum(AA.*(I1+I2),2) * r1;


end

