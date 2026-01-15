
data_name = 'database_nn_interp2';

load(data_name)

Ncel = size(data_proximity,1);

mu0 = 4*pi*1e-7;

%Ncel = 37;   % 18 simple case
Ntest = 0;
for ik = 1:Ncel
    if ~isempty(data_proximity{ik,1})
        ik
        
        dat = data_proximity{ik,1};
        %% calculate the skin effect
        Nc = length(dat.dim1);
        Nf = length(dat.frq);
        
        Rmtx = ones(Nc,Nc,Nf);
        Lmtx = zeros(Nc,Nc,Nf);
        
        pt_start = [dat.pt_2d, zeros(Nc,1)];
        pt_end = [dat.pt_2d, dat.len*ones(Nc,1)];
        [dv, len] = line_dv(pt_start, pt_end);
        
        mur = ones(Nc,1);
        id_rec = dat.shp_id == 1002;
        id_agi = dat.shp_id == 1003;
        id_cir = ~(id_rec + id_agi);
        
        S = zeros(Nc,1);
        S(id_rec) = dat.dim1(id_rec).*dat.dim2(id_rec);
        S(id_agi) = abs(2*dat.dim1(id_agi).*dat.dim2(id_agi)) - dat.dim2(id_agi).*dat.dim2(id_agi);
        S(id_cir) = pi*(dat.dim1(id_cir).^2 - dat.dim2(id_cir).^2);
        
        sig = 1./(dat.Rpul.*S);
        
        [Rself, Lself] = para_self_multi_frq(dat.shp_id, dat.dim1, dat.dim2, ...
            len, dat.Rpul, sig, mur, dat.frq);
        
        re = sqrt(S/pi);
        
        [Rtmp, Ltmp] = para_main_fila(pt_start, pt_end, dv, re, len);
        
        for ic = 1:Nf
            Lmtx(:,:,ic) = Ltmp;
        end
        
        for ic = 1:Nc
            Rmtx(ic,ic,:) = Rself(ic,:);
            Lmtx(ic,ic,:) = Lself(ic,:);
        end
        
        dat.rat_Rsp = dat.RmeshMF./Rmtx;
        dat.rat_Lsp = dat.LmeshMF./Lmtx;
        dat.Rskin = Rself;
        dat.Lskin = Lself;
        dat.re = re;
        dat.S = S;
        dat.sig = sig;
        data_proximity{ik,1} = dat;
        
        Ntest = Ntest+Nc*Nf;
    end
end

save(data_name,'data_proximity');


%% generate the input for the NN
Nseg = 6;
arg_quadrant = 2*pi/Nseg*(0:Nseg);

nn_input = zeros(4+Nseg,Ntest);
%nn_input = zeros(4,Ntest);
nn_output1 = zeros(1,Ntest);
nn_output2 = zeros(1,Ntest);


cnt = 0;
for ic = 1:Ncel
    dat = data_proximity{ic,1};
    
    if ~isempty(dat)
        Nc = length(dat.dim1);
        Nf = length(dat.frq);
        
        for ik = 1:Nc
            id_oth = (1:Nc);
            id_oth(ik) = [];
            dl = 2*pi*dat.re(ik)/Nseg; % choose 1/3 of the perimeter as the dl
            
            % mesh here
            [Xs,Ys] = mesh2d_nn_cir(dat.pt_2d(id_oth,:), dat.re(id_oth), dl);
            
            rs_quadrant = zeros(Nseg,1);
            arg_seg = atan((Ys-dat.pt_2d(ik,2))./(Xs-dat.pt_2d(ik,1)));
            for ig = 1:Nseg
                ind = (arg_seg>=arg_quadrant(ig)) & (arg_seg<arg_quadrant(ig+1));
                arg_mid = (arg_quadrant(ig)+arg_quadrant(ig+1))/2;
                pt_edge = [dat.pt_2d(ik,1)+ dat.dim1(ik)*cos(arg_mid) ...
                    dat.pt_2d(ik,2)+ dat.dim1(ik)*sin(arg_mid) ];
                rs_quadrant(ig) = sum(1./sqrt(((pt_edge(1)-Xs(ind)).^2+(pt_edge(2)-Ys(ind)).^2))*dl);
            end
            
            dsum = sum(sqrt( (dat.pt_2d(ik,1)-Xs).^2+(dat.pt_2d(ik,2)-Ys).^2 ));
            d2sum = sum( (dat.pt_2d(ik,1)-Xs).^2+(dat.pt_2d(ik,2)-Ys).^2 );
            logdsum = sum(-0.5*log( (dat.pt_2d(ik,1)-Xs).^2+(dat.pt_2d(ik,2)-Ys).^2 ));
            Ns = length(Xs);
            gmdd = prod( nthroot(sqrt((dat.pt_2d(ik,1)-Xs).^2+(dat.pt_2d(ik,2)-Ys).^2),Ns) );
            % r = 1/d
            rsum = sum(1./sqrt((dat.pt_2d(ik,1)-Xs).^2+(dat.pt_2d(ik,2)-Ys).^2));
            r2sum = sum(1./(dat.pt_2d(ik,1)-Xs).^2+(dat.pt_2d(ik,2)-Ys).^2);
            %logrsum = sum(-0.5*log((dat.pt_2d(ik,1)-Xs).^2+(dat.pt_2d(ik,2)-Ys).^2));
            for ig = 1:Nf
                cnt = cnt+1;
                
                s_dep = 1./sqrt(pi*dat.frq(ig).*dat.sig(ik).*mu0);
                
                nn_input(1,cnt) = sqrt(dat.frq(ig));
                nn_input(2,cnt) = dat.Rpul(ik);
                nn_input(3,cnt) = dat.S(ik);
                nn_input(4,cnt) = 1;
                nn_input(5:end,cnt) = rs_quadrant;
                %nn_input(5,cnt) = s_dep;
                %         nn_input(1,cnt) = dat.frq(ik);
                %         nn_input(1,cnt) = dat.frq(ik);
                %         nn_input(1,cnt) = dat.frq(ik);
                
                nn_output1(cnt)= dat.rat_Rsp(ik,ik,ig)-1;
                nn_output2(cnt)= -dat.rat_Lsp(ik,ik,ig)+1;
            end
        end
    end
end

%% multipole linear regression
nn_output = [nn_output1;nn_output2];
% [b, bint, r] = regress(nn_output1',nn_input');
% % [B,dev,stats] = mnrfit(nn_output1',nn_input');
% out_new = nn_input'*b;
% figure;
% hold on; 
% plot(out_new);
% plot(nn_output1,'.')
% 
% mdl = stepwiselm(nn_input',nn_output1','interactions');
% 

test_NN_example

pathname = './mlearn/tmpNet';
genFunction(net,pathname);

new_out = tmpNet(nn_input);

rat_err = (new_out-nn_output);
figure(5)
subplot(1,2,1)
hold on
plot(new_out(1,:),'b');
plot(nn_output(1,:),'r');
title('Ratio of the R')
hold off
subplot(1,2,2)
hold on
plot(new_out(2,:),'b');
plot(nn_output(2,:),'r');
title('Ratio of the L')
hold off

figure(4)
subplot(1,2,1)
plot(rat_err(1,:)./(nn_output(1,:)+1)*100);
title('Error of Ratio of the R %')
subplot(1,2,2)
plot(rat_err(2,:)./(-nn_output(2,:)+1)*100);
title('Error of Ratio of the L %')






