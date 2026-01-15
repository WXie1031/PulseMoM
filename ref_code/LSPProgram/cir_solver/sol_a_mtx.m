function Amtx = sol_a_mtx(nod_start, nod_end, nod, Asub)
%  Function:       sol_a_mtx
%  Description:    Generate Node bran matrix ( A matrix ) Ground is set to be
%                  node 0
%  Calls:
%
%  Input:          nod_start  --  start node name of all conductors
%                  nod_end    --  end node name of all conductors
%                  nod        --  all nodes
%  Output:         Amtx   --    A matrix
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2013-12-24
%  History:        Add string type input.
%                  2017-1-14


if isempty(nod_start) || isempty(nod)
    Amtx=[];
elseif isnumeric(nod_start)
    Nb = size(nod_start,1);
    Nn = size(nod,1);
    
    Amtx = zeros(Nn,Nb);
    
    for ik = 1:Nb
        Amtx(nod==nod_start(ik,:),ik) = 1;
        Amtx(nod==nod_end(ik,:),ik) = -1;
    end
else
    Nb = size(nod_start,1);
    Nn = size(nod, 1);
    
    Amtx = zeros(Nn,Nb);
    
    for ik = 1:Nb
        for ig = 1:Nn
            if strcmp(nod_start(ik,:),nod(ig,:))
                Amtx(ig,ik) = 1;
            end
            if strcmp(nod_end(ik,:),nod(ig,:))
                Amtx(ig,ik) = -1;
            end
        end
    end
end


if nargin > 3
    Amtx = [Amtx Asub];
end

end


