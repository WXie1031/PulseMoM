% SEARCH_NO     return the index of cells in M so that position of field
%               pts fall witnin the zeon ind the direction of "dir"
% June 30 2011
%
function ldex=SEARCH_NO(Pl,Ps,M,dir) 
dd=Ps(dir);
w0=Pl(dir);
[N1 N0]=size(M);   

sg1=sign(M(1:N1,dir)-(+0.5*dd+w0));
sg2=sign(M(1:N1,dir)-(-0.5*dd+w0));
sg=sg1.*sg2;

id=1;
for i=1:N1
    if sg(i)<0
        ldex(id)=i;
        id=id+1;
    end
end

end