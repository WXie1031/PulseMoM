% function [Rblk, Lblk] = para_main_body(IBCdat3D, Nibc_3D, ...
%      ICBdat3D, Nicb_3D) 

function [Rblk, Lblk] = para_main_body_old(IBCdat3D, Nibc_3D, ...
    ICBdat3D, Nicb_3D, TCGdat3D, Ntcg_3D, TCGdat_cs2D, Ntcg_cs2D) 
%  Function:       para_RL
%  Description:    Calculate R and length matrix of all conductors.
%                  1-5 : filament model
%                  For filament model, R is the internal resistance of
%                  specified frequency. 
%                  6   : 2D meshing calculation method
%                  For meshing model, R is the resistance of 500Hz, length is
%                  the inductance of 10kHz
%                  
%  Calls:          cal_L_filament
%                  cal_RL_mesh2D
%                  
%  Input:          IBC  --  data of basic conductors (structure)
%                  ICB  --  data of cables (structure)
%                  TCG  --  data of cable group (structure)
%                  N    --  demension info. of all data
%  Output:         Rblk --  R matrix
%                  length    --  length matrix
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2014-12-13
%  History:         
%      <author>      <time>       <desc>
%      David         96/10/12     build this moudle  


Nibc3D = Nibc_3D.num_all;
Nicb3D = Nicb_3D.num_all;
Ntcg3D = Ntcg_3D.num_all;

Ntcg3D_grp = Ntcg_3D.num_grp;
Ntcg3D_cdt = Ntcg_3D.num_cdt;
Ntcg3D_all = Ntcg_3D.num_all;

Ntcg2D_mgrp = Ntcg_cs2D.num_mgrp;
Ntcg2D_sgrp = Ntcg_cs2D.num_sgrp;
Ntcg2D_cdt = Ntcg_cs2D.num_cdt;
%Ntcg3D=0;
Nall = Nibc3D+Nicb3D+Ntcg3D;

Rblk = zeros(Nall, Nall);
Lblk = zeros(Nall, Nall);

%% 1. IBC-IBC part -- use filament model and internal indcutance
Ltmp = zeros(Nibc3D,Nibc3D);

pf1 = zeros(Nibc3D,3);
pf2 = zeros(Nibc3D,3);
dv2 = zeros(Nibc3D,3);
l2 = zeros(Nibc3D,1);
r2 = zeros(Nibc3D,1);
Rin = zeros(Nibc3D,1);
Lin = zeros(Nibc3D,1);

for ik = 1:Nibc3D
    pf1(ik,:) = IBCdat3D(ik).pt_start;
    pf2(ik,:) = IBCdat3D(ik).pt_end;
    dv2(ik,:) = IBCdat3D(ik).direct_vector;
    l2(ik) = IBCdat3D(ik).length;
    r2(ik) = IBCdat3D(ik).r_equivalent;
    
end

for ik = 1:Nibc3D

    ps1 = IBCdat3D(ik).pt_start;
    ps2 = IBCdat3D(ik).pt_end;
    dv1 = IBCdat3D(ik).direct_vector;
    l1 = IBCdat3D(ik).length;
    r1 = IBCdat3D(ik).r_equivalent;

    Rin(ik) = IBCdat3D(ik).Rin_pul*IBCdat3D(ik).length;
    Lin(ik) = IBCdat3D(ik).Lin_pul*IBCdat3D(ik).length;
    % calculate inductance using filament model
    Ltmp(1:ik,ik) = cal_L_filament(ps1,ps2,dv1,l1,r1, ...
        pf1(1:ik,:),pf2(1:ik,:),dv2(1:ik,:),l2(1:ik),r2(1:ik));
end

Lblk(1:Nibc3D,1:Nibc3D) = Ltmp+Ltmp'-diag(diag(Ltmp))/2 + diag(Lin);

% resistance
Rblk(1:Nibc3D,1:Nibc3D) = diag(Rin) ;

%clear Ltmp pf1 pf2 dv2 l2 r2

%% 2. IBC-ICB part -- use filament model and internal indcutance
Ltmp = zeros(Nicb3D,Nibc3D);

pf1 = zeros(Nicb3D,3);
pf2 = zeros(Nicb3D,3);
dv2 = zeros(Nicb3D,3);
l2 = zeros(Nicb3D,1);
r2 = zeros(Nicb3D,1);

for ik = 1:Nicb3D
    pf1(ik,:) = ICBdat3D(ik).pt_start;
    pf2(ik,:) = ICBdat3D(ik).pt_end;
    dv2(ik,:) = ICBdat3D(ik).direct_vector;
    l2(ik) = ICBdat3D(ik).length;
    r2(ik) = ICBdat3D(ik).r_equivalent;
end
    
for ik = 1:Nibc3D

    ps1 = IBCdat3D(ik).pt_start;
    ps2 = IBCdat3D(ik).pt_end;
    dv1 = IBCdat3D(ik).direct_vector;
    l1 = IBCdat3D(ik).length;
    r1 = IBCdat3D(ik).r_equivalent;

    % calculate inductance using filament model
    Ltmp(:,ik) = cal_L_filament(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2);
