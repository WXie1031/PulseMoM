function [Rmtx, Lmtx, Cmtx] = update_ground_to_grid(...
    Rmtx, Lmtx, Cmtx,Rg, Lg, Cg,Rgself, Lgself, ver)
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
%  Output:         Rmtx    --  final R matrix
%                  Lmtx    --  final L matrix
%                  Rself   --  self R matrix of fitted result (Nc*Nfit)
%                  Lself   --  self L matrix of fitted result (Nc*Nfit)
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2014-06-25

% [temp_a temp_b]=size(Rgself);
% Rgself=zeros(temp_a,temp_b);
% Lgself=zeros(temp_a,temp_b);

% Rg=-Rg;
% Lg=-Lg;
% Cg=-Cg;

if nargin<9
    ver=1;
end
ver=1;
%% 1. mutual parameter update
if ~isempty(Lg)
    if ver==1
        Rmtx = Rmtx + diag(diag(Rg));
    elseif ver==2
        Rmtx = Rmtx + Rg;
    end
    1
    Lmtx = Lmtx + Lg;
    
    Nf = size(Lgself,2);
    
%     if ver>0
%         Rself(:,1:Nf) = Rself(:,1:Nf) + Rgself(:,1:Nf);
%     end
%     
%     Lself(:,1:Nf) = Lself(:,1:Nf) + Lgself(:,1:Nf);
    
end

% Pg=inv(Cg);
if isempty(Cmtx)==1
    
else
    Pmtx=inv(Cg)+inv(Cmtx);
    Cmtx=inv(Pmtx);
end





end



