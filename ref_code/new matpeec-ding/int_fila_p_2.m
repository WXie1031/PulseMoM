function int = int_fila_p_2(ps1, ps2, dv1, r1, pf1, pf2, dv2, r2)
%  Function:       int_line_p
%  Description:    Calculate intergral between two parallel lines according
%                  Inductance Loop and Partial Ch5
%  Calls:
%  Input:          ps1  --  coordinate of start point of source line
%                  ps2  --  coordinate of end point of source line
%                  dv1  --  direction vector of source line
%                  l1   --  length of source line
%                  pf1  --  coordinate of start point of field line
%                  pf2  --  coordinate of end point of field line
%                  dv2  --  direction vector of field line
%                  l2   --  length of field line
%  Output:         int  --  integral result
%  Others:         1 source line and multi- field linesis are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-12-13
% ps1=[0 0 0];
% ps2=[10 0 0];
% dv1=[1 0 0];
% r1=0.01;
% pf1=[0 1 0;0 0 1];
% pf2=[10 1 0;10 0 1];
% dv2=[1 0 0;1 0 0];
% r2=[0.01 0.01];



Nf = size(pf1,1);

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
% for ia=1:Nf

ps10=ps1;
ps20=ps2;

for ia=1:Nf
    
        
    err1=dot(dv1,[0 0 1]);

err2=dot(dv2(ia,:),[0 0 1]);
if abs(err1)==1 || abs(err2)==1
    
else
rota=cross(dv1,[0 0 1]);
t=acos(dot(dv1,[0 0 1]));
M = makehgtform('axisrotate',rota,t);

% dv10=zeros(4,1);
% dv20=zeros(4,1);
% rotate to parallel to x-axis.
% dv10(1:3)=dv1;
% dv20(1:3)=dv2;


% ps11=(M*[ps1 0]')';
pf11(ia,:)=(M*[pf1(ia,:) 0]')';
% ps22=(M*[ps2 0]')';
pf22(ia,:)=(M*[pf2(ia,:) 0]')';


% ps1=ps11(1,1:3);
pf1(ia,:)=pf11(ia,1:3);
% ps2=ps22(1,1:3);
pf2(ia,:)=pf22(ia,1:3);
ps1=(M*[ps10(1,:) 0]')';
ps2=(M*[ps20(1,:) 0]')';
end

end




%% 3. calculation
s13 = zeros(Nf,1);
s14 = zeros(Nf,1);
s23 = zeros(Nf,1);
s24 = zeros(Nf,1);
s13(1:Nf) = (ps1(3)-pf1(:,3));
s14(1:Nf) = (ps1(3)-pf2(:,3));
s23(1:Nf) = (ps2(3)-pf1(:,3));
s24(1:Nf) = (ps2(3)-pf2(:,3));

r0 = zeros(Nf,1);
r02 = zeros(Nf,1);
r0(1:Nf) = max(r1,r2);
r02(1:Nf) = r0.*r0;

d2 = zeros(Nf,1);
% d2(1:Nf) = max( (ps1(:,1)-pf1(:,1)).^2+(ps1(:,2)-pf1(:,2)).^2, r02);
d2(1:Nf) = max( (ps1(:,1)-pf1(:,1)).^2+(ps1(:,2)-pf1(:,2)).^2, r02);

R13 = zeros(Nf,1);
R23 = zeros(Nf,1);
R14 = zeros(Nf,1);
R24 = zeros(Nf,1);
R13(1:Nf) = sqrt(d2 + s13.*s13);
R23(1:Nf) = sqrt(d2 + s23.*s23);
R14(1:Nf) = sqrt(d2 + s14.*s14);
R24(1:Nf) = sqrt(d2 + s24.*s24);

% using the exact formulas for calculation
I1 = zeros(Nf,1);
I2 = zeros(Nf,1);
I1(1:Nf) = - s13.*log(s13+R13) + s23.*log(s23+R23) + s14.*log(s14+R14) - s24.*log(s24+R24);
I2(1:Nf) = R13 - R23 - R14 + R24;

int = zeros(Nf,1);
% int(1:Nf) = dv_sign.*(I1 + I2);
int(1:Nf) = I1 + I2;


end

