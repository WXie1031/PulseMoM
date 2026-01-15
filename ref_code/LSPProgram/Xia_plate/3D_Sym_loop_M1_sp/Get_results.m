% the results matrix of Jx Jy Mx My Mz in the plate
% state: 0 - non symmetrical distribution in the full plate  
%        1 - symmetrical distribution under 'P' source
%        2 - symmetrical distribution under 'V' source
%
function [Jx1 Jx2 Jy1 Jy2 Mx1 Mx2 My1 My2 Mz1 Mz2]=Get_results(JX1,JX2,JY1,JY2,MX1,MX2,MY1,MY2,MZ1,MZ2,N,state)

Nx=N(1);   Ny=N(2);  
XNx=(Nx-1);          % Jx are zeros in the middle cells
YNy=(Ny-1);
Nxc=Nx/2; Nyc=Ny/2;
XNxc=Nxc-1; YNyc=Nyc-1;

% the solutions matrix of Jx Jy Mx My Mz
switch state
    case 0
        for i=1:Ny 
            Jx1(i,:)=JX1((i-1)*XNx+1:i*XNx);           % on the bottom ('1')
            Jx2(i,:)=JX2((i-1)*XNx+1:i*XNx);           % on the top ('2') 
            Mx1(i,:)=MX1((i-1)*Nx+1:i*Nx);
            Mx2(i,:)=MX2((i-1)*Nx+1:i*Nx);
            My1(i,:)=MY1((i-1)*Nx+1:i*Nx);
            My2(i,:)=MY2((i-1)*Nx+1:i*Nx); 
            Mz1(i,:)=MZ1((i-1)*Nx+1:i*Nx);
            Mz2(i,:)=MZ2((i-1)*Nx+1:i*Nx);
        end
        for i=1:YNy
            Jy1(i,:)=JY1((i-1)*Nx+1:i*Nx);
            Jy2(i,:)=JY2((i-1)*Nx+1:i*Nx);
        end            
    case 1
        for i=1:Nyc 
            Jx1(i,:)=JX1((i-1)*XNxc+1:i*XNxc);           % on the bottom ('1')
            Jx2(i,:)=JX2((i-1)*XNxc+1:i*XNxc);           % on the top ('2')    
            Mx1(i,:)=MX1((i-1)*Nxc+1:i*Nxc);
            Mx2(i,:)=MX2((i-1)*Nxc+1:i*Nxc);
            My1(i,:)=MY1((i-1)*Nxc+1:i*Nxc);
            My2(i,:)=MY2((i-1)*Nxc+1:i*Nxc); 
            Mz1(i,:)=MZ1((i-1)*Nxc+1:i*Nxc);
            Mz2(i,:)=MZ2((i-1)*Nxc+1:i*Nxc);
        end
        for i=1:YNyc
            Jy1(i,:)=JY1((i-1)*Nxc+1:i*Nxc); 
            Jy2(i,:)=JY2((i-1)*Nxc+1:i*Nxc);
        end
        Jx1=[Jx1          JX1(end-Nyc+1:end)          fliplr(Jx1);
            -flipud(Jx1) -flipud(JX1(end-Nyc+1:end)) -rot90(Jx1,2)];
        Jx2=[Jx2          JX2(end-Nyc+1:end)          fliplr(Jx2);
            -flipud(Jx2) -flipud(JX2(end-Nyc+1:end)) -rot90(Jx2,2)];
        Jy1=[Jy1                 -fliplr(Jy1); 
             JY1(end-Nxc+1:end)' -fliplr(JY1(end-Nxc+1:end)');
             flipud(Jy1)         -rot90(Jy1,2)];
        Jy2=[Jy2                 -fliplr(Jy2); 
             JY2(end-Nxc+1:end)' -fliplr(JY2(end-Nxc+1:end)');
             flipud(Jy2)         -rot90(Jy2,2)];
        Mx1=[Mx1         -fliplr(Mx1);
             flipud(Mx1) -rot90(Mx1,2)];
        Mx2=[Mx2         -fliplr(Mx2);
             flipud(Mx2) -rot90(Mx2,2)];
        My1=[My1          fliplr(My1);
            -flipud(My1) -rot90(My1,2)];
        My2=[My2          fliplr(My2);
            -flipud(My2) -rot90(My2,2)];
        Mz1=[Mz1          fliplr(Mz1);
             flipud(Mz1)  rot90(Mz1,2)];
        Mz2=[Mz2          fliplr(Mz2);
             flipud(Mz2)  rot90(Mz2,2)];         
    case 2
        for i=1:Nyc 
            Jx1(i,:)=JX1((i-1)*XNxc+1:i*XNxc);           % on the bottom ('1')
            Jx2(i,:)=JX2((i-1)*XNxc+1:i*XNxc);           % on the top ('2')    
            Mx1(i,:)=MX1((i-1)*Nxc+1:i*Nxc);
            Mx2(i,:)=MX2((i-1)*Nxc+1:i*Nxc);
            My1(i,:)=MY1((i-1)*Nxc+1:i*Nxc);
            My2(i,:)=MY2((i-1)*Nxc+1:i*Nxc); 
            Mz1(i,:)=MZ1((i-1)*Nxc+1:i*Nxc);
            Mz2(i,:)=MZ2((i-1)*Nxc+1:i*Nxc);
        end
        for i=1:YNyc
            Jy1(i,:)=JY1((i-1)*Nxc+1:i*Nxc); 
            Jy2(i,:)=JY2((i-1)*Nxc+1:i*Nxc);
        end
        Jx1=[Jx1         zeros(Nyc,1) -fliplr(Jx1);
            -flipud(Jx1) zeros(Nyc,1)  rot90(Jx1,2)];
        Jx2=[Jx2         zeros(Nyc,1) -fliplr(Jx2);
            -flipud(Jx2) zeros(Nyc,1)  rot90(Jx2,2)];
        Jy1=[Jy1                 fliplr(Jy1); 
             JY1(end-Nxc+1:end)' fliplr(JY1(end-Nxc+1:end)');
             flipud(Jy1)         rot90(Jy1,2)];
        Jy2=[Jy2                 fliplr(Jy2); 
             JY2(end-Nxc+1:end)' fliplr(JY2(end-Nxc+1:end)');
             flipud(Jy2)         rot90(Jy2,2)];
        Mx1=[Mx1         fliplr(Mx1);
             flipud(Mx1) rot90(Mx1,2)];
        Mx2=[Mx2         fliplr(Mx2);
             flipud(Mx2) rot90(Mx2,2)];
        My1=[My1        -fliplr(My1);
            -flipud(My1) rot90(My1,2)];
        My2=[My2        -fliplr(My2);
            -flipud(My2) rot90(My2,2)];
        Mz1=[Mz1        -fliplr(Mz1);
            flipud(Mz1) -rot90(Mz1,2)];
        Mz2=[Mz2        -fliplr(Mz2);
            flipud(Mz2) -rot90(Mz2,2)];        
end

end
