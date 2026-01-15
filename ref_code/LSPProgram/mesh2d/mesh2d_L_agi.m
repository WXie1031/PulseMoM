function [Xs, Ys, ws, hs, dS, Rps, NVsr] = mesh2d_L_agi(pt_2d, wo, to, Ro, f0)
%  Function:       mesh2D_angle_steel
%  Description:    Mesh angle steel cross sections (2D). Angle steel is
%                  divided into 3 boxes.
%  Calls:          mesh2D_box
%  Input:          p2D  --  coordinate of source line (2D cross section)
%                  wo   --  width of the conductor
%                  ho   --  higth of the conductor
%                  lo   --  length of the conductor
%                  Ro   --  resistivity of the conductor (ohm/m)
%                  Sig  --  coordinate of end point of field line
%                  f0   --  frequency
%  Output:         Xs   --  axis of the angle iron segments
%                  ws   --  width of the segments(x-axis)
%                  hs   --  higth of the segments(y-axis)
%                  Rs   --  resistance of the segments (ohm/m)
%                  ls   --  length of the segments
%                  NVsr --  (vector)the number of the segments for each rectangle
%  Others:         
%  Author:         Chen Hongcai    
%  Email :         hc.chen@live.com      
%  Date:           2013-11-13
%  History:         


Nc = size(pt_2d,1);

rec = zeros(3,2);
Xs = zeros(0,1);
Ys = zeros(0,1);
ws = zeros(0,1);
hs = zeros(0,1);
dS = zeros(0,1);
Rps = zeros(0,1);
NVsr = zeros(Nc,1);
%% segment method
for k = 1:Nc

    if wo(k)>0 && to(k)>0  
        rec(1,:) = [(wo(k)+to(k))/2  to(k)/2];
        rec(2,:) = [to(k)/2          to(k)/2];
        rec(3,:) = [to(k)/2         (wo(k)+to(k))/2];
        
    elseif wo(k)>0 && to(k)<0  
        rec(1,:) = [(wo(k)-to(k))/2  to(k)/2];
        rec(2,:) = [to(k)/2          to(k)/2];
        rec(3,:) = [-to(k)/2         -(wo(k)-to(k))/2];

    elseif wo(k)<0 && to(k)>0    
        rec(1,:) = [(wo(k)-to(k))/2  to(k)/2];
        rec(2,:) = [-to(k)/2          to(k)/2];
        rec(3,:) = [-to(k)/2          (-wo(k)+to(k))/2];

    elseif wo(k)<0 && to(k)<0    
        rec(1,:) = [(wo(k)+to(k))/2  to(k)/2];
        rec(2,:) = [to(k)/2          to(k)/2];
        rec(3,:) = [to(k)/2          (wo(k)+to(k))/2];
    end
    
    Xrec = pt_2d(k,1)+rec(:,1);
 	Yrec = pt_2d(k,2)+rec(:,2);
    p2Drec = [Xrec  Yrec];
    tseg = abs(to(k));
    wseg = abs(wo(k))-tseg;

    wrec = [wseg; tseg; tseg;];
    hrec = [tseg; tseg; wseg;];
    Rorec = Ro(k)*(wo(k)^2-(abs(wo(k))-abs(to(k)))^2)./(wrec.*hrec);

    
    [Xtmp, Ytmp, wtmp, htmp, dStmp, Rpstmp, NVtmp] = mesh2d_L_box( p2Drec, ...
        wrec, hrec, Rorec, f0 );
        
    Xs = [Xs; Xtmp;];
    Ys = [Ys; Ytmp;];
    ws = [ws; wtmp;];
    hs = [hs; htmp;];
    dS = [dS; dStmp;];
    Rps = [Rps; Rpstmp];

    NVsr(k) = sum(sum(NVtmp));

    %clear Xtmp Ytmp wtmp htmp ltmp dStmp Rstmp NVtmp
end



end


