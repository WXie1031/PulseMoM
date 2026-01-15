mu0 = 4e-7*pi;
ep0 = 8.854187817e-12;

base_dir = fileparts(mfilename('fullpath'));
ref_dir = fullfile(base_dir, '..', 'dgf-strata-main', 'test', 'examples', 'ling_jin_2000');
ref_dir2 = fullfile(base_dir, '..', '..', 'dgf-strata-main', 'test', 'examples', 'ling_jin_2000');
ref_base = '';
if exist(ref_dir,'dir') == 7
    ref_base = ref_dir;
    addpath(ref_dir);
elseif exist(ref_dir2,'dir') == 7
    ref_base = ref_dir2;
    addpath(ref_dir2);
end

%% configuration of the multi-layer structure
%% case 1
w0= 2*pi*30e9;
k0 = w0*sqrt(ep0.*mu0);
T = textread(fullfile(base_dir, 'GA.txt'));
xf = T(:,1);

Nxf = length(xf);
pt_s = [0,0,0.4]*1e-3;
pt_f = [xf./k0,zeros(Nxf,1),1.4*ones(Nxf,1)*1e-3];

epr_lyr = [ 1; 2.1; 12.5; 9.8; 8.6; 1];
mur_lyr = [1; 1; 1; 1; 1; 1;];
sig_lyr = [0; 0; 0; 0; 0; 1e10];

zbdy = [1.8; 1.1; 0.8; 0.3; 0; ]*1e-3; 

id_s = dgf_locate_lyr(pt_s(:,3), zbdy);
id_f = dgf_locate_lyr(pt_f(:,3), zbdy);
fprintf('source layer id_s=%d, field layer id_f in [%d,%d]\n', id_s, min(id_f), max(id_f));
zb_bot = zbdy(end);
fprintf('z from bottom: z_src=%g mm, z_obs=%g mm\n', (pt_s(3)-zb_bot)*1e3, (pt_f(1,3)-zb_bot)*1e3);


%% calculate the transmission Green's functions

