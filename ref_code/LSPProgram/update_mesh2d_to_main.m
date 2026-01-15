function [Rmtx,Lmtx] = update_mesh2d_to_main(Rmtx, Lmtx, ...
    offset, Rmesh,Lmesh)

Nc = size(Rmesh,1);

Rmtx(offset+(1:Nc),offset+(1:Nc)) = Rmesh(1:Nc,1:Nc);
Lmtx(offset+(1:Nc),offset+(1:Nc)) = Lmesh(1:Nc,1:Nc);


end

