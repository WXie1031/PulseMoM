% MNA_SOLVER   solve the matrix of the circuit using MNA method
% %%%%%% INPUT %%%%%%%%
% f0            specific frequency
% R             resistance matrix
% L             inductance matrix (retarding time is calculated within the L)
% A             connection matrix of the circuit network
% Vs            voltage source adding to the network
% Is            current source adding to the network
% %%%%% OUTPUT %%%%%%%%
% Vo            output of node voltage
% Io            output of branch current
% %%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.3
function [Ibt, Vnt] = sol_peec_time_ssf_td( Rpeec,Lpeec,Cs,Ss, ...
    Rec,Lec,Cec, id_TdL,id_TdP, Amtx, id_sv,id_si, sr_t, dt, Nt, p_flag )

sr_max = max(abs(sr_t));
[Nn, Nb] = size(Amtx);

id_si = full(id_si);
id_sv = full(id_sv);

Nb_peec = size(Rpeec,1);

Nb_ec_r = size(Rec,1);
NtdL = max(max(id_TdL));
if p_flag==0
    NtdP = 0;
    Nn_peec = 0;
else
    NtdP = max(max(id_TdP));
    Nn_peec = size(Cs,1);
end

Ntd = max(NtdL,NtdP);

ist = [ zeros(1,Ntd) sr_t ];
% vst = [ zeros(1,Ntd) sr_t ];



%% Separate the Retarded Terms and No Retarded Terms into 2 Groups
Lnr = zeros(Nb_peec,Nb_peec);
Lr = zeros(Nb_peec,Nb_peec);
id_Lnr = id_TdL == 0;
id_Lr = ~id_Lnr;
Lnr(id_Lnr) = Lpeec(id_Lnr);
Lr(id_Lr) = Lpeec(id_Lr);

if p_flag>0
    Snr = zeros(Nn_peec,Nn_peec);
    Sr = zeros(Nn_peec,Nn_peec);
    id_Snr = id_TdP == 0;
    id_Sr = ~id_Snr;
    Snr(id_Snr) = Ss(id_Snr);
    Sr(id_Sr) = Ss(id_Sr);
else
    Snr = [];
    Sr = [];
end

% dLec = Lec./dt;
% dCec = Cec./dt;
% Zleft = blkdiag( (Rpeec+Lnr), diag(Rec), diag(dLec) );
Cleft = blkdiag(Cs, diag(Cec));
Lleft = blkdiag(Lnr, diag(Lec));
clear id_Lnr id_Lr id_Snr id_Sr


%% %%%%%%% Backward Euler Method for Solving the Network %%%%%%%%%

tt = (Nt-1)*dt;

td_vec = unique( [unique(id_TdL);unique(id_TdP)] );
td_vec(td_vec==0) = [];
Ntvec = length(td_vec);
Ai = zeros(Nn+Nb,Nn+Nb, Ntvec);
Bi = zeros(Nn+Nb,Nn+Nb, Ntvec);
Gi = zeros(Nn+Nb,Nn+Nb, Ntvec);
ui = zeros(Nn+Nb,1,Ntvec);

Aleft = inv([zeros(Nn,Nb) -Lleft; Cleft  zeros(Nb,Nn)]);
A0 = [Amtx' blkdiag(Rpeec, diag(Rec));
    -Snr  Snr*Amtx];
G0 = [eye(Nb) zeros(Nb,Nn); zeros(Nn,Nb) Snr];

out = zeros(Nn+Nb,Nt+Ntd);
for ig = 1:Ntvec
    
    ind_Lr = id_TdL==td_vec(ig);
    ind_Sr = id_TdP==td_vec(ig);
    
    Srtmp = zeros(Nn,Nn);
    Srtmp(ind_Sr) = Sr(ind_Sr);
    Lrtmp = zeros(Nb,Nb);
    Lrtmp(ind_Lr) = Lr(ind_Lr);
    
    Ai(:,:,ig) = [zeros(Nb,Nb+Nn); Srtmp  Srtmp*Amtx];
    Bi(:,:,ig) = [zeros(Nb,Nn)  Lrtmp; zeros(Nn,Nn+Nb)];
    Gi(:,:,ig) = [zeros(Nb,Nb+Nn); zeros(Nn,Nn)  Srtmp];