end

Lblk(1:Nibc3D, Nibc3D+(1:Nicb3D)) = Ltmp';
Lblk(Nibc3D+(1:Nicb3D), 1:Nibc3D) = Ltmp;

%clear Ltmp pf1 pf2 dv2 l2 r2

%% 3. IBC-TCG part -- use filament model and internal indcutance
Ltmp = zeros(Ntcg3D,Nibc3D);

pf1 = zeros(Ntcg3D,3);
pf2 = zeros(Ntcg3D,3);
dv2 = zeros(Ntcg3D,3);
l2 = zeros(Ntcg3D,1);
r2 = zeros(Ntcg3D,1);

offset = 0;
for ik = 1:Ntcg3D_grp
%     ind = offset + (1:Ntcg3D_cdt(ik));
    pf1(offset + (1:Ntcg3D_cdt(ik)),:) = TCGdat3D(ik).pt_start;
    pf2(offset + (1:Ntcg3D_cdt(ik)),:) = TCGdat3D(ik).pt_end;
    dv2(offset + (1:Ntcg3D_cdt(ik)),:) = TCGdat3D(ik).direct_vector;
    l2(offset + (1:Ntcg3D_cdt(ik))) = TCGdat3D(ik).length;
    r2(offset + (1:Ntcg3D_cdt(ik))) = TCGdat3D(ik).r_equivalent;
    offset = offset+Ntcg3D_cdt(ik);
end
    
for ik = 1:Nibc3D

    ps1 = IBCdat3D(ik).pt_start;
    ps2 = IBCdat3D(ik).pt_end;
    dv1 = IBCdat3D(ik).direct_vector;
    l1 = IBCdat3D(ik).length;
    r1 = IBCdat3D(ik).r_equivalent;

    % calculate inductance using filament model
    Ltmp(:,ik) = cal_L_filament(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2);
end

% indx = 1:Nibc3D;
% indy = Nibc3D+Nicb3D+(1:Ntcg3D);
Lblk(1:Nibc3D,Nibc3D+Nicb3D+(1:Ntcg3D)) = Ltmp';
Lblk(Nibc3D+Nicb3D+(1:Ntcg3D),1:Nibc3D) = Ltmp;

%clear Ltmp pf1 pf2 dv2 l2 r2

%% 4. ICB-ICB part -- use filament model and internal indcutance
Ltmp = zeros(Nicb3D,Nicb3D);

pf1 = zeros(Nicb3D,3);
pf2 = zeros(Nicb3D,3);
dv2 = zeros(Nicb3D,3);
l2 = zeros(Nicb3D,1);
r2 = zeros(Nicb3D,1);
Rin = zeros(Nicb3D,1);
Lin = zeros(Nicb3D,1);

for ik = 1:Nicb3D
    pf1(ik,:) = ICBdat3D(ik).pt_start;
    pf2(ik,:) = ICBdat3D(ik).pt_end;
    dv2(ik,:) = ICBdat3D(ik).direct_vector;
    l2(ik) = ICBdat3D(ik).length;
    r2(ik) = ICBdat3D(ik).r_equivalent;
end
    
for ik = 1:Nicb3D
    
%     ind = 1:ik;

    ps1 = ICBdat3D(ik).pt_start;
    ps2 = ICBdat3D(ik).pt_end;
    dv1 = ICBdat3D(ik).direct_vector;
    l1 = ICBdat3D(ik).length;
    r1 = ICBdat3D(ik).r_equivalent;

    Rin(ik) = ICBdat3D(ik).Rin_pul*ICBdat3D(ik).length;
    Lin(ik) = ICBdat3D(ik).Lin_pul*ICBdat3D(ik).length;
    
    % calculate inductance using filament model
    Ltmp(1:ik,ik) = cal_L_filament(ps1,ps2,dv1,l1,r1, ...
        pf1(1:ik,:),pf2(1:ik,:),dv2(1:ik,:),l2(1:ik),r2(1:ik));
end

% indx = Nibc3D+(1:Nicb3D);
% indy = Nibc3D+(1:Nicb3D);
% self inductance
Lblk(Nibc3D+(1:Nicb3D), Nibc3D+(1:Nicb3D)) = Ltmp+Ltmp'-diag(diag(Ltmp))/2 + diag(Lin);

% resistance
Rblk(Nibc3D+(1:Nicb3D), Nibc3D+(1:Nicb3D)) = diag(Rin);

%clear Ltmp pf1 pf2 dv2 l2 r2

%% 5. ICB-TCG part -- use filament model and internal indcutance
Ltmp = zeros(Ntcg3D,Nicb3D);

pf1 = zeros(Ntcg3D,3);
pf2 = zeros(Ntcg3D,3);
dv2 = zeros(Ntcg3D,3);
l2 = zeros(Ntcg3D,1);
r2 = zeros(Ntcg3D,1);

