% TAPE_2_TAPE_P     working horse for inte(1/R, x, y,  x' y')
% TAPE_2_TAPE_Pa    working horse for inte(1/R, x, y,  x' y') -- (x' y')
%                   parallel tapes
% xf=[x1 x2]  yf=[y1 y2] xs=[x'1 x'2]  ys=[y'1 y'2]   2-col vectors
% zf  zs                                              single value
% Dec. 2015
%
function f=G_T2TP(xf,yf,zf,xs,ys,zs)
[nf m]=size(xf);
[ns m]=size(xs);
[nzf m]=size(zf);
[nzs m]=size(zs);

DZ=repmat(zf(1:nzf,1),1,nzs)-repmat(zs(1:nzs,1)',nzf,1);

YF1=repmat(yf(1:nf,1),1,ns);
YF2=repmat(yf(1:nf,2),1,ns);
YS1=repmat(ys(1:ns,1)',nf,1);
YS2=repmat(ys(1:ns,2)',nf,1);

% 2nd level integration (x x')
for ik=1:nzf
    for jk=1:nzs
        dz=DZ(ik,jk);
% 11
        dy=YF1-YS1;
        f11=G_T2TP3(xf,xs,dy,dz);
% 12
        dy=YF1-YS2;
        f12=G_T2TP3(xf,xs,dy,dz);
% 21
        dy=YF2-YS1;
        f21=G_T2TP3(xf,xs,dy,dz);
% 22
        dy=YF2-YS2;
        f22=G_T2TP3(xf,xs,dy,dz);
      
        f(1:nf,1:ns,ik,jk)=f11-f12-f21+f22;
    end
end

