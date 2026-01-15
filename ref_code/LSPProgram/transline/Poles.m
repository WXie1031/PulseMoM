%
% PURPOSE : Calculate the rational approximation f(s)= C*(s*I-A)^(-1)*B + D + s*E
%           where A is diagonal and B is a column of ones (weight function).
%
% The function is programed as follows
%
%   [A]=Poles(Fs,s,Pi,Ns)
%
% INPUTS
%   Fs(s) : function (column vector) to be fitted. 
%   s     : column vector of frequency points [rad/sec] 
%   Pi    : vector of starting poles [rad/sec] 
%   Ns    : number of samples
%   Ka   : 1 --> order(numerator)=order(denominator)-1 ('Strictly proper')
%          2 --> order(numerator)=order(denominator)   ('Proper')
%          3 --> order(numerator)=order(denominator)+1 ('Improper')
%
% OUTPUTS
%   A : poles
%
%
function [A]=Poles(Fs,s,Pi,Ns,Ka);

%   For vectors, SORT(X) sorts the elements of X in ascending order.
%   For matrices, SORT(X) sorts each column of X in ascending order.
%   For N-D arrays, SORT(X) sorts the along the first non-singleton
% dimension of X.
%   When X is complex, the elements are sorted by ABS(X).  Complex
% matches are further sorted by ANGLE(X).

% Sort poles in ascending order. First real poles and then complex poles
Np     = length(Pi);   % Length of the vector that contains the starting poles
CPX    = imag(Pi)~=0;  % Put 0 for a real pole and 1 for a complex pole
rp     = 0;            % Initialize the index for real poles
cp     = 0;            % Initialize the index for complex poles
RePole = [];           % Initialize the vector of real poles
CxPole = [];           % Initializa the vector of complex poles

% Loop to separate real poles and complex poles
for k = 1:Np
    if CPX(k) == 0     % Real Pole
        rp = rp + 1;
        RePole(rp) = Pi(k);
    elseif CPX(k) == 1 % Complex pole
        cp = cp + 1;
        CxPole(cp) = Pi(k);
    end
end

Lambda = Pi.';
RePole = sort(RePole);       % Sort real poles
CxPole = sort(CxPole);       % Sort complex poles
Lambda = [RePole CxPole];    % Concentrate the full set of starting poles
I      = diag(ones(1,Np));   % Unit diagonal matrix of ones
A      = [];                 % Poles
B      = ones(Ns,1);         % the weight factor (always one for each pole) 
C      = [];                 % Residues
D      = zeros(1);           % Initialize the variable to store the constant term (is produced if asympflag=2 or 3)
E      = zeros(1);           % Initialize the variable to store proportional term (is produced if asympflag=3)
KQA    = ones(Ns,1);

% Identifies which poles are complex and creates a vector with
%  0 - for a real pole
%  1 - for the real part of a complex conjugate poles
%  2 - for the imaginary part of a complex conjugate poles

cpx = imag(Lambda)~=0;  % This instruction put 0 for real pole and 1 for complex pole
dix = zeros(1,Np);      % Initialize the vector to identifies poles
if cpx(1)~=0            % If the first pole is complex
    dix(1)=1;           % put 1 in dix(1) for the real part
    dix(2)=2;           % put 2 in dix(2) for the imag part
    k=3;                % continue dix for the third position
else
    k=2;                % If the first pole is real continue dix for the second position
end

% complete the clasification of the poles
for m=k:Np 
   if cpx(m)~=0         % If the pole is complex
       if dix(m-1)==1
           dix(m)=2;    % If the previous position has the real part put 2 to identifies the imag part
       else
           dix(m)=1;    % put 1 for the real part of a complex pole
       end
   end
end

% Creates matriz A which id divided in four parts A = [A1 A2 A3 A4]
% A1 = Dk
% A2 = B.*ones(Ns,1)
% A3 = B.*s
% A4 = -Dk*Fs