dv_s=[1 0 0];
dv_f=ones(Nxf,1)*[1 0 0];
tic
[Gxx,Gzx,Gzz,Gphi] = dgf_main(pt_s,dv_s, pt_f,dv_f, zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
toc

pt_s2 = [0,0,1.4]*1e-3;
pt_f2 = [xf./k0,zeros(Nxf,1),0.4*ones(Nxf,1)*1e-3];
id_s2 = dgf_locate_lyr(pt_s2(:,3), zbdy);
id_f2 = dgf_locate_lyr(pt_f2(:,3), zbdy);
fprintf('swap-z test: source layer id_s2=%d, field layer id_f2 in [%d,%d]\n', id_s2, min(id_f2), max(id_f2));
tic
[Gxx2_calc,Gzx2_calc,Gzz2_calc,Gphi2_calc] = dgf_main(pt_s2,dv_s, pt_f2,dv_f, zbdy,epr_lyr,mur_lyr,sig_lyr, w0);
toc



paperGphi = [];
paperGxx = [];
paperGzx = [];
paperGzz = [];
paperGxz = [];

paperGphi2 = [];
paperGxx2 = [];
paperGzx2 = [];
paperGzz2 = [];

if exist(fullfile(base_dir, 'Gphi_exp1.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gphi_exp1.mat'));
    if isfield(tmp,'Gphi_exp1')
        paperGphi = tmp.Gphi_exp1;
    end
end
if exist(fullfile(base_dir, 'Gphi_exp2.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gphi_exp2.mat'));
    if isfield(tmp,'Gphi_exp2')
        paperGphi2 = tmp.Gphi_exp2;
    end
end
if ~isempty(ref_base) && exist(fullfile(ref_base, 'fig3_Gphi.csv'),'file')==2
    if exist('readmatrix','file')==2
        paperGphi = readmatrix(fullfile(ref_base, 'fig3_Gphi.csv'));
    else
        paperGphi = csvread(fullfile(ref_base, 'fig3_Gphi.csv'));
    end
end

if exist(fullfile(base_dir, 'Gxx_exp1.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gxx_exp1.mat'));
    if isfield(tmp,'Gxx_exp1')
        paperGxx = tmp.Gxx_exp1;
    end
end
if exist(fullfile(base_dir, 'Gxx_exp2.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gxx_exp2.mat'));
    if isfield(tmp,'Gxx_exp2')
        paperGxx2 = tmp.Gxx_exp2;
    end
end
if ~isempty(ref_base) && exist(fullfile(ref_base, 'fig3_Gxx.csv'),'file')==2
    if exist('readmatrix','file')==2
        paperGxx = readmatrix(fullfile(ref_base, 'fig3_Gxx.csv'));
    else
        paperGxx = csvread(fullfile(ref_base, 'fig3_Gxx.csv'));
    end
end

if exist(fullfile(base_dir, 'Gzx_exp1.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gzx_exp1.mat'));
    if isfield(tmp,'Gzx_exp1')
        paperGzx = tmp.Gzx_exp1;
    end
end
if exist(fullfile(base_dir, 'Gzx_exp2.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gzx_exp2.mat'));
    if isfield(tmp,'Gzx_exp2')
        paperGzx2 = tmp.Gzx_exp2;
    end
end
if ~isempty(ref_base) && exist(fullfile(ref_base, 'fig3_Gzx.csv'),'file')==2
    if exist('readmatrix','file')==2
        paperGzx = readmatrix(fullfile(ref_base, 'fig3_Gzx.csv'));
    else
        paperGzx = csvread(fullfile(ref_base, 'fig3_Gzx.csv'));
    end
end

if ~isempty(ref_base)
    mgf_interp_path = fullfile(ref_base, 'MGFdata_interp.txt');
    mgf_path = fullfile(ref_base, 'MGFdata.txt');
    mgf_file = '';
    if exist(mgf_interp_path, 'file') == 2
        mgf_file = mgf_interp_path;
    elseif exist(mgf_path, 'file') == 2
        mgf_file = mgf_path;
    end

    if ~isempty(mgf_file)
        fid = fopen(mgf_file, 'r');
        if fid ~= -1
            header_found = false;
            line = fgetl(fid);
            while ischar(line)
                if strncmp(strtrim(line), 'rho', 3)
                    header_found = true;
                    break;
                end
                line = fgetl(fid);
            end

            if header_found
                C = textscan(fid, '%f%f%f%f%f%f%f%f%f%f%f', 'CollectOutput', true);
                data = C{1};
                if size(data, 2) >= 11
                    rho = data(:,1);
                    Gxz_col = data(:,4);
                    Gzx_col = data(:,8);
                    xk0 = rho * k0;
                    paperGxz = [xk0, abs(Gxz_col)];
                    if isempty(paperGzx)
                        paperGzx = [xk0, abs(Gzx_col)];
                    end
                end
            end

            fclose(fid);
        end
    end
end

if exist(fullfile(base_dir, 'Gzz_exp1.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gzz_exp1.mat'));
    if isfield(tmp,'Gzz_exp1')
        paperGzz = tmp.Gzz_exp1;
    end
end
if exist(fullfile(base_dir, 'Gzz_exp2.mat'),'file')==2
    tmp = load(fullfile(base_dir, 'Gzz_exp2.mat'));
    if isfield(tmp,'Gzz_exp2')
        paperGzz2 = tmp.Gzz_exp2;
    end
end
if ~isempty(ref_base) && exist(fullfile(ref_base, 'fig3_Gzz.csv'),'file')==2
    if exist('readmatrix','file')==2
        paperGzz = readmatrix(fullfile(ref_base, 'fig3_Gzz.csv'));
    else
        paperGzz = csvread(fullfile(ref_base, 'fig3_Gzz.csv'));
    end
end


figure(4)

Gxx_mag = max(abs(Gxx), realmin);
Gzx_mag = max(abs(Gzx), realmin);
Gzz_mag = max(abs(Gzz), realmin);
Gphi_mag = max(abs(Gphi), realmin);
Gzx2_mag = max(abs(Gzx2_calc), realmin);

loglog(xf,Gxx_mag,'-','LineWidth',1)
hold on
loglog(xf,Gzx_mag,'-','LineWidth',1)
loglog(xf,Gzz_mag,'-','LineWidth',1)
loglog(xf,Gphi_mag,'-','LineWidth',1)

has_ref = ~isempty(paperGxx) && ~isempty(paperGzx) && ~isempty(paperGzz) && ~isempty(paperGphi);
if has_ref
    loglog(paperGxx(:,1),max(abs(paperGxx(:,2)),realmin),'--');
    loglog(paperGzx(:,1),max(abs(paperGzx(:,2)),realmin),'--');
    loglog(paperGzz(:,1),max(abs(paperGzz(:,2)),realmin),'--');
    loglog(paperGphi(:,1),max(abs(paperGphi(:,2)),realmin),'--');
    legend('Gxx','Gzx','Gzz','Gphi','refGxx','refGzx','refGzz','refGphi');
else
    legend('Gxx','Gzx','Gzz','Gphi');
end

if has_ref
    names = {'Gxx','Gzx','Gzz','Gphi'};
    calc_mag = {Gxx_mag(:), Gzx_mag(:), Gzz_mag(:), Gphi_mag(:)};
    ref_data = {paperGxx, paperGzx, paperGzz, paperGphi};
    ref_data2 = {paperGxx2, paperGzx2, paperGzz2, paperGphi2};

    figure(5)
    for ii = 1:4
        P = ref_data{ii};
        if isempty(P)
            continue;
        end

        xr = P(:,1);
        yr = abs(P(:,2));
        idv = isfinite(xr) & isfinite(yr) & xr>0 & yr>0;
        xr = xr(idv);
        yr = yr(idv);
        if numel(xr) < 2
            continue;
        end
        [xr, idx] = sort(xr);
        yr = yr(idx);
        [xr, ia] = unique(xr,'stable');
        yr = yr(ia);
        if numel(xr) < 2
            continue;
        end

        id_in = isfinite(xf) & xf>0 & xf>=xr(1) & xf<=xr(end);
        if ~any(id_in)
            continue;
        end

        ref_interp = 10.^interp1(log10(xr), log10(yr), log10(xf(id_in)), 'linear');
        calc_interp = calc_mag{ii}(id_in);
        rel_err = abs(calc_interp - ref_interp) ./ max(ref_interp, realmin);
        err_db = 20*log10(calc_interp ./ ref_interp);

        alpha = (calc_interp.'*ref_interp) / max(calc_interp.'*calc_interp, realmin);
        alpha = max(alpha, realmin);
        rel_err_s = abs(alpha*calc_interp - ref_interp) ./ max(ref_interp, realmin);
        err_db_s = 20*log10((alpha*calc_interp) ./ ref_interp);

        rel_err_plot = max(rel_err, realmin);

        fprintf('%s(exp1): N=%d, rel.max=%g, rel.mean=%g, rel.std=%g, rel.rms=%g, rel.median=%g, |dB|max=%g, |dB|mean=%g, alpha=%g\n', ...
            names{ii}, numel(rel_err), max(rel_err), mean(rel_err), std(rel_err), sqrt(mean(rel_err.^2)), median(rel_err), max(abs(err_db)), mean(abs(err_db)), alpha);
        fprintf('%s(exp1,scaled): N=%d, rel.max=%g, rel.mean=%g, rel.std=%g, rel.rms=%g, rel.median=%g, |dB|max=%g, |dB|mean=%g\n', ...
            names{ii}, numel(rel_err_s), max(rel_err_s), mean(rel_err_s), std(rel_err_s), sqrt(mean(rel_err_s.^2)), median(rel_err_s), max(abs(err_db_s)), mean(abs(err_db_s)));

        P2 = ref_data2{ii};
        if ~isempty(P2)
            xr2 = P2(:,1);
            yr2 = abs(P2(:,2));
            idv2 = isfinite(xr2) & isfinite(yr2) & xr2>0 & yr2>0;
            xr2 = xr2(idv2);
            yr2 = yr2(idv2);
            if numel(xr2) >= 2
                [xr2, idx2] = sort(xr2);
                yr2 = yr2(idx2);
                [xr2, ia2] = unique(xr2,'stable');
                yr2 = yr2(ia2);
                id_in2 = isfinite(xf) & xf>0 & xf>=xr2(1) & xf<=xr2(end);
                if any(id_in2)
                    ref_interp2 = 10.^interp1(log10(xr2), log10(yr2), log10(xf(id_in2)), 'linear');
                    calc_interp2 = calc_mag{ii}(id_in2);
                    rel_err2 = abs(calc_interp2 - ref_interp2) ./ max(ref_interp2, realmin);
                    err_db2 = 20*log10(calc_interp2 ./ ref_interp2);

                    alpha2 = (calc_interp2.'*ref_interp2) / max(calc_interp2.'*calc_interp2, realmin);
                    alpha2 = max(alpha2, realmin);
                    rel_err2_s = abs(alpha2*calc_interp2 - ref_interp2) ./ max(ref_interp2, realmin);
                    err_db2_s = 20*log10((alpha2*calc_interp2) ./ ref_interp2);

                    fprintf('%s(exp2): N=%d, rel.max=%g, rel.mean=%g, rel.std=%g, rel.rms=%g, rel.median=%g, |dB|max=%g, |dB|mean=%g, alpha=%g\n', ...
                        names{ii}, numel(rel_err2), max(rel_err2), mean(rel_err2), std(rel_err2), sqrt(mean(rel_err2.^2)), median(rel_err2), max(abs(err_db2)), mean(abs(err_db2)), alpha2);
                    fprintf('%s(exp2,scaled): N=%d, rel.max=%g, rel.mean=%g, rel.std=%g, rel.rms=%g, rel.median=%g, |dB|max=%g, |dB|mean=%g\n', ...
                        names{ii}, numel(rel_err2_s), max(rel_err2_s), mean(rel_err2_s), std(rel_err2_s), sqrt(mean(rel_err2_s.^2)), median(rel_err2_s), max(abs(err_db2_s)), mean(abs(err_db2_s)));
                end
            end
        end

        subplot(2,2,ii)
        loglog(xf(id_in), rel_err_plot, '-', 'LineWidth', 1)
        grid on
        title([names{ii}, ' RelErr'])
        xlabel('xf')
        ylabel('Relative Error')
    end

    if ~isempty(paperGzx)
        Pa = paperGzx;
        xra = Pa(:,1);
        yra = abs(Pa(:,2));
        idva = isfinite(xra) & isfinite(yra) & xra>0 & yra>0;
        xra = xra(idva);
        yra = yra(idva);
        if numel(xra) >= 2
            [xra, idxa] = sort(xra);
            yra = yra(idxa);
            [xra, iaa] = unique(xra,'stable');
            yra = yra(iaa);
            id_ina = isfinite(xf) & xf>0 & xf>=xra(1) & xf<=xra(end);
            if any(id_ina)
                ref_interp_a = 10.^interp1(log10(xra), log10(yra), log10(xf(id_ina)), 'linear');
                ref_interp_a = ref_interp_a(:);
                calc_interp_a = Gzx_mag(id_ina);
                calc_interp_a = calc_interp_a(:);
                rel_a = abs(calc_interp_a - ref_interp_a) ./ max(ref_interp_a, realmin);
                alpha_a = (calc_interp_a.'*ref_interp_a) / max(calc_interp_a.'*calc_interp_a, realmin);
                alpha_a = max(alpha_a, realmin);
                rel_a_s = abs(alpha_a*calc_interp_a - ref_interp_a) ./ max(ref_interp_a, realmin);

                if ~isempty(paperGxz)
                    Pb = paperGxz;
                else
                    Pb = paperGzx;
                end
                xrb = Pb(:,1);
                yrb = abs(Pb(:,2));
                idvb = isfinite(xrb) & isfinite(yrb) & xrb>0 & yrb>0;
                xrb = xrb(idvb);
                yrb = yrb(idvb);
                if numel(xrb) >= 2
                    [xrb, idxb] = sort(xrb);
                    yrb = yrb(idxb);
                    [xrb, iab] = unique(xrb,'stable');
                    yrb = yrb(iab);
                    id_inb = isfinite(xf) & xf>0 & xf>=xrb(1) & xf<=xrb(end);
                    if any(id_inb)
                        ref_interp_b = 10.^interp1(log10(xrb), log10(yrb), log10(xf(id_inb)), 'linear');
                        ref_interp_b = ref_interp_b(:);
                        calc_interp_b = Gzx2_mag(id_inb);
                        calc_interp_b = calc_interp_b(:);
                        rel_b = abs(calc_interp_b - ref_interp_b) ./ max(ref_interp_b, realmin);
                        alpha_b = (calc_interp_b.'*ref_interp_b) / max(calc_interp_b.'*calc_interp_b, realmin);
                        alpha_b = max(alpha_b, realmin);
                        rel_b_s = abs(alpha_b*calc_interp_b - ref_interp_b) ./ max(ref_interp_b, realmin);

                        fprintf('Gzx(exp1): rel.mean=%g (base), rel.mean=%g (swap-z)\n', mean(rel_a), mean(rel_b));
                        fprintf('Gzx(exp1): rel.median=%g (base), rel.median=%g (swap-z)\n', median(rel_a), median(rel_b));
                        fprintf('Gzx(exp1): rel.max=%g (base), rel.max=%g (swap-z)\n', max(rel_a), max(rel_b));
                        fprintf('Gzx(exp1): alpha=%g (base), alpha=%g (swap-z)\n', alpha_a, alpha_b);
                        fprintf('Gzx(exp1,scaled): rel.mean=%g (base), rel.mean=%g (swap-z)\n', mean(rel_a_s), mean(rel_b_s));
                        [max_rel_a, idx_max_a] = max(rel_a);
                        xfa = xf(id_ina);
                        fprintf('Gzx(exp1): max@xf=%g (base), rel=%g, calc=%g, ref=%g\n', xfa(idx_max_a), max_rel_a, calc_interp_a(idx_max_a), ref_interp_a(idx_max_a));
                        [max_rel_b, idx_max_b] = max(rel_b);
                        xfb = xf(id_inb);
                        fprintf('Gzx(exp1): max@xf=%g (swap-z), rel=%g, calc=%g, ref=%g\n', xfb(idx_max_b), max_rel_b, calc_interp_b(idx_max_b), ref_interp_b(idx_max_b));
                    end
                end
            end
        end
    end
end

