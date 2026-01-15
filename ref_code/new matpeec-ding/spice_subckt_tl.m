function spice_subckt_tl(fname,fpath,line_num,sect_num,Zmt,Zm,Y, Rg0, Lg0, Rgn, Lgn,tl_nod_start,tl_nod_end,tl_name,sect_length,group_num)
% function generate .cir file of transmission lines DEPACT model

gn=group_num;
TD0=sect_length/3e8;
% line_num=3;
% sect_num=4;
% fname='test6a';
% fpath='C:\Users\Ding\Desktop\test6\';
% tl_nod_start=['tl_start1';'tl_start2';'tl_start3'];
% tl_nod_end=['tl_end1';'tl_end2';'tl_end3'];
line_num=size(tl_nod_start,1);
%-----------------------------------------------------
switch line_num
    case 1
        
        tl_start1=tl_nod_start(1,:);
        tl_end1=tl_nod_end(1,:);

        
    case 2
        tl_start1=tl_nod_start(1,:);
        tl_start2=tl_nod_start(2,:);

        tl_end1=tl_nod_end(1,:);
        tl_end2=tl_nod_end(2,:);

    case 3    
        tl_start1=tl_nod_start(1,:);
        tl_start2=tl_nod_start(2,:);
        tl_start3=tl_nod_start(3,:);
        tl_end1=tl_nod_end(1,:);
        tl_end2=tl_nod_end(2,:);
        tl_end3=tl_nod_end(3,:);
end
        
%-----------------------------------------------------        
        
tl_nod_start(1,:)

fid = fopen([fpath, fname,'.cir'],'w+');
fprintf(fid,'* PSpice Model Editor - Ding test1 \n');
switch line_num
    case 1
for ia=1:line_num
    for ib=1:sect_num