%   This routine build Dk matrix equal to 1./(s-Pi), where Pi are the start poles,
% this matriz contains at the beginning the real poles and then the complex ones
Dk = zeros(Ns,Np);               % Initialize the matrix in zeros
for m=1:Np                       % Iterative cicle for all poles
   if dix(m)== 0                 % For a real pole
      Dk(:,m) = B./(s-Lambda(m));  
   elseif dix(m)== 1             % For the real part of a complex pole
      Dk(:,m)     = B./(s-Lambda(m)) + B./(s-Lambda(m)');
   elseif dix(m)== 2             % For the imag part of a complex pole
      Dk(:,m) = i.*B./(s-Lambda(m-1)) - i.*B./(s-Lambda(m-1)');
   end
end

% Creates the space to work for matrix A
A1 = Dk;
A2 = B.*ones(Ns,1); 
A3 = B.*s;  
for col = 1:Np
   A4(:,col) = -(Dk(:,col).*Fs.');
end 
      
% Asign the values of A
if Ka == 1
    A = [A1 A4];           % Strictly proper rational fitting
elseif Ka == 2
    A = [A1 A2 A4];        % Proper rational fitting
elseif Ka == 3
    A = [A1 A2 A3 A4];     % Improper rational fitting
else
    disp('Ka need to be 1, 2 or 3')
end

% Creates matrix b = B*Fs
b = B.*Fs.';

% Separete real and imaginary part
Are = real(A);    % Real part of matrix A
Aim = imag(A);    % Imaginary part of matrix A
bre = real(b);    % Real part of matrix b
bim = imag(b);    % Imaginary part of matrix b

An = [Are; Aim];   % Real and imaginary part of A
bn = [bre; bim];   % Real and imaginary part of b

% Rutine to applies the euclidian norm to the matrix An
[Xmax Ymax] = size(An);
for col=1:Ymax
  Euclidian(col)=norm(An(:,col),2);     % Euclidian norm : NORM(V,P) = sum(abs(V).^P)^(1/P).
  An(:,col)=An(:,col)./Euclidian(col);  % Applies the Euclidian norm to An
end
 
% Solving system  X=inv(A'*A)*A'*b  over-determined system (Ax=b ===> x=A\b)                   
Xn = An\bn;
Xn = Xn./Euclidian.';

% Put the residues into matrix C
if Ka == 1
    C = Xn(Np+1:Ymax);      % Strictly proper rational fitting
elseif Ka == 2
    C = Xn(Np+2:Ymax);     % Proper rational fitting
elseif Ka == 3
    C = Xn(Np+3:Ymax);     % Improper rational fitting
else
    disp('Ka need to be 1, 2 or 3')
end

% Make C complex when the residues are complex
for m=1:Np
   if dix(m)==1
         alfa   = C(m);            % real part of a complex pole
         betta  = C(m+1);          % imag part of a complex pole
         C(m)   = alfa + i*betta;  % the complex pole
         C(m+1) = alfa - i*betta;  % the conjugate of the previous complex pole
   end
end


%%%%%% This partr of the theory need to be well documented  %%%%%%
% Now calculate the zeros for sigma
BDA = zeros(Np);
KQA = ones(Np,1);

% Loop to calculate the zeros of sigma which are the new poles
for km = 1:Np
   if dix(km)== 0          % For a real pole
       BDA(km,km) = Lambda(km);
   elseif dix(km)== 1      % For a complex pole with negative imag part
       BDA(km,km)   = real(Lambda(km));
       BDA(km,km+1) = imag(Lambda(km));
       KQA(km)      = 2;
       Aux = C(km);
       C(km)         = real(Aux);
   elseif dix(km)== 2      % For a complex pole with positive imag part
       BDA(km,km)   = real(Lambda(km));
       BDA(km,km-1) = imag(Lambda(km));
       KQA(km)      = 0;
       C(km)        = imag(Aux);
   end
end


ZEROS = BDA - KQA*C.';
POLS  = eig(ZEROS).';

%Forcing unstable poles to be stable
uns       = real(POLS)>0;  
POLS(uns) = POLS(uns)-2*real(POLS(uns));

% Sort poles in ascending order. First real poles and then complex poles
CPX    = imag(POLS)~=0;  % Put 0 for a real pole and 1 for a complex pole
rp     = 0;   % Initializa the index for real poles
cp     = 0;   % Initialize the index for complex poles
RePole = [];  % Initialize the vector of real poles
CxPole = [];  % Initialize the vector of complex poles
% Loop to separate real poles and complex poles
for k = 1:Np
    if CPX(k) == 0     % Real Pole
        rp = rp + 1;
        RePole(rp) = POLS(k);
    elseif CPX(k) == 1 % Complex pole
        cp = cp + 1;
        CxPole(cp) = POLS(k);
    end
end

RePole = sort(RePole);     % Sort real poles
CxPole = sort(CxPole);     % Sort complex poles
% ==> For a pair of complex poles, first put the positive imag part
CxPole = (CxPole.')';      % Is the same: CxPole = CxPole-2*i*imag(CxPole)
NewPol = [RePole CxPole];  % Concentrate the full set of relocated poles

A = NewPol.';   % Output
