% NODE_PMERGE   Generating P-matricx for megerged network (common nodes)
%              
% Pmtx, Pmtx_Rev            Potential coef.(originaland merged P matrix)
% Node_List, Node_List_Rev  Node name (original and merged list)
% Node_Com                  list of common nodes
% Node_Del                  common nodes to be deleted
% rad len                   N*1 vector (radius and length)
%
% Revised April 20, 2018
%[Nr,Pr]=NODE_PMERGE(nod_name,flag,common_delete,re_2,len_2,Pmtx);
function [Nr,Pr]=NODE_PMERGE(Node,Node_Com,Node_Del,rad,len,Pmtx)
Pr=Pmtx;
Nr=Node;
[NP,n0]=size(Pr);               % NS # of comm node set
[NS,Ni]=size(Node_Com);         % NS # of comm node set


A=2*pi*rad.*len;      % surface area of node branches
AsAf=A*A';                      % As * Af (area product matrix)
Pr=Pr.*AsAf;                     % potential matrix'
B=A;                            % area with merged codes

% (1) Generating Prev after common nodes are megered********************
for ik=1:NS
    node_tmp=Node_Com(ik,1:Ni);
    node_t=find(node_tmp>0);        % index of common list
    if length(node_t)==1
        continue;
    end
    node_dex=node_tmp(node_t);
    node_id=node_dex(1);            % index to be kept

% updating P row
    ps=Pr(node_dex,1:NP);
    Prow=sum(ps,1);                 % merged potential (row)
    Pr(node_id,1:NP)=Prow;          % updating p row
    
% updating P col.
    ps=Pr(1:NP,node_dex);
    Pcol=sum(ps,2);                 % merged potential (row)
    Pr(1:NP,node_id)=Pcol;          % updating p row

% updating area list
    B(node_id)=sum(B(node_dex));
end
AsAf=B*B';                          % updated area product matrix
Pr=Pr./AsAf;

% (2) Deleting common row and low ********************
Pr(Node_Del,:)=[];
Pr(:,Node_Del)=[];

Nr(Node_Del,:)=[];
end