fprintf(fid,'.SUBCKT L%d%dA_G%d L%d%dA1_G%d L%d%dA2_G%d L%d%dA3_G%d L%d%dA4_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,'I_I%d%d_1_G%d  L%d%dA2_G%d 0 STIMULUS=U_IND_L%d%d0001_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,group_num);
fprintf(fid,'I_I%d%d_2_G%d  L%d%dA3_G%d 0  STIMULUS=U_IND_L%d%d0002_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,group_num);
fprintf(fid,'R_R%d%d_2_G%d  L%d%dA2_G%d 0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,Zmt(ia,ia));
fprintf(fid,'R_R%d%d_3_G%d  L%d%dA3_G%d 0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,Zmt(ia,ia));
fprintf(fid,'G_G%d%d_1_G%d  L%d%dA2_G%d  0  N%d%d_E1_3_G%d  0 1 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'G_G%d%d_2_G%d  L%d%dA3_G%d  0  N%d%d_E2_3_G%d  0 1\n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'X_F%d%d_1_G%d  N%d%d_F3_4_G%d  0  L%d%dA2_G%d  0 SCHEMATIC1_F%d%d_1_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'X_F%d%d_2_G%d  N%d%d_F4_4_G%d  0  L%d%dA3_G%d  0 SCHEMATIC1_F%d%d_2_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'T_T%d%d_1_G%d  N%d%d_E1_2_G%d  0  N%d%d_E1_3_G%d  0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);
fprintf(fid,'T_T%d%d_2_G%d  N%d%d_F3_2_G%d  0  N%d%d_F3_3_G%d  0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);
fprintf(fid,'T_T%d%d_3_G%d  N%d%d_E2_2_G%d  0  N%d%d_E2_3_G%d  0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);
fprintf(fid,'T_T%d%d_4_G%d  N%d%d_F4_2_G%d  0  N%d%d_F4_3_G%d 0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);
fprintf(fid,'E_E%d%d_1_G%d  N%d%d_E1_1_G%d  0  L%d%dA4_G%d  n_R%d%d_14_G%d %d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,-1/Zm(ia,ia));
fprintf(fid,'X_F%d%d_3_G%d  L%d%dA3_G%d  L%d%dA4_G%d  N%d%d_F3_1_G%d  0 SCHEMATIC1_F%d%d_3_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'X_F%d%d_4_G%d  L%d%dA1_G%d  L%d%dA2_G%d  N%d%d_F4_1_G%d  0 SCHEMATIC1_F%d%d_4_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'E_E%d%d_2_G%d  N%d%d_E2_1_G%d  0  L%d%dA1_G%d  n_R%d%d_13_G%d %d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,-1/Zm(ia,ia));
fprintf(fid,'R_R%d%d_5_G%d  N%d%d_E1_1_G%d  N%d%d_E1_2_G%d  0.0001 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_6_G%d  N%d%d_E1_3_G%d  0  1 TC=0,0 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_7_G%d  N%d%d_F3_1_G%d  N%d%d_F3_2_G%d  1 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_8_G%d  N%d%d_F3_3_G%d  N%d%d_F3_4_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_9_G%d  N%d%d_F4_1_G%d  N%d%d_F4_2_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn); 
fprintf(fid,'R_R%d%d_10_G%d  N%d%d_F4_3_G%d  N%d%d_F4_4_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_11_G%d  N%d%d_E2_3_G%d  0  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_12_G%d  N%d%d_E2_1_G%d  N%d%d_E2_2_G%d  0.0001 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_13_G%d  n_R%d%d_13_G%d  0 1G TC=0,0  \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_14_G%d  n_R%d%d_14_G%d  0 1G TC=0,0  \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'.ENDS  L%d%dA_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_1_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_1_G%d  3 4 VF_F%d%d_1_G%d -1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_1_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_1_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_2_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_2_G%d  3 4 VF_F%d%d_2_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_2_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_2_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_3_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_3_G%d  3 4 VF_F%d%d_3_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_3_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_3_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_4_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_4_G%d  3 4 VF_F%d%d_4_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_4_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_4_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,' \n');
    end
end

    case 2
        
            for ib=1:sect_num
        fprintf(fid,'.SUBCKT L%dA_G%d L1%dA1_G%d L2%dA1_G%d L1%dA4_G%d L2%dA4_G%d \n',ib,gn,ib,gn,ib,gn,ib,gn,ib,gn);
          for ia=1:line_num
fprintf(fid,' \n');
fprintf(fid,'I_I%d%d_1_G%d  L%d%dA2_G%d 0 STIMULUS=U_IND_L%d%d0001_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,group_num);
fprintf(fid,'I_I%d%d_2_G%d  L%d%dA3_G%d 0  STIMULUS=U_IND_L%d%d0002_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,group_num);
fprintf(fid,'R_R%d%d_2_G%d  L%d%dA2_G%d 0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,Zmt(ia,ia));
fprintf(fid,'R_R%d%d_3_G%d  L%d%dA3_G%d 0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,Zmt(ia,ia));

fprintf(fid,'X_F%d%d_1_G%d  N%d%d_F3_4_G%d  0  L%d%dA2_G%d  0 SCHEMATIC1_F%d%d_1_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'X_F%d%d_2_G%d  N%d%d_F4_4_G%d  0  L%d%dA3_G%d  0 SCHEMATIC1_F%d%d_2_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);

fprintf(fid,'T_T%d%d_2_G%d  N%d%d_F3_2_G%d  0  N%d%d_F3_3_G%d  0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);
fprintf(fid,'T_T%d%d_4_G%d  N%d%d_F4_2_G%d  0  N%d%d_F4_3_G%d  0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);

fprintf(fid,'X_F%d%d_3_G%d  L%d%dA3_G%d  L%d%dA4_G%d  N%d%d_F3_1_G%d  0 SCHEMATIC1_F%d%d_3_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'X_F%d%d_4_G%d  L%d%dA1_G%d  L%d%dA2_G%d  N%d%d_F4_1_G%d  0 SCHEMATIC1_F%d%d_4_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);

fprintf(fid,'R_R%d%d_7_G%d  N%d%d_F3_1_G%d  N%d%d_F3_2_G%d  1 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_8_G%d  N%d%d_F3_3_G%d  N%d%d_F3_4_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_9_G%d  N%d%d_F4_1_G%d  N%d%d_F4_2_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn); 
fprintf(fid,'R_R%d%d_10_G%d  N%d%d_F4_3_G%d  N%d%d_F4_4_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);

fprintf(fid,'R_R%d%d_13_G%d  n_R%d%d_13_G%d  0 1G TC=0,0  \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_14_G%d  n_R%d%d_14_G%d  0 1G TC=0,0  \n',ia,ib,gn,ia,ib,gn);

          end
                  
        fprintf(fid,'R_R12_%dA2_G%d  L1%dA2_G%d  L2%dA2_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(1,2));
        fprintf(fid,' \n');  
        fprintf(fid,'R_R12_%dA3_G%d  L1%dA3_G%d  L2%dA3_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(1,2));
        fprintf(fid,' \n');   
        
                  for ic=1:2
              for id=1:2
                  fprintf(fid,'E_E%d%d_%d2_G%d  N%d%d_E%d2_1_G%d  0  L%d%dA1_G%d  n_R%d%d_13_G%d %d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,gn,ic,ib,gn,-Y(ic,id));
                  fprintf(fid,'R_N%d%d_E%d2_1_G%d  N%d%d_E%d2_1_G%d  N%d%d_E%d2_2_G%d  0.0001 TC=0,0  \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'T_T%d%d_%d2_G%d  N%d%d_E%d2_2_G%d  0  N%d%d_E%d2_3_G%d  0 Z0=1 TD=%d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn,TD0);
                  fprintf(fid,'R_N%d%d_E%d2_2_G%d  N%d%d_E%d2_3_G%d  0  1 TC=0,0 \n',ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'G_G%d%d_%d2_G%d  L%d%dA3_G%d  0  N%d%d_E%d2_3_G%d  0 1 \n',ic,ib,id,gn,id,ib,gn,ic,ib,id,gn);
                  fprintf(fid,' \n');                

                  fprintf(fid,'E_E%d%d_%d1_G%d  N%d%d_E%d1_1_G%d  0  L%d%dA4_G%d  n_R%d%d_14_G%d %d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,gn,ic,ib,gn,-Y(ic,id));
                  fprintf(fid,'R_N%d%d_E%d1_1_G%d  N%d%d_E%d1_1_G%d  N%d%d_E%d1_2_G%d  0.0001 TC=0,0  \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'T_T%d%d_%d1_G%d  N%d%d_E%d1_2_G%d  0  N%d%d_E%d1_3_G%d  0 Z0=1 TD=%d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn,TD0);
                  fprintf(fid,'R_N%d%d_E%d1_2_G%d  N%d%d_E%d1_3_G%d  0  1 TC=0,0 \n',ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'G_G%d%d_%d1_G%d  L%d%dA2_G%d  0  N%d%d_E%d1_3_G%d  0 1 \n',ic,ib,id,gn,id,ib,gn,ic,ib,id,gn);
                  fprintf(fid,' \n'); 
              end
          end
          fprintf(fid,'.ENDS  L%dA_G%d \n',ib,gn);
            end
            for ia=1:line_num
    for ib=1:sect_num

fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_1_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_1_G%d  3 4 VF_F%d%d_1_G%d -1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_1_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_1_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_2_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_2_G%d  3 4 VF_F%d%d_2_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_2_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_2_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_3_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_3_G%d  3 4 VF_F%d%d_3_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_3_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_3_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_4_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_4_G%d  3 4 VF_F%d%d_4_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_4_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_4_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
    end
end
        
    case 3
      
    for ib=1:sect_num
        fprintf(fid,'.SUBCKT L%dA_G%d L1%dA1_G%d L2%dA1_G%d L3%dA1_G%d L1%dA4_G%d L2%dA4_G%d L3%dA4_G%d \n',ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn);
          for ia=1:line_num
fprintf(fid,' \n');
fprintf(fid,'I_I%d%d_1_G%d  L%d%dA2_G%d 0 STIMULUS=U_IND_L%d%d0001_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,group_num);
fprintf(fid,'I_I%d%d_2_G%d  L%d%dA3_G%d 0  STIMULUS=U_IND_L%d%d0002_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,group_num);
fprintf(fid,'R_R%d%d_2_G%d  L%d%dA2_G%d 0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,Zmt(ia,ia));
fprintf(fid,'R_R%d%d_3_G%d  L%d%dA3_G%d 0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,Zmt(ia,ia));

fprintf(fid,'X_F%d%d_1_G%d  N%d%d_F3_4_G%d  0  L%d%dA2_G%d  0 SCHEMATIC1_F%d%d_1_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'X_F%d%d_2_G%d  N%d%d_F4_4_G%d  0  L%d%dA3_G%d  0 SCHEMATIC1_F%d%d_2_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);

fprintf(fid,'T_T%d%d_2_G%d  N%d%d_F3_2_G%d  0  N%d%d_F3_3_G%d  0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);
fprintf(fid,'T_T%d%d_4_G%d  N%d%d_F4_2_G%d  0  N%d%d_F4_3_G%d  0 Z0=1 TD=%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,TD0);

fprintf(fid,'X_F%d%d_3_G%d  L%d%dA3_G%d  L%d%dA4_G%d  N%d%d_F3_1_G%d  0 SCHEMATIC1_F%d%d_3_G%d \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'X_F%d%d_4_G%d  L%d%dA1_G%d  L%d%dA2_G%d  N%d%d_F4_1_G%d  0 SCHEMATIC1_F%d%d_4_G%d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn,ia,ib,gn);

fprintf(fid,'R_R%d%d_7_G%d  N%d%d_F3_1_G%d  N%d%d_F3_2_G%d  1 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_8_G%d  N%d%d_F3_3_G%d  N%d%d_F3_4_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_9_G%d  N%d%d_F4_1_G%d  N%d%d_F4_2_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn); 
fprintf(fid,'R_R%d%d_10_G%d  N%d%d_F4_3_G%d  N%d%d_F4_4_G%d  1 TC=0,0  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);

fprintf(fid,'R_R%d%d_13_G%d  n_R%d%d_13_G%d  0 1G TC=0,0  \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'R_R%d%d_14_G%d  n_R%d%d_14_G%d  0 1G TC=0,0  \n',ia,ib,gn,ia,ib,gn);



          end
          
        fprintf(fid,'R_R12_%dA2_G%d  L1%dA2_G%d  L2%dA2_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(1,2));
        fprintf(fid,'R_R23_%dA2_G%d  L2%dA2_G%d  L3%dA2_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(2,3));
        fprintf(fid,'R_R31_%dA2_G%d  L3%dA2_G%d  L1%dA2_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(3,1));
        fprintf(fid,' \n');  
        fprintf(fid,'R_R12_%dA3_G%d  L1%dA3_G%d  L2%dA3_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(1,2));
        fprintf(fid,'R_R23_%dA3_G%d  L2%dA3_G%d  L3%dA3_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(2,3));
        fprintf(fid,'R_R31_%dA3_G%d  L3%dA3_G%d  L1%dA3_G%d  %d TC=0,0 \n',ib,gn,ib,gn,ib,gn,Zmt(3,1));
        fprintf(fid,' \n');   
          for ic=1:3
              for id=1:3
                  fprintf(fid,'E_E%d%d_%d2_G%d  N%d%d_E%d2_1_G%d  0  L%d%dA1_G%d  n_R%d%d_13_G%d %d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,gn,ic,ib,gn,-Y(ic,id));
                  fprintf(fid,'R_N%d%d_E%d2_1_G%d  N%d%d_E%d2_1_G%d  N%d%d_E%d2_2_G%d  0.0001 TC=0,0  \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'T_T%d%d_%d2_G%d  N%d%d_E%d2_2_G%d  0  N%d%d_E%d2_3_G%d  0 Z0=1 TD=%d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn,TD0);
                  fprintf(fid,'R_N%d%d_E%d2_2_G%d  N%d%d_E%d2_3_G%d  0  1 TC=0,0 \n',ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'G_G%d%d_%d2_G%d  L%d%dA3_G%d  0  N%d%d_E%d2_3_G%d  0 1 \n',ic,ib,id,gn,id,ib,gn,ic,ib,id,gn);
                  fprintf(fid,' \n');                

                  fprintf(fid,'E_E%d%d_%d1_G%d  N%d%d_E%d1_1_G%d  0  L%d%dA4_G%d  n_R%d%d_14_G%d %d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,gn,ic,ib,gn,-Y(ic,id));
                  fprintf(fid,'R_N%d%d_E%d1_1_G%d  N%d%d_E%d1_1_G%d  N%d%d_E%d1_2_G%d  0.0001 TC=0,0  \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'T_T%d%d_%d1_G%d  N%d%d_E%d1_2_G%d  0  N%d%d_E%d1_3_G%d  0 Z0=1 TD=%d \n',ic,ib,id,gn,ic,ib,id,gn,ic,ib,id,gn,TD0);
                  fprintf(fid,'R_N%d%d_E%d1_2_G%d  N%d%d_E%d1_3_G%d  0  1 TC=0,0 \n',ic,ib,id,gn,ic,ib,id,gn);
                  fprintf(fid,'G_G%d%d_%d1_G%d  L%d%dA2_G%d  0  N%d%d_E%d1_3_G%d  0 1 \n',ic,ib,id,gn,id,ib,gn,ic,ib,id,gn);
                  fprintf(fid,' \n'); 
              end
          end
          fprintf(fid,'.ENDS  L%dA_G%d \n',ib,gn);
    end
for ia=1:line_num
    for ib=1:sect_num

fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_1_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_1_G%d  3 4 VF_F%d%d_1_G%d -1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_1_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_1_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_2_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_2_G%d  3 4 VF_F%d%d_2_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_2_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_2_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_3_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_3_G%d  3 4 VF_F%d%d_3_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_3_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_3_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
fprintf(fid,'.subckt SCHEMATIC1_F%d%d_4_G%d 1 2 3 4  \n',ia,ib,gn);
fprintf(fid,'F_F%d%d_4_G%d  3 4 VF_F%d%d_4_G%d 1 \n',ia,ib,gn,ia,ib,gn);
fprintf(fid,'VF_F%d%d_4_G%d 1 2 0V \n',ia,ib,gn);
fprintf(fid,'.ends SCHEMATIC1_F%d%d_4_G%d \n',ia,ib,gn);
fprintf(fid,' \n');
fprintf(fid,' \n');
    end
end
end

        
        


% switch line_num
%     case 1
%         for ib=1:sect_num
%         
%         fprintf(fid,'R_R11_%dA2  L1%dA2  0  1G TC=0,0 \n',ib,ib);
%         fprintf(fid,'R_R11_%dA3  L1%dA3  0  1G TC=0,0 \n',ib,ib);
%         fprintf(fid,' \n');
%         fprintf(fid,' \n');       
%         end
%         
%     case 2
%         for ib=1:sect_num
%             fprintf(fid,'R_R12_%dA2  L1%dA2  L2%dA2  %d TC=0,0 \n',ib,ib,Zm(1,2));  
%             fprintf(fid,'R_R12_%dA3  L1%dA3  L2%dA3  %d TC=0,0 \n',ib,ib,Zm(1,2));
%             fprintf(fid,' \n');
%                
%         end
%     case 3
%         for ib=1:sect_num
%         fprintf(fid,'R_R12_%dA2  L1%dA2  L2%dA2  %d TC=0,0 \n',ib,ib,ib,Zm(1,2));
%         fprintf(fid,'R_R23_%dA2  L2%dA2  L3%dA2  %d TC=0,0 \n',ib,ib,ib,Zm(2,3));
%         fprintf(fid,'R_R31_%dA2  L3%dA2  L1%dA2  %d TC=0,0 \n',ib,ib,ib,Zm(3,1));
%         fprintf(fid,' \n');  
%         fprintf(fid,'R_R12_%dA2  L1%dA3  L2%dA3  %d TC=0,0 \n',ib,ib,ib,Zm(1,2));
%         fprintf(fid,'R_R23_%dA2  L2%dA3  L3%dA3  %d TC=0,0 \n',ib,ib,ib,Zm(2,3));
%         fprintf(fid,'R_R31_%dA2  L3%dA3  L1%dA3  %d TC=0,0 \n',ib,ib,ib,Zm(3,1));
%         fprintf(fid,' \n');
%         
%         end
% end


switch line_num
    case 1
        fprintf(fid,'.SUBCKT Zg_model1_G%d N_Zg1_G%d N_Zg2_G%d \n',gn,gn,gn);
        fprintf(fid,'R_Rv110_G%d N_Zg1_G%d nR_110_G%d %d TC=0,0 \n',gn,gn,gn,sect_length*Rg0);
        fprintf(fid,'L_LV110_G%d  nR_110_G%d  nL_110_G%d  %d \n',gn,gn,gn,sect_length*Lg0);
        fprintf(fid,'R_V111_G%d  nL_110_G%d  nV_111_G%d  %d  TC=0,0 \n',gn,gn,gn,sect_length*Rgn(1,1,1));
        fprintf(fid,'L_V111_G%d  nL_110_G%d  nV_111_G%d  %d \n',gn,gn,gn,sect_length*Lgn(1,1,1));
        fprintf(fid,'R_V112_G%d  nV_111_G%d  nV_112_G%d  %d  TC=0,0 \n',gn,gn,gn,sect_length*Rgn(1,1,2));
        fprintf(fid,'L_V112_G%d  nV_111_G%d  nV_112_G%d  %d  \n',gn,gn,gn,sect_length*Lgn(1,1,2));
        fprintf(fid,'R_V113_G%d  nV_112_G%d  N_Zg2_G%d  %d  TC=0,0  \n',gn,gn,gn,sect_length*Rgn(1,1,3));
        fprintf(fid,'L_V113_G%d  nV_112_G%d  N_Zg2_G%d  %d  \n',gn,gn,gn,sect_length*Lgn(1,1,3));
        fprintf(fid,'.ENDS  Zg_model1_G%d \n',gn);
        fprintf(fid,' \n');
        fprintf(fid,' \n');
        
    case 2
        fprintf(fid,'.SUBCKT Zg_model_G%d N1_Zg1_G%d N1_Zg2_G%d N2_Zg1_G%d N2_Zg2_G%d \n',gn,gn,gn,gn,gn);
        
               
        ia=1;
                fprintf(fid,'R_V%d%d0_G%d  N%d_Zg1_G%d  nR_%d%d0_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,gn,ia,ia,gn,sect_length*Rg0(ia,ia));
                fprintf(fid,'L_V%d%d0_G%d  nR_%d%d0_G%d  nL_%d%d0_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lg0(ia,ia));
                fprintf(fid,'R_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,1));
                fprintf(fid,'L_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,1));
                fprintf(fid,'R_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,2));
                fprintf(fid,'L_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,2));
                fprintf(fid,'R_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,3));
                fprintf(fid,'L_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,3)); 
                
                fprintf(fid,'X_Fm2%d_G%d  nV_%d%d3_G%d   n_Fm2%d_G%d  N2%d_1_G%d 0  SCHEMATIC1_Fm2%d_G%d \n',ia,gn,ia,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em2%d_G%d  n_Fm2%d_G%d N%d_Zg2_G%d  N2%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);
                
                
                        ia=2;
                fprintf(fid,'R_V%d%d0_G%d  N%d_Zg1_G%d  nR_%d%d0_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,gn,ia,ia,gn,sect_length*Rg0(ia,ia));
                fprintf(fid,'L_V%d%d0_G%d  nR_%d%d0_G%d  nL_%d%d0_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lg0(ia,ia));
                fprintf(fid,'R_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,1));
                fprintf(fid,'L_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,1));
                fprintf(fid,'R_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,2));
                fprintf(fid,'L_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,2));
                fprintf(fid,'R_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,3));
                fprintf(fid,'L_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,3)); 
                
                fprintf(fid,'X_Fm1%d_G%d  nV_%d%d3_G%d   n_Fm1%d_G%d  N1%d_1_G%d 0  SCHEMATIC1_Fm1%d_G%d \n',ia,gn,ia,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em1%d_G%d  n_Fm1%d_G%d  N%d_Zg2_G%d  N1%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);
                
                for ib=1:line_num
            for ia=1:line_num
                if ia==ib
                
                else 
                    fprintf(fid,'R_V%d%d0_G%d    N%d%d_1_G%d  nR_%d%d0_G%d  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Rg0(ia,ib));
                    fprintf(fid,'L_V%d%d0_G%d    nR_%d%d0_G%d  nL_%d%d0_G%d  %d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Lg0(ia,ib));
                    fprintf(fid,'R_V%d%d1_G%d    nL_%d%d0_G%d  nV_%d%d1_G%d  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Rgn(ia,ib,1));
                    fprintf(fid,'L_V%d%d1_G%d    nL_%d%d0_G%d  nV_%d%d1_G%d  %d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Lgn(ia,ib,1));
                    fprintf(fid,'R_V%d%d2_G%d    nV_%d%d1_G%d  nV_%d%d2_G%d  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Rgn(ia,ib,2));
                    fprintf(fid,'L_V%d%d2_G%d    nV_%d%d1_G%d  nV_%d%d2_G%d  %d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Lgn(ia,ib,2));
                    fprintf(fid,'R_V%d%d3_G%d    nV_%d%d2_G%d  0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,sect_length*Rgn(ia,ib,3));            
                    fprintf(fid,'L_V%d%d3_G%d    nV_%d%d2_G%d  0  %d  \n',ia,ib,gn,ia,ib,gn,sect_length*Lgn(ia,ib,3));
                    fprintf(fid,' \n');
                    
                end
            end
        end
        fprintf(fid,'.ENDS  Zg_model_G%d \n',gn);
        fprintf(fid,' \n'); 
        fprintf(fid,' \n'); 
        
        
        
    case 3
        fprintf(fid,'.SUBCKT Zg_model_G%d N1_Zg1_G%d N1_Zg2_G%d N2_Zg1_G%d N2_Zg2_G%d N3_Zg1_G%d N3_Zg2_G%d \n',gn,gn,gn,gn,gn,gn,gn);
        
               
        ia=1;
                fprintf(fid,'R_V%d%d0_G%d  N%d_Zg1_G%d  nR_%d%d0_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,gn,ia,ia,gn,sect_length*Rg0(ia,ia));
                fprintf(fid,'L_V%d%d0_G%d  nR_%d%d0_G%d  nL_%d%d0_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lg0(ia,ia));
                fprintf(fid,'R_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,1));
                fprintf(fid,'L_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,1));
                fprintf(fid,'R_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,2));
                fprintf(fid,'L_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,2));
                fprintf(fid,'R_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,3));
                fprintf(fid,'L_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,3)); 
                
                fprintf(fid,'X_Fm2%d_G%d  nV_%d%d3_G%d   n_Fm2%d_G%d  N2%d_1_G%d 0  SCHEMATIC1_Fm2%d \n',ia,gn,ia,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'X_Fm3%d_G%d  n_Fm2%d_G%d   n_Fm3%d_G%d  N3%d_1_G%d 0  SCHEMATIC1_Fm3%d \n',ia,gn,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em2%d_G%d  n_Fm3%d_G%d  n_Em2%d_G%d  N2%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em3%d_G%d  n_Em2%d_G%d  N%d_Zg2_G%d  N3%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);
                
                
                        ia=2;
                fprintf(fid,'R_V%d%d0_G%d  N%d_Zg1_G%d  nR_%d%d0_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,gn,ia,ia,gn,sect_length*Rg0(ia,ia));
                fprintf(fid,'L_V%d%d0_G%d  nR_%d%d0_G%d  nL_%d%d0_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lg0(ia,ia));
                fprintf(fid,'R_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,1));
                fprintf(fid,'L_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,1));
                fprintf(fid,'R_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,2));
                fprintf(fid,'L_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,2));
                fprintf(fid,'R_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,3));
                fprintf(fid,'L_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,3)); 
                
                fprintf(fid,'X_Fm1%d_G%d  nV_%d%d3_G%d   n_Fm1%d_G%d  N1%d_1_G%d 0  SCHEMATIC1_Fm1%d_G%d \n',ia,gn,ia,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'X_Fm3%d_G%d  n_Fm1%d_G%d   n_Fm3%d_G%d  N3%d_1_G%d 0  SCHEMATIC1_Fm3%d_G%d \n',ia,gn,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em1%d_G%d  n_Fm3%d_G%d  n_Em1%d_G%d  N1%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em3%d_G%d  n_Em1%d_G%d  N%d_Zg2_G%d  N3%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);
                
                
                        ia=3;
                fprintf(fid,'R_V%d%d0_G%d  N%d_Zg1_G%d  nR_%d%d0_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,gn,ia,ia,gn,sect_length*Rg0(ia,ia));
                fprintf(fid,'L_V%d%d0_G%d  nR_%d%d0_G%d  nL_%d%d0_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lg0(ia,ia));
                fprintf(fid,'R_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,1));
                fprintf(fid,'L_V%d%d1_G%d  nL_%d%d0_G%d  nV_%d%d1_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,1));
                fprintf(fid,'R_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,2));
                fprintf(fid,'L_V%d%d2_G%d  nV_%d%d1_G%d  nV_%d%d2_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,2));
                fprintf(fid,'R_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  TC=0,0 \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Rgn(ia,ia,3));
                fprintf(fid,'L_V%d%d3_G%d  nV_%d%d2_G%d  nV_%d%d3_G%d  %d  \n',ia,ia,gn,ia,ia,gn,ia,ia,gn,sect_length*Lgn(ia,ia,3)); 
                
                fprintf(fid,'X_Fm2%d_G%d  nV_%d%d3_G%d   n_Fm2%d_G%d  N2%d_1_G%d 0  SCHEMATIC1_Fm2%d_G%d \n',ia,gn,ia,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'X_Fm1%d_G%d  n_Fm2%d_G%d   n_Fm1%d_G%d  N1%d_1_G%d 0  SCHEMATIC1_Fm1%d_G%d \n',ia,gn,ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em2%d_G%d  n_Fm1%d_G%d  n_Em2%d_G%d  N2%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);
                fprintf(fid,'E_Em1%d_G%d  n_Em2%d_G%d  N%d_Zg2_G%d  N1%d_1_G%d  0  1 \n',ia,gn,ia,gn,ia,gn,ia,gn);

        

       
        for ib=1:line_num
            for ia=1:line_num
                if ia==ib
                
                else 
                    fprintf(fid,'R_V%d%d0_G%d    N%d%d_1_G%d  nR_%d%d0_G%d  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Rg0(ia,ib));
                    fprintf(fid,'L_V%d%d0_G%d    nR_%d%d0_G%d  nL_%d%d0_G%d  %d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Lg0(ia,ib));
                    fprintf(fid,'R_V%d%d1_G%d    nL_%d%d0_G%d  nV_%d%d1_G%d  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Rgn(ia,ib,1));
                    fprintf(fid,'L_V%d%d1_G%d    nL_%d%d0_G%d  nV_%d%d1_G%d  %d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Lgn(ia,ib,1));
                    fprintf(fid,'R_V%d%d2_G%d    nV_%d%d1_G%d  nV_%d%d2_G%d  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Rgn(ia,ib,2));
                    fprintf(fid,'L_V%d%d2_G%d    nV_%d%d1_G%d  nV_%d%d2_G%d  %d  \n',ia,ib,gn,ia,ib,gn,ia,ib,gn,sect_length*Lgn(ia,ib,2));
                    fprintf(fid,'R_V%d%d3_G%d    nV_%d%d2_G%d  0  %d  TC=0,0 \n',ia,ib,gn,ia,ib,gn,sect_length*Rgn(ia,ib,3));            
                    fprintf(fid,'L_V%d%d3_G%d    nV_%d%d2_G%d  0  %d  \n',ia,ib,gn,ia,ib,gn,sect_length*Lgn(ia,ib,3));
                    fprintf(fid,' \n');
                    
                end
            end
        end
        fprintf(fid,'.ENDS  Zg_model_G%d \n',gn);
        fprintf(fid,' \n'); 
        fprintf(fid,' \n'); 
end


switch line_num
    case 1
        fprintf(fid,'.SUBCKT main_%s %s %s \n',tl_name,tl_start1,tl_end1);
         for ib=1:sect_num
             fprintf(fid,'R_R11_%dA1_G%d  L1%dA4_G%d  L1%dA1_G%d  0.0001 TC=0,0 \n',ib,gn,ib-1,gn,ib,gn);
             fprintf(fid,'R_R11_%dA2_G%d  L1%dA2_G%d  0  1G TC=0,0 \n',ib,gn,ib,gn);
             fprintf(fid,'R_R11_%dA3_G%d  L1%dA3_G%d  0  1G TC=0,0 \n',ib,gn,ib,gn);
             fprintf(fid,'R_R11_%dA4_G%d  L1%dA1_G%d   L1%dA4_G%d  0.0001 TC=0,0 \n',ib,gn,ib+1,gn,ib,gn);
             fprintf(fid,' \n');
             fprintf(fid,'X_L1%dA_G%d L1%dA1_G%d L1%dA2_G%d L1%dA3_G%d L1%dA4_G%d L1%dA_G%d \n',ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn);
         end
                fprintf(fid,'R_start1_G%d %s L10A4_G%d 0.0001 TC=0,0  \n',gn, tl_start1,gn);
                fprintf(fid,' \n');
                fprintf(fid,'R_end1_G%d %s L1%dA1_G%d 0.0001 TC=0,0  \n',gn, tl_end1,sect_num+1,gn);
                fprintf(fid,' \n');    
         
         
         fprintf(fid,'.ENDS main_%s  \n',tl_name);
         
        
    case 2
        fprintf(fid,'.SUBCKT main_%s %s %s %s %s \n',tl_name,tl_start1,tl_start2,tl_end1,tl_end2);
              
                    for ib=1:sect_num               
                 fprintf(fid,'X_L%dA_G%d L1%dA1_G%d L2%dA1_G%d L1%dA4_G%d L2%dA4_G%d L%dA_G%d \n',ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn);
                 for ia=1:line_num
                 if mod(ib,2)==0;
                     fprintf(fid,'R_%d%dA1_G%d L%d%dA1_G%d N%d%d_Zg2_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib-1,gn);
                     fprintf(fid,'R_%d%dA4_G%d L%d%dA4_G%d L%d%dA1_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib+1,gn);
                     fprintf(fid,' \n');
                 else                     
                      fprintf(fid,'R_%d%dA4_G%d L%d%dA4_G%d N%d%d_Zg1_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
                      fprintf(fid,'R_%d%dA1_G%d L%d%dA1_G%d L%d%dA4_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib-1,gn);
                      fprintf(fid,' \n');
                    end
                 end
                    end
                  fprintf(fid,'R_start1_G%d %s L10A4_G%d 0.0001 TC=0,0  \n',gn, tl_start1,gn);
                fprintf(fid,'R_start2_G%d %s L20A4_G%d 0.0001 TC=0,0  \n',gn, tl_start2,gn);
                 fprintf(fid,' \n');
                fprintf(fid,'R_end1_G%d %s L1%dA1_G%d 0.0001 TC=0,0  \n',gn, tl_end1,sect_num+1,gn);
                fprintf(fid,'R_end2_G%d %s L2%dA1_G%d 0.0001 TC=0,0  \n',gn, tl_end2,sect_num+1,gn);
                fprintf(fid,' \n');              
                  
            for ib=1:sect_num
                if mod(ib,2)==0
                else
                    
                fprintf(fid,'X_Zg_model%d_G%d N1%d_Zg1_G%d N1%d_Zg2_G%d N2%d_Zg1_G%d N2%d_Zg2_G%d Zg_model_G%d \n',ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,gn);
                fprintf(fid,' \n');
                end
            end
            
                        fprintf(fid,'.ENDS main_%s  \n',tl_name);
                        fprintf(fid,' \n');
                        fprintf(fid,' \n');
                        
            for ia=1:line_num
                for ib=1:line_num
                    if ia==ib
                        
                    else
                        
            fprintf(fid,'.subckt SCHEMATIC1_Fm%d%d_G%d 1 2 3 4 \n',ia,ib,gn);
            fprintf(fid,'F_Fm%d%d_G%d         3 4 VF_Fm%d%d_G%d -1 \n',ia,ib,gn,ia,ib,gn);
            fprintf(fid,'VF_Fm%d%d_G%d         1 2 0V \n',ia,ib,gn);
            fprintf(fid,'.ends SCHEMATIC1_Fm%d%d_G%d \n',ia,ib,gn);
            fprintf(fid,' \n');
                    end
                end
            end
            
    case 3
        fprintf(fid,'.SUBCKT main_%s %s %s %s %s %s %s \n',tl_name,tl_start1,tl_start2,tl_start3,tl_end1,tl_end2,tl_end3);
              
                    for ib=1:sect_num               
                 fprintf(fid,'X_L%dA_G%d L1%dA1_G%d L2%dA1_G%d L3%dA1_G%d L1%dA4_G%d L2%dA4_G%d L3%dA4_G%d L%dA_G%d \n',ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn);
                 for ia=1:line_num
                 if mod(ib,2)==0;
                     fprintf(fid,'R_%d%dA1_G%d L%d%dA1_G%d N%d%d_Zg2_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib-1,gn);
                     fprintf(fid,'R_%d%dA4_G%d L%d%dA4_G%d L%d%dA1_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib+1,gn);
                     fprintf(fid,' \n');
                 else                     
                      fprintf(fid,'R_%d%dA4_G%d L%d%dA4_G%d N%d%d_Zg1_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib,gn);
                      fprintf(fid,'R_%d%dA1_G%d L%d%dA1_G%d L%d%dA4_G%d 0.0001 TC=0,0 \n',ia,ib,gn,ia,ib,gn,ia,ib-1,gn);
                      fprintf(fid,' \n');
                    end
                 end
                    end
                  fprintf(fid,'R_start1_G%d %s L10A4_G%d 0.0001 TC=0,0  \n',gn, tl_start1,gn);
                fprintf(fid,'R_start2_G%d %s L20A4_G%d 0.0001 TC=0,0  \n',gn, tl_start2,gn);
                fprintf(fid,'R_start3_G%d %s L30A4_G%d 0.0001 TC=0,0  \n',gn, tl_start3,gn);
                fprintf(fid,' \n');
                fprintf(fid,'R_end1_G%d %s L1%dA1_G%d 0.0001 TC=0,0  \n',gn, tl_end1,sect_num+1,gn);
                fprintf(fid,'R_end2_G%d %s L2%dA1_G%d 0.0001 TC=0,0  \n',gn, tl_end2,sect_num+1,gn);
                fprintf(fid,'R_end3_G%d %s L3%dA1_G%d 0.0001 TC=0,0  \n',gn, tl_end3,sect_num+1,gn);
                fprintf(fid,' \n');              
                  
            for ib=1:sect_num
                if mod(ib,2)==0
                else
                    
                fprintf(fid,'X_Zg_model%d_G%d N1%d_Zg1_G%d N1%d_Zg2_G%d N2%d_Zg1_G%d N2%d_Zg2_G%d N3%d_Zg1_G%d N3%d_Zg2_G%d Zg_model_G%d \n',ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn,ib,gn);
                fprintf(fid,' \n');
                end
            end
            
                        fprintf(fid,'.ENDS main_%s  \n',tl_name);
                        fprintf(fid,' \n');
                        fprintf(fid,' \n');
                        
            for ia=1:line_num
                for ib=1:line_num
                    if ia==ib
                        
                    else
                        
            fprintf(fid,'.subckt SCHEMATIC1_Fm%d%d_G%d 1 2 3 4 \n',ia,ib,gn);
            fprintf(fid,'F_Fm%d%d_G%d         3 4 VF_Fm%d%d_G%d -1 \n',ia,ib,gn,ia,ib,gn);
            fprintf(fid,'VF_Fm%d%d_G%d         1 2 0V \n',ia,ib,gn);
            fprintf(fid,'.ends SCHEMATIC1_Fm%d%d_G%d \n',ia,ib,gn);
            fprintf(fid,' \n');
                    end
                end
            end


            
            
end

                    

fclose(fid);
        
% 
% Uss2=spline(Uss(:,1),Uss(:,2),(1:Nt)*dt);



