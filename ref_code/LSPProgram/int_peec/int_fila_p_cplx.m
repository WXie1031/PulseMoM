function int = int_fila_p_cplx(ps1, ps2, dv1, r1, pf1, pf2, dv2, r2)
%  Function:       int_line_p
%  Description:    Calculate intergral between two parallel lines according
%                  Inductance Loop and Partial Ch5
%  Calls:          angle_line
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
   
% ax = -acos(dv1(1));
% ay = -acos(dv1(2));
% az = 0;
% ps1 = ps1-ps1;
% ps2 = point_rot3D(ps2-ps1, ax, ay, az);
% 
% pf1(:,1) = pf1(:,1)-ps1(1);
% pf1(:,2) = pf1(:,2)-ps1(2);
% pf1(:,3) = pf1(:,3)-ps1(3);
% pf1 = point_rot3D(pf1, ax, ay, az);
% 
% pf2(:,1) = pf2(:,1)-ps1(1);
% pf2(:,2) = pf2(:,2)-ps1(2);
% pf2(:,3) = pf2(:,3)-ps1(3);
% pf2 = point_rot3D(pf2, ax, ay, az);

% check if the direction is the same or oppsite
ERR = 1e-3;

Nf = size(pf1,1);
dv_sign = complex(ones(Nf,1));
for ik = 1:Nf
    if sum( abs(dv2(ik,1:3) + dv1) )<ERR
        dv_sign(ik) = -1;
    end
end

zdv = [0 0 1];
wv = cross(dv1,zdv);
sina = norm(wv)/(norm(dv1)*norm(zdv));
cosa = dot(dv1,zdv)/(norm(dv1)*norm(zdv));
W = [0  -wv(3)  wv(2);  wv(3)  0  -wv(1);  -wv(2)  wv(1)  0;];

M = complex( cosa*eye(3) + (1-cosa)*kron(wv',wv) + sina*W );

ps1 = ps1*M;
ps2 = ps2*M;
pf1 = pf1*M;
pf2 = pf2*M;

s13 = (ps1(:,3)-pf1(:,3));
s14 = (ps1(:,3)-pf2(:,3));
s23 = (ps2(:,3)-pf1(:,3));
s24 = (ps2(:,3)-pf2(:,3));

r0 = max(r1,r2);
r02 = complex( r0.*r0 );
    
d0 = max( (ps1(:,1)-pf1(:,1)).^2+(ps1(:,2)-pf1(:,2)).^2, r02);


R13 = sqrt(d0 + s13.*s13); 
R23 = sqrt(d0 + s23.*s23);  
R14 = sqrt(d0 + s14.*s14);  
R24 = sqrt(d0 + s24.*s24);  

% using the exact formulas for calculation
I1 = - s13.*log(s13+R13) + s23.*log(s23+R23) + s14.*log(s14+R14) - s24.*log(s24+R24);
I2 = R13 - R23 - R14 + R24;

int = dv_sign.*(I1 + I2);

end

