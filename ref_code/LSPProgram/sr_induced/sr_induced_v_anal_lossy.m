function [U_rs,Er_rs,Ez_rs,Ha0_rs] = sr_induced_v_anal_lossy(pt_hit,h_ch, ...
    pt_start,pt_end, i_sr,t_sr, soil_sig,soil_epr)
%  Function:       sr_induced_v_anal_lossy
%  Description:    Calculate induced voltage of all conductors using
%                  analytical model introduced in "Methods for calculating 
%                  the electromagnetic fields from a known source 
%                  distribution: Application to lightning". The model is 
%                  simplified by the assumption z=0. Lossy ground is
%                  added in this model.
%
%  Calls:          
%
%  Input:          pt_hit   --  lightning strike point in xy plane (1*2) (m)
%                  h_ch     --  height of lightning channel (m)
%                  pt_start --  start point of conductors (N*3) (m)
%                  pt_end   --  end point of conductors (N*3) (m)
%                  i_sr     --  lightning waveform
%                  t_sr     --  time sequeece (s)
%                  soil_sig --  resistivity of the soil
%                  soil_epr --  relative conductivity of the soil
%  Output:         U_rs  --  induced U on conductors
%                  Er_rs --  induced Er field on conductors
%                  Ez_rs --  induced Ez field on conductors
%                  Ha0_rs--  induced Ha field at z=0
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2017-01-17
% 

mu0 = 4*pi*1e-7;
ep0 = 8.85*1e-12;
vc = 3e8;

vrs = 1.3e8;

Nc = size(pt_start,1);
Nt = length(t_sr);

x_hit = pt_hit(:,1);
y_hit = pt_hit(:,2);

dt = t_sr(2)-t_sr(1);

if size(i_sr,2)==1
    i_sr = i_sr';
end

%% 1. calculate the E and H field for every conductor
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
Ha0_rs = zeros(Nc,Nt);
U_rs = zeros(Nc,Nt);

v1 = vrs;
v2 = vrs;
v1rto = v1/vc;
v2rto = v2/vc;


df = 1/max(t_sr);
w = 2*pi* (0:Nt-1)*df;
%ZHw = sqrt(1j*w*mu0./(1j*w*ep0*soil_epr+soil_sig));
ZHw = vc*mu0./sqrt(soil_epr+soil_sig./(1j*w*ep0));
ZHt = ifft(ZHw);

for ik = 1:Nc

    n_td_tmp = floor( (t_sr - h_ch/v1 - Rxy(ik)/vc) /dt );
    id_t = n_td_tmp>0;
    
    h1 = v1rto * ( vc*t_sr(id_t)-sqrt((v1*t_sr(id_t)).^2+Rxy(ik).^2*(1-v1rto^2)) )/(1-v1rto^2);
    R12 = (h1.^2+Rxy(ik).^2);
    Ez_rs(ik,id_t) = i_sr(id_t)./(2*pi*ep0) .* ( (-t_sr(id_t).*h1+(2*h1.^2+Rxy(ik).^2)./v1)./(R12.^(1.5)) ...
        - 1./(Rxy(ik).*v1) - Rxy(ik).^2./(vc^2*R12.^(1.5).*(1/v1+h1/vc./sqrt(R12))) );
    
    Ha0_rs(ik,id_t) = i_sr(id_t)./(2*pi) .* ( h1./sqrt(R12)./Rxy(ik) ...
        + Rxy(ik)./(vc/v1*R12+h1.*sqrt(R12)) );
    
    
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

end


%% 2. add the lossy ground usign convelution
% vector fitting is used to remove the sigularity in the direct convelution
Nfit = 9;
frq = [0.1 0.2 0.5 1 2 5 10 15 20 30 50 100 200 500 1e3 2e3 5e3 10e3 20e3 50e3 ...
    100e3 200e3 500e3 1e6 2e6 5e6 1e7 2e7 5e7];
Nf = length(frq);
Zin = zeros(1,1,Nf);
Zin(1,1,1:Nf) = vc*mu0./sqrt(soil_epr+soil_sig./(1i*2*pi*frq*ep0));
[R0,L0, Rn,Ln,~,~] = vecfit_kernel_Z(Zin, frq, Nfit, 0);

Hcov_cof=zeros(Nt,Nfit);
if ndims(Rn) == 3
    for ih=1:Nfit
        Hcov_cof(:,ih)=-Rn(1,1,ih)^2/Ln(1,1,ih)*exp(-Rn(1,1,ih)/Ln(1,1,ih).*(t_sr));
    end
elseif ndims(Rn) == 2
    for ih=1:Nfit
        Hcov_cof(:,ih) = -Rn(1,ih)^2/Ln(1,ih)*exp(-Rn(1,ih)/Ln(1,ih).*(t_sr));
    end
end
   
    
for ik = 1:Nc
    Htmp_div = zeros(1,Nt);
    Htmp_div(1) = Ha0_rs(ik,1)/dt;
    Htmp_div(2:Nt) = diff(Ha0_rs(ik,:))/dt;
    
    Hcov_vf = zeros(2*Nt-1,Nfit);
    for ig = 1:Nfit
        Hcov_vf(:,ig) = dt*conv(Ha0_rs(ik,1:Nt),Hcov_cof(1:Nt,ig));
    end
    Hcov_3 = sum(Hcov_vf,2)';

    Hcov_T = R0*Ha0_rs(ik,1:Nt) + L0*Htmp_div + Hcov_3(1:Nt);
    
    U_rs(ik,1:Nt)= (Er_rs(ik,1:Nt)-Hcov_T(1:Nt)) .* (Rx(ik)/Rxy(ik)*dx(ik) ...
        + Ry(ik)/Rxy(ik)*dy(ik)) + Ez_rs(ik,1:Nt)*dz(ik);

end


