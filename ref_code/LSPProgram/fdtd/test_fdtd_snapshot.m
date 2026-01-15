
fname = 'Station_grid_hit2_short_conn_snap.dat';
vname = 'hit2_short_e_m1_4.txt';

% duke_RBS_square_grid_rod_snap  rbs_rod_v_m1 
% duke_RBS_square_grid_snap      rbs_v_m1
%   
% Ex,Ey,Ez -- 1 mid,  4 o plane
 pt_m = [-2.5,-2.5];
 tdisplay = 5; %s

% Station_grid_hit2_snap             hit2_v_m2
% Station_grid_hit2_ver_rod_snap     hit2_ver_v_m2
% Station_grid_hit2_short_conn_snap  hit2_short_v_m2
% Station_grid_hit2_no_conn_snap     hit2_no_v_m2

% Ez -- 1 mid, 2 0 plane
% pt_m = [110,230];
  pt_m = [110,180];
 tdisplay = 99.9; %s
 tdisplay = 15; %s
  
snap_id = 1;
display_id = 2;

if ~isempty(vname)
    
%     pt_m = [110,230];
%     tdisplay = 15; %s
    [vout, t] = fdtd_ez2v(vname, 0.05,4);
    %[v_pt, t] = fdtd_v(vname); % t is in us
    v_pt = -vout(2,:)/10;
    figure(11);
    plot(t,v_pt);
else
    tdisplay = [];
    v_pt = [];
    t = [];
end


%% flag setting and reading snapshot
Ex = 1;     %%%%% bit mask for each component
Ey = 2;     
Ez = 4;
Hx = 8;
Hy = 16;
Hz = 32;

%%%%% in this case, we know we capture "Ex, Ey & Ez", so, here, we enable
%%%%% these mask
% mask_for_case = bitor( bitor(Ex, Ey), Ez );

mask_for_case = Ez;

[snapshotFun] = fdtd_read_snapshot(fname, mask_for_case);


%% here, we will show the Edisplay snapshot in a 2D plane
Edisplay = snapshotFun{snap_id}.Ez;

Ndisplay = size( Edisplay,3 );   %%% get transient trace length

if ~isempty(vname)
    display_id = floor(Ndisplay*tdisplay/max(t));
end

Efinal = (Edisplay(:,:,display_id));
Eoff = -Efinal(1,end);

Efinal = Efinal+Eoff;

figure(31);
mesh( snapshotFun{snap_id}.x,snapshotFun{snap_id}.y,  Efinal/1e3 );
zlabel('Ez (kV/m)')
axis equal

if ~isempty(vname)
    [~, dim1_id] = min( abs(pt_m(1)-snapshotFun{snap_id}.x(1,:)) );
    [~, dim2_id] = min( abs(pt_m(2)-snapshotFun{snap_id}.y(:,1)) );
    
    [~,t_id] = min(abs(display_id/Ndisplay*max(t)-t));
    
    v_pt_t = v_pt(t_id);
    Pfinal = v_pt_t/Efinal(dim2_id,dim1_id)*Efinal;
     Pfinal = max(Pfinal,-50e3);
    
    figure(32);
    surf( snapshotFun{snap_id}.x, snapshotFun{snap_id}.y, Pfinal/1e3, 'linestyle', 'none');
    colormap jet
    zlabel('Potential (kV)')
     caxis([-50 200])
   % axis equal
   xlabel('Y(m)')
   ylabel('X(m)')
%    xlim([0 160]);
%    ylim([0 240]);
   
    h = rotate3d;
    h.RotateStyle = 'box';

    %axes('Xgrid','off','Ygrid','off','GridLineStyle','--','GridAlpha',0.8)
        

    figure(33);
    contour( snapshotFun{snap_id}.x,  snapshotFun{snap_id}.y,Pfinal/1e3);
    colormap jet
    zlabel('Potential (kV)')
    axis equal
    
    xlabel('Y(m)')
    ylabel('X(m)')
    xlim =([0 160]);
    ylim =([0 240]);
end

display_id


