function [Rmtx, Lmtx, Rself, Lself] = update_mesh2d_to_main_pul(Rmtx, Lmtx, Rself, Lself, ...
    offset, Rmesh_pul, Lmesh_pul, Rmesh_self_pul, Lmesh_self_pul, shape, dim1, dim2, len)

Nc = size(Rmesh_pul,1);

[Rmesh, Lmesh] = update_mesh_para_on_length(Rmesh_pul, Lmesh_pul, ...
    shape, dim1, dim2, len);

Rmtx(offset+(1:Nc),offset+(1:Nc)) = Rmesh;
Lmtx(offset+(1:Nc),offset+(1:Nc)) = Lmesh;

[Rmesh_self, Lmesh_self] = update_self_para_on_length(Rmesh_self_pul, Lmesh_self_pul, ...
    shape, dim1, dim2, len);

Rself(offset+(1:Nc),offset+(1:Nf)) = Rmesh_self;
Lself(offset+(1:Nc),offset+(1:Nf)) = Lmesh_self;

end

