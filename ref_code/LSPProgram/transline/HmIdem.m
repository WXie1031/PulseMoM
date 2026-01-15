%
% Idempotent matrices of Y*Z
%
% Input
%
%    Z      ==> Total impedance
%    Y      ==> Total admittance
%    L      ==> Line lenght
%    f      ==> Vector of frequencies
%    Delays ==> Modal delays
%    Hm     ==> Modal Hm
%
% Outputs
%
%    Idem ==> Idempotent asociated to lambda 1,2 and 3
%    Idem = [Id1 Id2 Id3]
%
% Call the function
%
%    [Idem] = HmIdem(Z,Y,L,f,Delays,Hm)
%
%
function [Idem] = HmIdem(Z,Y,L,f,Delays,Hm)

s  = i*2*pi*f;       % Vector of the variable "s"
t1 = Delays(1);   % Time delay of mode 1
t2 = Delays(2);   % Time delay of mode 2
t3 = Delays(3);   % Time delay of mode 3

[M,Lambda] = eig(Y*Z);   % Eigenvectors and eigenvalues of Y*Z
Mi                 = inv(M);      % Inverse of the eigenvectors

C1 = M(:,1);   R1 = Mi(1,:);  % First column and row of the matrices M and Mi respectively
C2 = M(:,2);   R2 = Mi(2,:);  % Second column and row of the matrices M and Mi respectively
C3 = M(:,3);   R3 = Mi(3,:);  % Third column and row of the matrices M and Mi respectively

Id1 = (C1*R1)*Hm(1)*exp(s*t1);  % First Idempotent
Id2 = (C2*R2)*Hm(2)*exp(s*t2);  % Second Idempotent
Id3 = (C3*R3)*Hm(3)*exp(s*t3);  % Third Idempotent

Idem(:,:,1) = Id1;
Idem(:,:,2) = Id2;
Idem(:,:,3) = Id3;



