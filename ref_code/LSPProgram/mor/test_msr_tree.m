load('WT_paper_1.mat')

Amtx = sol_a_mtx(nod_start, nod_end, nod_name);

nod_sr = nod_out(end,:);

msr_tree(Amtx, Lmtx, nod_start, nod_end, nod_name, nod_sr)

