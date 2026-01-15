function [U_rs,Er_rs,Ez_rs, U_l,Er_l,Ez_l, U_t,Er_t,Ez_t] = ...
    sr_induced_v_anal_L_RS(pt_hit,h_ch, pt_start,pt_end, i_sr,t_sr)
% equivalent to TL model
ep0 = 8.85*1e-12;
vc = 3e8;

vrs = 1.9e8;
vl = 1e7;

Nc = size(pt_start,1);
Nt = length(t_sr);

x_hit = pt_hit(:,1);
y_hit = pt_hit(:,2);

dt = t_sr(2)-t_sr(1);

if size(i_sr,2)==1
    i_sr = i_sr';
end


% distance between midpoint and channel segment in x-y plane
Rx = (pt_start(:,1)/2+pt_end(:,1)/2-x_hit);
Ry = (pt_start(:,2)/2+pt_end(:,2)/2-y_hit);
Rxy = sqrt( Rx.^2 + Ry.^2 ) ;

dx = pt_end(:,1)-pt_start(:,1);
dy = pt_end(:,2)-pt_start(:,2);
dz = pt_end(:,3)-pt_start(:,3);

%% 1. calculate the induced filed by return strokes
Ez_rs = zeros(Nc,Nt);
Er_rs = zeros(Nc,Nt);
U_rs = zeros(Nc,Nt);

v1 = vrs;
v2 = vrs;
v1rto = v1/vc;
v2rto = v2/vc;
for ik = 1:Nc

    n_td_tmp = floor( (t_sr - h_ch/v1 - Rxy(ik)/vc) /dt );
    id_t = n_td_tmp>0;
    
    h1 = v1rto * ( vc*t_sr(id_t)-sqrt((v1*t_sr(id_t)).^2+Rxy(ik).^2*(1-v1rto^2)) )/(1-v1rto^2);
    R12 = (h1.^2+Rxy(ik).^2);
    Ez_rs(ik,id_t) = i_sr(id_t)./(2*pi*ep0) .* ( (-t_sr(id_t).*h1+(2*h1.^2+Rxy(ik).^2)./v1)./(R12.^(1.5)) ...
        - 1./(Rxy(ik).*v1) - Rxy(ik).^2./(vc^2*R12.^(1.5).*(1/v1+h1/vc./sqrt(R12))) );
    
    
    zmid = (pt_start(ik,3)+pt_end(ik,3))/2;
    n_td_tmp = floor( (t_sr - h_ch/v2 - sqrt(Rxy(ik)^2+zmid^2)/vc) /dt );
    id_t = n_td_tmp>0;

    h2 = v2rto * ( vc*t_sr(id_t) - v2rto*zmid - sqrt((v1*t_sr(id_t)-zmid).^2+Rxy(ik).^2*(1-v1rto^2)) ) ...
        /(1-v1rto^2);
    Rxyx2 = Rxy(ik).^2+zmid.^2;
    Er_rs(ik,id_t) = i_sr(id_t)./(4*pi*ep0) .* ( 2*zmid./(Rxy(ik).*v2)./sqrt(Rxyx2) ...
        + (Rxy(ik).*t_sr(id_t)-Rxy(ik)*zmid/v2-(zmid-h2).^3/(Rxy(ik)*v2))./((zmid-h2).^2+Rxy(ik).^2).^(1.5) ...
        - (Rxy(ik).*t_sr(id_t)+Rxy(ik)*zmid/v2+(zmid+h2).^3/(Rxy(ik)*v2))./((zmid+h2).^2+Rxy(ik).^2).^(1.5) ...
        + 2*Rxy(ik)*zmid./(vc^2*(h2.^2+Rxy(ik)^2).^(1.5).*(1/v2-(h2-zmid)/vc./sqrt((h2.^2+Rxy(ik)^2))) )  );

    
    U_rs(ik,:)= Er_rs(ik,:).*Rx(ik)/Rxy(ik)*abs(dx(ik)) ...
        + Er_rs(ik,:)*Ry(ik)/Rxy(ik)*abs(dy(ik)) + Ez_rs(ik,:).*abs(dz(ik));
end


%% 2. calculate the induced filed by leader
Ez_l = zeros(Nc,Nt);
Er_l = zeros(Nc,Nt);
U_l = zeros(Nc,Nt);

v1 = vl;
v2 = vl;
v1rto = v1/vc;
v2rto = v2/vc;
for ik = 1:Nc

    n_td_tmp = floor( (t_sr - h_ch/v1 - Rxy(ik)/vc) /dt );
    id_t = n_td_tmp>0;
    
    h1 = v1rto * ( vc*t_sr(id_t)-sqrt((v1*t_sr(id_t)).^2+Rxy(ik).^2*(1-v1rto^2)) )/(1-v1rto^2);
    R12 = (h1.^2+Rxy(ik).^2);
    Ez_l(ik,id_t) = i_sr(id_t)./(2*pi*ep0) .* ( (-t_sr(id_t).*h1+(2*h1.^2+Rxy(ik).^2)./v1)./(R12.^(1.5)) ...
        - 1./(Rxy(ik).*v1) - Rxy(ik).^2./(vc^2*R12.^(1.5).*(1/v1+h1/vc./sqrt(R12))) );
    
    
    zmid = (pt_start(ik,3)+pt_end(ik,3))/2;
    n_td_tmp = floor( (t_sr - h_ch/v2 - sqrt(Rxy(ik)^2+zmid^2)/vc) /dt );
    id_t = n_td_tmp>0;

    h2 = v2rto * ( vc*t_sr(id_t) - v2rto*zmid - sqrt((v1*t_sr(id_t)-zmid).^2+Rxy(ik).^2*(1-v1rto^2)) ) ...
        /(1-v1rto^2);
    Rxyx2 = Rxy(ik).^2+zmid.^2;
    Er_l(ik,id_t) = i_sr(id_t)./(4*pi*ep0) .* ( 2*zmid./(Rxy(ik).*v2)./sqrt(Rxyx2) ...
        + (Rxy(ik).*t_sr(id_t)-Rxy(ik)*zmid/v2-(zmid-h2).^3/(Rxy(ik)*v2))./((zmid-h2).^2+Rxy(ik).^2).^(1.5) ...
        - (Rxy(ik).*t_sr(id_t)+Rxy(ik)*zmid/v2+(zmid+h2).^3/(Rxy(ik)*v2))./((zmid+h2).^2+Rxy(ik).^2).^(1.5) ...
        + 2*Rxy(ik)*zmid./(vc^2*(h2.^2+Rxy(ik)^2).^(1.5).*(1/v2-(h2-zmid)/vc./sqrt((h2.^2+Rxy(ik)^2))) )  );

    
    U_l(ik,:)= Er_l(ik,:).*Rx(ik)/Rxy(ik)*(dx(ik)) ...
        + Er_l(ik,:)*Ry(ik)/Rxy(ik)*(dy(ik)) + Ez_l(ik,:).*(dz(ik));
end


%% 3. calculate the total induced field
U_t = U_rs - U_l;
Er_t = Er_rs + Er_l;
Ez_t = Ez_rs + Ez_l;