offset = 0;
for ik = 1:Ntcg3D_grp
%     ind = offset + (1:Ntcg3D_cdt(ik));
    pf1( offset + (1:Ntcg3D_cdt(ik)),:) = TCGdat3D(ik).pt_start;
    pf2( offset + (1:Ntcg3D_cdt(ik)),:) = TCGdat3D(ik).pt_end;
    dv2( offset + (1:Ntcg3D_cdt(ik)),:) = TCGdat3D(ik).direct_vector;
    l2( offset + (1:Ntcg3D_cdt(ik))) = TCGdat3D(ik).length;
    r2( offset + (1:Ntcg3D_cdt(ik))) = TCGdat3D(ik).r_equivalent;
    offset = offset+Ntcg3D_cdt(ik);
end
    
for ik = 1:Nicb3D

    ps1 = ICBdat3D(ik).pt_start;
    ps2 = ICBdat3D(ik).pt_end;
    dv1 = ICBdat3D(ik).direct_vector;
    l1 = ICBdat3D(ik).length;
    r1 = ICBdat3D(ik).r_equivalent;
    
    % calculate inductance using filament model
    Ltmp(:,ik) = cal_L_filament(ps1,ps2,dv1,l1,r1, pf1,pf2,dv2,l2,r2);
end

% indx = Nibc3D+(1:Nicb3D);
% indy = Nibc3D+Nicb3D+(1:Ntcg3D);

Lblk(Nibc3D+(1:Nicb3D), Nibc3D+Nicb3D+(1:Ntcg3D)) = Ltmp';
Lblk(Nibc3D+Nicb3D+(1:Ntcg3D), Nibc3D+(1:Nicb3D)) = Ltmp;


%% 6. TCG-TCG part -- use 2D meshing calculation program
Ltcg_blk = zeros(Ntcg3D_all,Ntcg3D_all);
Rtcg_blk = zeros(Ntcg3D_all,Ntcg3D_all);

% Self inductance and resistance block
offset = 0;
for ik = 1:Ntcg2D_mgrp
  	[Rtmp, Ltmp] = cal_RL_mesh2D(TCGdat_cs2D(ik));

    for ih = 1:Ntcg2D_sgrp(ik)
%         indx = offset+(1:Ntcg2D_cdt(ik));
%         indy = offset+(1:Ntcg2D_cdt(ik));
    
        Ltcg_blk(offset+(1:Ntcg2D_cdt(ik)), offset+(1:Ntcg2D_cdt(ik))) = Ltmp(:,:,ih);
        Rtcg_blk(offset+(1:Ntcg2D_cdt(ik)), offset+(1:Ntcg2D_cdt(ik))) = Rtmp(:,:,ih);
        
        offset = offset+Ntcg2D_cdt(ik);
    end
end


for ik = 1:Ntcg3D_grp-1
    
%     indx = (1:Ntcg3D_cdt(ik));
    
    ps1 = TCGdat3D(ik).pt_start;
    ps2 = TCGdat3D(ik).pt_end;
    dv1 = TCGdat3D(ik).direct_vector;
    l1 = TCGdat3D(ik).length;
    r1 = TCGdat3D(ik).r_equivalent;
    Ns = size(l1,1);
    
    for ih = ik+1:Ntcg3D_grp
        
%         indy = sum(Ntcg3D_cdt(1:ih-1))+(1:Ntcg3D_cdt(ih));
        
        pf1 = TCGdat3D(ih).pt_start;
        pf2 = TCGdat3D(ih).pt_end;
        dv2 = TCGdat3D(ih).direct_vector;
        l2 = TCGdat3D(ih).length;
        r2 = TCGdat3D(ih).r_equivalent;
        Nf = size(l2,1);
        Ltmp = zeros(Ns,Nf);
        
        for ig = 1:Ns
            % calculate inductance using filament model
            Ltmp(:,ig) = cal_L_filament(ps1(ig,:),ps2(ig,:),dv1(ig,:),l1(ig),r1(ig), ...
                pf1,pf2,dv2,l2,r2);
        end
        
        Ltcg_blk(1:Ntcg3D_cdt(ik), sum(Ntcg3D_cdt(1:ih-1))+(1:Ntcg3D_cdt(ih))) = Ltmp;
        Ltcg_blk(sum(Ntcg3D_cdt(1:ih-1))+(1:Ntcg3D_cdt(ih)), 1:Ntcg3D_cdt(ik)) = Ltmp';
    end
end

% indx = Nibc3D+Nicb3D+(1:Ntcg3D);
% indy = Nibc3D+Nicb3D+(1:Ntcg3D);
Lblk(Nibc3D+Nicb3D+(1:Ntcg3D), Nibc3D+Nicb3D+(1:Ntcg3D)) = Ltcg_blk;
Rblk(Nibc3D+Nicb3D+(1:Ntcg3D), Nibc3D+Nicb3D+(1:Ntcg3D)) = Rtcg_blk;

Lblk = abs(Lblk);

end


