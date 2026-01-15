function int = int_fila_a_anal_c(ps1,ps2,ls, pf1,pf2,lf)
% have sigular point in some cases, need to be fixed.

%  Function:       int_line_a_anal
%  Description:    Calculate intergral between two arbitrary lines according
%                  Inductance Calculations: working formulas and tables
%                  p.56
%  Calls:
%  Input:          ps1  --  coordinate of start point of source line
%                  ps2  --  coordinate of end point of source line
%                  l1   --  length of source line
%                  pf1  --  coordinate of start point of field line
%                  pf2  --  coordinate of end point of field line
%                  l2   --  length of field line
%  Output:         int  --  integral result
%  Others:         1 source line and multi- field linesis are supported
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-13

r0 = 1e-10; % to avoid 0

Nf = length(lf);
OMG = zeros(Nf,1);

ls2 = ls.^2;
lf2 = lf.^2;


R12 = (ps2(:,1)-pf2(:,1)).^2 + (ps2(:,2)-pf2(:,2)).^2 + (ps2(:,3)-pf2(:,3)).^2;
R22 = (ps2(:,1)-pf1(:,1)).^2 + (ps2(:,2)-pf1(:,2)).^2 + (ps2(:,3)-pf1(:,3)).^2;
R32 = (ps1(:,1)-pf1(:,1)).^2 + (ps1(:,2)-pf1(:,2)).^2 + (ps1(:,3)-pf1(:,3)).^2;
R42 = (ps1(:,1)-pf2(:,1)).^2 + (ps1(:,2)-pf2(:,2)).^2 + (ps1(:,3)-pf2(:,3)).^2;

R1 = sqrt(R12);
R2 = sqrt(R22); 
R3 = sqrt(R32);
R4 = sqrt(R42);

a2 = (R42-R32+R22-R12);

cose =a2./(2*ls.*lf);
sine2 = 1-cose.^2;
sine = sqrt(sine2);

u = (ls.*( (2*lf2.*(R22-R32-ls2)+a2.*(R42-R32-lf2))./(4*ls2.*lf2-a2.*a2) ));

v = (lf.*( (2*ls2.*(R42-R32-lf2)+a2.*(R22-R32-ls2))./(4*ls2.*lf2-a2.*a2) ));

d2 = abs(R32-u.^2-v.^2+2*u.*v.*cose);
d = (sqrt(d2));


id_sp = d==0;    % lines in the same plane
id_dp = ~id_sp;  % lines in different planes

R1 = max(r0,R1);
R2 = max(r0,R2);
R3 = max(r0,R3);
R4 = max(r0,R4);


% lines in different planes
OMG(id_dp) = atan((d2(id_dp).*cose(id_dp)+(u(id_dp)+ls).*(v(id_dp)+lf(id_dp)).*sine2(id_dp))./(d(id_dp).*R1(id_dp).*sine(id_dp))) ...
    - atan((d2(id_dp).*cose(id_dp)+(u(id_dp)+ls).*v(id_dp).*sine2(id_dp))./(d(id_dp).*R2(id_dp).*sine(id_dp))) ...
    + atan((d2(id_dp).*cose(id_dp)+(u(id_dp).*v(id_dp)).*sine2(id_dp))./(d(id_dp).*R3(id_dp).*sine(id_dp))) ...
    - atan((d2(id_dp).*cose(id_dp)+u(id_dp).*(v(id_dp)+lf(id_dp)).*sine2(id_dp))./(d(id_dp).*R4(id_dp).*sine(id_dp)));


% int = cose.* ( 2*( (u+ls).*atanh(lf./(R1+R2)) + (v+lf).*atanh(ls./(R1+R4)) ...
%     - u.*atanh(lf./(R3+R4)) - v.*atanh(ls./(R2+R3)) ) - OMG.*d./sine) ;
int = ( 2*( (u+ls).*atanh(lf./(R1+R2)) + (v+lf).*atanh(ls./(R1+R4)) ...
    - u.*atanh(lf./(R3+R4)) - v.*atanh(ls./(R2+R3)) ) - OMG.*d./sine) ;

end