end


cnt = 1;
disp(['Total Simulation Time: ', num2str(tt*1e6,'%.2f'),' us'])
disp('Simulation Start')
for ik = 2:Nt
    
    if ik == ceil(Nt*(10*cnt/100))
        disp(['Simulation Complete: ', num2str(10*cnt,'%d'),'% ...'])
        cnt = cnt +1;
    end
    
    u0 = [id_sv.*sr_t(ik); id_si.*sr_t(ik)];
    
    % first order backward Eular
    sr_const = 0;
    x_right = 0;
    for ig = 1:Ntvec
        
        x_right = x_right + Ai(:,:,ig)*out(:,Ntd+ik-td_vec(ig)) + ...
            + Bi(:,:,ig)*(out(:,Ntd+ik-td_vec(ig))-out(:,Ntd+ik-td_vec(ig)-1))./dt;
        
        if ik>td_vec(ig)
            ui(:,1,ig) = [zeros(Nb,1); id_si.*ist(Ntd + ik - td_vec(ig))];
            sr_const = sr_const + Gi(:,:,ig)*ui(:,1,ig);
        end
    end
    sr_const = G0*u0 + sr_const;
    
    out(:,Ntd+ik) = (1/dt*eye(Nn+Nb) - Aleft*A0) \ ( out(:,Ntd+ik-1)./dt + Aleft*(x_right+sr_const));
    
    % second order backward Eular
    
    
    
    if find(abs(out(:,Ntd+ik))>sr_max*1e6)
        disp('The result is damping');
        break;
    elseif isnan(out)
        disp('The result is sigular');
        break;
    end
    
    %     sol = ddensd(@(t,y,ydel,ypdel) ddefun(t,y,ydel,ypdel, A0,Ai,Bi,sr_const,Ntd), ...
    %         @(t,y) dely(t,y,td_vec), @(t,y) delyp(t,y,td_vec), ...
    %         @(t) hist(t, tt, sr_t, id_sv,id_si),[0 max(tt)]);
    %     out = deval(sol,tt);
    
    
end

Ibt = out(Nn+1:Nn+Nb,Ntd+1:Ntd+Nt);
Vnt = out(1:Nn,Ntd+1:Ntd+Nt);


end


%
% function yp = ddefun(t, y, ydel, ypdel, A0, Ai, Bi, sr_const, Ntd)
% % Differential equations function for ddefun.
% yd_tmp = 0;
% ypd_tmp = 0;
% for ik = 1:Ntd
%     yd_tmp = yd_tmp + Ai(:,:,ik)*ydel(:,ik);
%     ypd_tmp = ypd_tmp + Bi(:,:,ik)*ypdel(:,:,ik);
% end
%
% yp = sr_const + A0*y +  yd_tmp + ypd_tmp;
%
% end
%
% function h = hist(t, tt, sr_t, id_sv,id_si)
% [~,indt] = min(t-tt);
% h = [id_sv.*sr_t(indt); id_si.*sr_t(indt)];
%
% %     sr_const = 0;
% %     for ig = 1:Ntd
% %         ui(:,ig) = [zeros(Nb,1); id_si.*ist(NtdP + ik - td_vec(ig))];
% %         sr_const = sr_const + Gi(:,:,ig)*ui(:,1,ig);
% %     end
% %
% %     sr_const = G0*u0 + sr_const;
%
% end
%
% function d = dely(t,y, td_vec)
%   d = [t-td_vec];
% end
%
% function d = delyp(t,y, td_vec)
%   d = [t-td_vec];
% end

