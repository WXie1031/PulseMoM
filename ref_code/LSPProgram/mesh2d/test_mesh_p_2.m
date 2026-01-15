clear

 dim1 = [1e-4; 1.8e-4];
 dim2 = [0;    0 ];
 pt_2d = [0  0;
     0    3e-4;];
  
%   dim1 = [2e-4];
%  dim2 = [    0 ];
%  pt_2d = [0  0; ];
%  
Nc = size(dim1,1);

 epr =  [1;]*ones(Nc,1);

 shp_id = ones(Nc,1);
 
 S = pi.*(dim1.^2-dim2.^2);
 sig  = repmat(5.8001e7,Nc,1); 
 Rpul = 1./(sig.*S);

 len = 1;
 frq = 1e4;
 mur = ones(Nc,1);

 p_flag = 1;

[~,~,Pmesh] = main_mesh2d_cmplt( ...
    shp_id, pt_2d, dim1, dim2, Rpul, sig,mur,epr, len, frq, p_flag);


pt_start = [pt_2d zeros(Nc,1)];
pt_end = [pt_2d len*ones(Nc,1)];
[dv,len] = line_dv(pt_start,pt_end);
re = dim1;

[~,~,Pmtx] = para_main_fila_rlp(pt_start,pt_end,dv,re,len, ...
    pt_start,pt_end,dv,re,len,p_flag);
Pmtx = full(Pmtx);
Cmtx = inv(Pmtx);
