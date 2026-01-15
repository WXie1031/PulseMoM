function Lext = induct_bar_ext(wid, hig, len)
%  Function:       induct_bar_ext
%  Description:    Calculate external inductance of the rectangle conductor 
%                  using concept L=ep0*mu0*inv(C). That means four surfaces
%                  intergral and combine to one.
%
%  Calls:          int_tape_p2d
%                  int_tape_v2d
%
%  Input:          wid    --  width of conductors (N*1) (m)
%                  hig    --  thick of conductors (N*1) (m)
%                  len    --  length of conductors (N*1)
%  Output:         Lext   --  Lext vector
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2016-05-06

mu0 = 4*pi*1e-7;

Nc = length(len);

PoM = zeros(4,2,Nc); % position of corner
for k = 1:Nc
    PoM(:,1,k) = [-wid(k)/2 -wid(k)/2 wid(k)/2 wid(k)/2]';
    PoM(:,2,k) = [-hig(k)/2 hig(k)/2 hig(k)/2 -hig(k)/2]';
end

Nm = 4;

Lext = zeros(Nc,1);
for k = 1:Nc 
    
    Mt = zeros(Nm,Nm);
    
    X1 = PoM(:,1,k);
    Y1 = PoM(:,2,k);

    X2 = [X1(2:end); X1(1)];
    Y2 = [Y1(2:end); Y1(1)];

    dX = X2 - X1;
    dY = Y2 - Y1;
    %C0 = sum(sqrt(dX.^2+dY.^2));
    
    Np = length(X1);
    for g = 1:Np-1
        for m = g+1:Np
            if (dX(g) == 0 && dX(m) == 0) 
                w1 = dY(g); % w1 = q4-q1;
                w2 = dY(m); % w2 = q3-q4;
                Mt(g,m) = mu0/(4*pi)./w1./w2.* ...
                    int_tape_p_p3d(Y1(g),Y2(g),X1(g),Y1(m),Y2(m),X1(m),len(k));
            elseif dY(g) == 0 && dY(m) == 0
                w1 = dX(g); % w1 = q4-q1;
                w2 = dX(m); % w2 = q3-q4;
                Mt(g,m) = mu0/(4*pi)./w1./w2.* ...
                    int_tape_p_p3d(X1(g),X2(g),Y1(g),X1(m),X2(m),Y1(m),len(k));
            elseif dX(g) == 0 && dY(m) == 0
                w1 = dY(g); % w1 = q1-q2;
                w2 = dX(m); % w2 = r1-r2;
                Mt(g,m) = mu0/(4*pi)./w1./w2.* ...
                    int_tape_v_p3d(Y1(g),Y2(g),X1(g),Y2(m),X1(m),X2(m),len(k));
            elseif dY(g) == 0 && dX(m) == 0
                w1 = dX(g); % w1 = q1-q2;
                w2 = dY(m); % w2 = r1-r2;
                Mt(g,m) = mu0/(4*pi)./w1./w2.* ...
                    int_tape_v_p3d(X1(g),X2(g),Y1(g),X2(m),Y1(m),Y2(m),len(k));
            end
        end
    end
    
    %wt = sqrt(dX.*dX+dY.*dY);
    wt = max(abs(dX),abs(dY));
    Lt = induct_tape(wt, len);
    Mt(:,:,k) = Mt+Mt'+diag(Lt);
    
    Lext(k) = 1./sum(sum(inv(Mt(:,:,k))));
end
    

end


