function [Uout, Er,Ez0] = sr_induced_v_anal_test(pt_hit,h_ch,Ns_ch, pt_start,pt_end, i_sr,t_sr, flag_type)

ep0 = 8.85*1e-12;
vc = 3e8;


if flag_type == 1  %% TL model
    v1 = 1.9e8;
    v2 = 1.9e8;
elseif flag_type == 2  %% MTLL
    v1 = 1.9e8;
    v2 = 1.9e8;
    H = 7e3;
else %% MTLE
    v1 = 1.9e8;
    v2 = 1.9e8;
    lamda = 2e3;  % constant in MTLE -- decays exponentially with the height
end

Nc = size(pt_start,1);
Nt = length(t_sr);

x_hit = pt_hit(:,1);
y_hit = pt_hit(:,2);

dz_ch = h_ch/Ns_ch;
z_ch = ((1:Ns_ch)-0.5)'*dz_ch; % mid point of the channel segment

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

Ez0 = zeros(Nc,Nt);
Er = zeros(Nc,Nt);
Uout = zeros(Nc,Nt);


v1rto = v1/vc;
v2rto = v2/vc;
for ik = 1:Nc

    zmid = (pt_start(ik,3)+pt_end(ik,3))/2;
    Rxyz = zeros(1,Ns_ch);
    
    for ig=1:Ns_ch
        if flag_type == 1  %% TL model
            cof_isr = 1;
        elseif flag_type == 2  %% MTLL
            cof_isr = (1 - z_ch(ig)/H);
        else %% MTLE
            cof_isr = exp(-z_ch(ig)/lamda);
        end
        
        Rxyz(ig) = sqrt( Rxy(ik)^2+(zmid-z_ch(ig))^2 );
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/v1+Rxyz(ig)/vc) )/dt );
        id_t = n_td_tmp>0;

        h1 = v1rto * ( vc*t_sr(id_t)-sqrt((v1*t_sr(id_t)).^2+Rxy(ik).^2*(1-v1rto^2)) )/(1-v1rto^2);
        R12 = (h1.^2+Rxy(ik).^2);
        Ez0(ik,id_t) = Ez0(ik,id_t) + cof_isr*i_sr(id_t)./(2*pi*ep0) ...
            .* ( (-t_sr(id_t).*h1+(2*h1.^2+Rxy(ik).^2)./v1)./(R12.^(1.5)) ...
            - 1./(Rxy(ik).*v1) - Rxy(ik).^2./(vc^2*R12.^(1.5).*(1/v1+h1/vc./sqrt(R12))) );
        
        
        n_td_tmp = floor( (t_sr - (z_ch(ig)/v2+Rxyz(ig)/vc) )/dt );
        id_t = n_td_tmp>0;

        h2 = v2rto * ( vc*t_sr(id_t) - v2rto*zmid - sqrt((v2*t_sr(id_t)-zmid).^2+Rxy(ik).^2*(1-v2rto^2)) ) ...
            /(1-v2rto^2);
        Rxyx2 = Rxy(ik).^2+zmid.^2;
        Er(ik,id_t) = Er(ik,id_t) + cof_isr*i_sr(id_t)./(4*pi*ep0) ...
            .* ( 2*zmid./(Rxy(ik).*v2)./sqrt(Rxyx2) ...
            + (Rxy(ik).*t_sr(id_t)-Rxy(ik)*zmid/v2-(zmid-h2).^3/(Rxy(ik)*v2))./((zmid-h2).^2+Rxy(ik).^2).^(1.5) ...
            - (Rxy(ik).*t_sr(id_t)+Rxy(ik)*zmid/v2+(zmid+h2).^3/(Rxy(ik)*v2))./((zmid+h2).^2+Rxy(ik).^2).^(1.5) ...
            + 2*Rxy(ik)*zmid./(vc^2*(h2.^2+Rxy(ik)^2).^(1.5).*(1/v2-(h2-zmid)/vc./sqrt((h2.^2+Rxy(ik)^2))) )  );
    end
    
    Uout(ik,:)= Er(ik,:).*Rx(ik)/Rxy(ik)*abs(dx(ik)) ...
        + Er(ik,:)*Ry(ik)/Rxy(ik)*abs(dy(ik)) + Ez0(ik,:).*abs(dz(ik));
end





