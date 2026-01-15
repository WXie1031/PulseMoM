%% calculate the current in the frequency domain
% So      Source
function [Ibf, Vnf] = MAIN_SOLVER_F(Rall, Lall, Pall, Lg, Pg, Rec, Lec, Cec, Rcon, ...
   	TrLF, TrPF, TrLg, TrPg, NdP, NdGD, ID_SV, ID_SI, A, ...
    SRt, Tmax, Lflag, Pflag, Trflag, Gtype, SVtype)
    
   
global Nb  %Nb = size(BN,1);
global Nn  %Nn = length(NOD);  % total number of the nodes
global NbL % No. of Branches for Conductors
global NnP % No. of Nodes for Conductors

Nf = length(SRt);
df = 1/Tmax;

SRtF = fft(SRt);
if (rem(Nf,2)==0)
    Nfh = Nf/2+1;
else
    Nfh = (Nf+1)/2;
end

f = (0:Nf-1)*df; 
if f(1) == 0
 	f(1) = 0.1;
end

fh = f(1:Nfh);
w = 2*pi*fh;


%% Adding Ground Effect and Retarded Time
if Lflag > 0
  	if Trflag > 0
        if Gtype > 0
            for ik = 1:Nfh 
                Lall(:,:,ik) = Lall(:,:,ik).*exp(-1j*w(ik).*TrLF) ...
                    - Lg.*exp(-1j*w(ik).*TrLg);
            end
        else
            for ik = 1:Nfh
                Lall(:,:,ik) = Lall(:,:,ik).*exp(-1j*w(ik).*TrLF);
            end
        end
  	end
end
    

Pf = zeros(NnP,NnP,Nfh);
if Pflag > 0
    if Trflag > 0 && Gtype > 0
        for ik = 1:Nfh
        	Pf(:,:,ik) = Pall.*exp(-1j*w(ik).*TrPF) ...
              	- Pg.*exp(-1j*w(ik).*TrPg);
        end
 	elseif Trflag > 0 && Gtype == 0 
      	Cs = diag(1./diag(Pall));
        Sstmp = zeros(NnP,NnP);
       	for ig = 1:NnP
           	Sstmp(:,ig) = Pall(:,ig)./Pall(ig,ig);
        end
        
        Ss = zeros(NnP,NnP,Nfh);
        for ik = 1:Nfh
            Ss(:,:,ik) = Sstmp.*exp(-1j*w(ik).*TrPF);
        end
                
  	elseif  Trflag == 0 && Gtype > 0
        Ptmp = Pall-Pg;
      	Cs = diag(1./diag(Ptmp));
        Ss = zeros(NnP,NnP);
       	for ig = 1:NnP
           	Ss(:,ig) = Ptmp(:,ig)./Ptmp(ig,ig);
        end 
        
 	elseif  Trflag == 0 && Gtype == 0
        Cs = diag(1./diag(Pall));
        Ss = zeros(NnP,NnP);
       	for ig = 1:NnP
           	Ss(:,ig) = Pall(:,ig)./Pall(ig,ig);
        end 
    end

end


%% Branch Parameter (Combining Componects Belonging to a Conductor)
if Lflag > 0
    Zb = zeros(NbL,NbL,Nfh);
   	for k = 1:Nfh
      	Zb(:,:,k) = Rall(:,:,k)+1j*w(k)* Lall(:,:,k);
    end
else
    Zb = zeros(NbL,NbL);
end

%%  %%%%%%%%%%%%%%%%%%% Ground Node Remove P %%%%%%%%%%%%%%%%%%%%%
if Pflag > 0 
    ind = ismember(NdP, NdGD);
   	for ik = length(ind):-1:1
        if ind(ik) == 1
            NdP(ik) = [];
        end
    end
    NnP =length(NdP);
    
    if Trflag > 0 && Gtype > 0 
        Pc = zeros(NnP,NnP,Nfh);
        ind = ~ind;
        for ik = 1:Nfh
            Pc(:,:,ik) = Pf(ind,ind,ik);
        end
        
    elseif Trflag > 0 && Gtype == 0   
        Sc = zeros(NnP,NnP,Nfh);
        ind = ~ind;
        
        Cc = Cs(ind,ind);
        for ik = 1:Nfh
            Sc(:,:,ik) = Ss(ind,ind,ik);
        end
        
    elseif Trflag == 0 
        ind = ~ind;
        
        Cc = Cs(ind,ind);
       	Sc = Ss(ind,ind);
    end
else
    Pc = zeros(NnP,NnP);
    Cc = zeros(NnP,NnP);
    Sc = eye(NnP);
end



tSta = tic;
%% %%%%%%%%%%%%%% Network Solving using MNA Method %%%%%%%%%%%%%%%
Io1 = zeros(Nb,Nfh);
Vo1 = zeros(Nn,Nfh);

if SVtype == 2
    
%     if Gtype == 0
%         Pm=Pall;
%         for ik = 1:Nfh
%             Pf(:,:,ik) = Pm.*exp(-1j*w(ik).*TrFP);    
%         end
%     end
    
    for k = 1:Nfh
        Io1(:,k) = MLA_Fsolver( Zb(:,:,k), Pc(:,:,k), ...
            Rec, Lec, Cec, Rcon, A, ID_SV, ID_SI, SRtF(k), w(k) );
    end

else
    if Trflag > 0 && Gtype > 0
        if Pflag > 0
            for k = 1:Nfh
                [Io1(:,k), Vo1(:,k)] = MNA_GFsolver( Zb(:,:,k), Pc(:,:,k), ...
                    Rec, Lec, Cec, Rcon, A, ID_SV, ID_SI, SRtF(k), w(k) );
            end
        else
            for k = 1:Nfh
                [Io1(:,k), Vo1(:,k)] = MNA_GFLsolver( Zb(:,:,k), ...
                    Rec, Lec, Cec, Rcon, A, ID_SV, ID_SI, SRtF(k), w(k) );
            end
        end
    
    elseif Trflag > 0 && Gtype == 0
    
        for k = 1:Nfh
            [Io1(:,k), Vo1(:,k)] = MNA_Fsolver( Zb(:,:,k), Cc, Sc(:,:,k), ...
                Rec, Lec, Cec, Rcon, A, ID_SV, ID_SI, SRtF(k), w(k) );
        end
    
    elseif Trflag == 0
    
        for k = 1:Nfh
            [Io1(:,k), Vo1(:,k)] = MNA_Fsolver( Zb(:,:,k), Cc, Sc, ...
                Rec, Lec, Cec, Rcon, A, ID_SV, ID_SI, SRtF(k), w(k) );
        end    
    
    end

end

%% %%%%%%%%%%%%%% Using Symmetry of the Fourier Transform %%%%%%%%%%%%%%%%% 
% only half frequency points need to calculate
if (rem(Nf,2)==0)
    Vo2 = conj(Vo1(:,Nfh-1:-1:2));
    Io2 = conj(Io1(:,Nfh-1:-1:2));
else
    Vo2 = conj(Vo1(:,Nfh:-1:2));
    Io2 = conj(Io1(:,Nfh:-1:2));
end

Vnf = cat(2,Vo1,Vo2);
Ibf = cat(2,Io1,Io2);

tSto = toc(tSta);
str = sprintf('Time for Frequency Domain Network Solving in is %f s', tSto);
disp(str);


%PLOT_RL(Rs,Ls,fh,1000,'red');


end




