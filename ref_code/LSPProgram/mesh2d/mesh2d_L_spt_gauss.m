function [Xs,Ys, rs1,rs2, as1,as2, dS, Rs, NVst] = mesh2d_L_spt_gauss(pt_2d, ...
    rout, rin, Rp, p2Doth, f0)
%  Function:       mesh2d_L_spt_gauss
%  Description:    Mesh circular cross sections (2D). The centre is diged out            
%  Calls:          
%  Input:          pt_2d --  coordinate of source line (2D cross section)
%                  rout  --  outer radius of the conductor
%                  rin   --  inner radius of the conductor
%                  Rp    --  resistivity of the conductor (ohm/m)
%  Output:         Xs    --  [Ns*1] X axis of the center of segments (anulus section)
%                  Ys    --  [Ns*1] Y axis of the center of segments (anulus section)
%                  rs1   --  [Nc*1] outer radius of the segments (anulus section)
%                  rs2   --  [Nc*1] inner radius of the segments (anulus section)
%                  as1   --  [Ns*1] start angle of the segments (anulus section)
%                  as2   --  [Ns*1] end angle of the segments (anulus section)
%                  ls    --  [Nc*1] length of the segments (anulus section)
%                  Rs    --  [Ns*1] resistance of the segments (anulus section) (ohm)
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2018-03-19
%  Update:         


mu0 = 4*pi*1e-7;
Nc = size(pt_2d,1);  % the number of cable
f0 = min(f0, 1e6);
cond = 5.8e7;  % conductivity

% dl = 0.1;
% % NVst = ceil(2*pi*rout./dl);

S = pi .* (rout.^2-rin.^2);           % area of cross section

s_dep = 1./sqrt(pi*f0*cond.*mu0) * ones(Nc,1);     % skin depth

% X = 10*s_dep./rout;
X = s_dep./rout;
a = 3.519;
b = -0.7034;
c = 4.345;
NVst = min(max(ceil( a*X.^b+c ), 64), 4000);


%% 3. add exact coordinate and mesh all circular conductors
Ns = sum(NVst);
rs1 = zeros(Ns,1);
rs2 = zeros(Ns,1);
as1 = zeros(Ns,1);
as2 = zeros(Ns,1);
Xs = zeros(Ns,1);
Ys = zeros(Ns,1);
dS = zeros(Ns,1);
Rs = zeros(Ns,1);


off_gwei = 0.2;
cnt = 0;
for ik = 1:Nc

    ang_rng = linspace(0,2*pi,NVst(ik));
    % 1D gaussian function is used for ono-uniform meshing
%     p_ang = zeros(Nf,1);
%     p_r = sqrt(sum(((p2Doth-ones(Nf,1)*pt_2d(ik,1:2)).^2),2));
%     p_cos = (p2Doth(:,1)-ones(Nf,1)*pt_2d(ik,1))./p_r;
%     
%     id_cos_pi = (p_cos>pi);
%     p_ang(~id_cos_pi) = acos(p_cos(~id_cos_pi));
%     p_ang(id_cos_pi) = pi+acos(p_cos(id_cos_pi));
    
    % change angles into cartesian coordinate 
%     x_ang = zeros(Nf,Nang);
%     x_offset = pol2cart(pi, rout(ik));
%     for ig = 1:Nf
%         x_ang(ig,:) = pol2cart(ang_rng, p_r(ig));
%     end
%     x_ang = x_ang+x_offset;
    
    
    [r0f, ang_oth] = angle_point2d([0 0], p2Doth);
    Gwei = mesh2d_L_spt_gauss_sub(ang_rng, ang_oth, off_gwei);
    
    dang = abs(ang_rng(2)-ang_rng(1));
    dGmin = off_gwei*dang;
    
    [Gmax, id_start] = max(Gwei); 
    ang_start = ang_rng(id_start);
    
    cnt_1 = 1;
    ang_vec_1(cnt_1) = ang_start;
    ang_tmp = 0;
    while ang_tmp < 2*pi
        Gwei = mesh2d_L_spt_gauss_sub(ang_vec_1(cnt_1), ang_oth, off_gwei);
        
        dang_tmp = dGmin/Gwei;
        ang_tmp =  ang_vec_1(cnt_1) + dang_tmp;
        
        if ang_tmp > (2*pi-dang/2) && (dang_tmp>dang/2)
            cnt_1 = cnt_1+1;
            ang_tmp = 2*pi;
            ang_vec_1(cnt_1) = ang_tmp;
        else
            cnt_1 = cnt_1+1;
            ang_vec_1(cnt_1) = ang_tmp;
        end
    end
    
    cnt_2 = 1;
    ang_vec_2(cnt_2) = ang_start;
    ang_tmp = 1;
    while ang_tmp > 0
        Gwei = mesh2d_L_spt_gauss_sub(ang_vec_2(cnt_2), ang_oth, off_gwei);
        
        dang_tmp = dGmin/Gwei;
        ang_tmp =  ang_vec_2(cnt_2) - dang_tmp;

        if ang_tmp < (0+dang/2) && (dang_tmp>dang/2)
            cnt_2 = cnt_2+1;
            ang_tmp = 0;
            ang_vec_2(cnt_2) = 0;
        else
            cnt_2 = cnt_2+1;
            ang_vec_2(cnt_2) = ang_tmp;
        end
    end
    
    NVst(ik) = cnt_1+cnt_2-2;
    ang_vec = [ang_vec_2(end:-1:2) ang_vec_1]; 
%     figure(22)
%     hold on
%     plot(ang_rng, Gwei,'k', 'LineWidth',1);
%     plot(ang_rng, Gwei_vec);
%     set(gca,'xtick',0:pi/2:2*pi)
%     set(gca,'xticklabel',{'0','\pi/2','\pi','3\pi/2','2\pi'})
%     xlim([0 2*pi])
%     area(ang_rng, Gwei);

    ind = cnt+(1:NVst(ik));
    
    rs1(ind,1) = rout(ik)*ones(1,NVst(ik));
    rs2(ind,1) = rin(ik)*ones(1,NVst(ik));
    as1(ind,1) = ang_vec(1:NVst(ik));
    as2(ind,1) = ang_vec(2:NVst(ik)+1);
    
    cnt = cnt + NVst(ik);
end

cnt = 0;
for ik = 1 : Nc
    % start and end index
    ind = cnt+(1:NVst(ik));
    
    % caculate the R of the segments
    dS(ind,1) = (as2(ind,1)-as1(ind,1)).*(rs2(ind,1).^2-rs1(ind,1).^2)/2;
    Rs(ind,1) = Rp(ik,1).*S(ik,1)./dS(ind,1);

    X_a = (as2(ind,1)+as1(ind,1))/2; 
    X_r = (rs2(ind,1)+rs1(ind,1)) /2;

    Xs(ind,1) = X_r.*cos(X_a) + pt_2d(ik,1);
    Ys(ind,1) = X_r.*sin(X_a) + pt_2d(ik,2);

    cnt = cnt + NVst(ik);
end
% 
% figure(21)
% plot(Xs, Ys, '*');

end

