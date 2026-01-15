function [Rfinal, Lfinal, Rfit, Lfit] = update_ground_to_main_interp(...
    Rmtx, Lmtx, Rself, Lself, Rg, Lg, Rgself, Lgself, freq, Nfit)
%  Function:       update_ground_to_main
%  Description:    Add the ground effect to the parameter matrix. Vector
%                  fitting will be employed to generate the final
%                  parameters.
%                  Ground effect matrix Rg and Lg is calculated on a
%                  specified frequency to correct the mutual parameters.
%                  Self parameter part will be calculated based on a
%                  separate frequency dependent Rgself and Lgself.
%                  Vector fitting will be employed after parameter update.
%  Calls:          vecfit_main_Z
%  Input:          Rmtx    --  R matrix
%                  Lmtx    --  L matrix
%                  Rself   --  self R matrix of different frequency (Nc*Nf)
%                  Lself   --  self L matrix of different frequency (Nc*Nf)
%                  Rg      --  ground effect R matrix
%                  Lg      --  ground effect L matrix
%                  Rself   --  self R matrix of different frequency (Nc*Nf)
%                  Lself   --  self L matrix of different frequency (Nc*Nf)
%                  Nfit    --  num. of fitting points
%  Output:         Rfinal  --  final R matrix
%                  Lmtx    --  final L matrix
%                  Rfit    --  self R matrix of fitted result (Nc*Nfit)
%                  Lself   --  self L matrix of fitted result (Nc*Nfit)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-06-25


%% 1. mutual parameter update
if ~isempty(Lg)
    Rfinal = Rmtx + Rg;
    Lfinal = Lmtx + Lg;
    
    if ~isempty(Lself)
        Rself = Rself + Rgself;
        Lself = Lself + Lgself;
    end
else
    Rfinal = Rmtx;
    Lfinal = Lmtx;
end


%% 2. self parameter update with vector fitting
Nc = size(Rmtx,1);

Rfit = zeros(Nc,10);
Lfit = zeros(Nc,10);

if Nfit>10
    disp('Number of vector fitting points should not exceed 10.');
    return;
end



fi = [ 1 5 (10:10:100) (150:50:500) 750 (1e3:1e3:5e3) (10e3:10e3:100e3) 200e3 300e3 500e3 750e3 1e6 1.5e6 2e6];
Nfi = length(fi);
Rsi = zeros(Nc,Nfi);
Lsi = zeros(Nc,Nfi);

for ig = 1:Nc
    Rsi(ig,:) = interp1(freq,squeeze(Rself(ig,:)),fi,'linear');%spline linear
    Lsi(ig,:) = interp1(freq,squeeze(Lself(ig,:)),fi,'linear');
end



for ik = 1:Nc
    
    Zs = zeros(1,1,Nfi);
    Zs(1,1,:) = Rsi(ik,:) + 1j*2*pi*fi.*Lsi(ik,:);
    
    
    [R0, L0, Rn, Ln] = vecfit_main_Z(Zs, fi, Nfit);
    Rfinal(ik,ik) = R0-sum(Rn);
    Lfinal(ik,ik) = L0;
    Rfit(ik,1:Nfit) = Rn;
    Lfit(ik,1:Nfit) = Ln;
    
    Lfinal(ik,1:Nc) = min(Lfinal(ik,1:Nc),Lfinal(ik,ik));
    Lfinal(1:Nc,ik) = min(Lfinal(1:Nc,ik),Lfinal(ik,ik));
end



