function [common_delete,flag,re_2,len_2] = commonnode(re,len,nod_name,Pmtx);
% Input:
%       re: radius
%       len: wire length
%       nod_name: name of nod
%       Pmtx: potential coefficient
% 
% Output:
%       common_delete
%       flag
%       re_2
%       len_2


Nn=size(nod_name,1);
common_node=[];
unique_node=unique(nod_name(:,:),'rows');
Nn2=size(unique_node,1);
flag=0;
ic=0;
for ia=1:Nn2
    for ib=1:Nn
        if nod_name(ib,:)==unique_node(ia,:);
            ic=ic+1;
            flag(ia,ic)=ib;   
%             flag2(ia)=ic;

        end

    end
    ic=0;
end
[flag_a flag_b]=size(flag);
if flag_b<2;
    flag2=[];
    common_delete=[];
    re_2=[re;re];
len_2=[len/2;len/2];
else
    flag2=flag(:,2:end);
    common_delete0=find(flag2>0);
    common_delete=flag2(common_delete0);
re_2=[re;re];
len_2=[len/2;len/2];
          
common_delete=sort(common_delete,1);
end



end


