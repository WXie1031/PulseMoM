%
% Function to compute the distances between conductors
%
% Input
%      Geom - line geometry data
%
% Output
%      Dij - distances from conductors to the images
%      dij - direct distances between conductors
%      hij - perperdicular distances between conductors and their images
%
%  The function is programed as follows
%
%      [Dij,dij,hij]=Height(Geom)
%
function[Dij,dij,hij]=Height(Geom)

Ls   = Geom(max(Geom(:,1)),1);   % Limit of the iterative routine to compute the equivalent radii of each conductor
Req  = zeros(Ls,1);              % Initialize in zeros the equivalent radii

% routine to compute the equivalent radii of each conductor
k4  = sqrt(2*(Geom(:,6)/2).^2);   % Radii of the circle that circunscribe the haz conductors in m 
for nc = 1: Ls;
    if Geom(nc,5)==1
        Req(nc) = Geom(nc,4);     % When one has only a single conductor
    else
        Req(nc) = (Geom(nc,4).*Geom(nc,5).*k4(nc).^(Geom(nc,5)-1)).^(1./Geom(nc,5));  % When one haz more than one conductor
    end
end

% Direct distances between conductors
for xl = 1:Ls;
    for yl = 1:Ls;
        if xl==yl
            dij(xl,yl)=Req(xl);
        else
            x=abs(Geom(yl,2)-Geom(xl,2));
            y=abs(Geom(yl,3)-Geom(xl,3));
            dij(xl,yl)=sqrt(x^2 + y^2);
        end
    end
end

% Perpendicular height between conductors and the images
for xl = 1:Ls;
    for yl = 1:Ls;
        if xl==yl
            y1=Geom(yl,3);
            hij(xl,yl)=2*y1;
        else
            y1=Geom(xl,3);
            y2=Geom(yl,3);
            hij(xl,yl)=y1+y2;
        end
    end
end

% Distances between conductors and images
for xl = 1:Ls;
    for yl = 1:Ls;
        if xl==yl
            Dij(xl,yl)=hij(xl,yl);
        else
            x=abs(Geom(yl,2)-Geom(xl,2));
            y=hij(xl,yl);
            Dij(xl,yl)=sqrt(x^2 + y^2);
        end
    end
end
