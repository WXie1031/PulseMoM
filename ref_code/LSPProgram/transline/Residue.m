%
% PURPOSE : Calculate the rational approximation f(s)= C*(s*I-A)^(-1)*B + D + s*E
%           where A is diagonal and B is a column of ones (weight function).
%
% The function is programed as follows
%
%   [C,D]=Residue(Fs,s,Pi,Ns)
%
% INPUTS
%   Fs   : function (column vector) to be fitted. 
%   s    : column vector of frequency points [rad/sec] 
%   Pi   : vector of starting poles
%   Ns   : number of samples
%   Ka   : 1 --> order(numerator)=order(denominator)-1 ('Strictly proper')
%          2 --> order(numerator)=order(denominator)   ('Proper')
%          3 --> order(numerator)=order(denominator)+1 ('Improper')
%
% OUTPUTS
%   C : residues
%   D : constant term
%   E : proportional term
%
function [C,D,E]=Residue(Fs,s,Pi,Ns,Ka);

%   For vectors, SORT(X) sorts the elements of X in ascending order.
%   For matrices, SORT(X) sorts each column of X in ascending order.
%   For N-D arrays, SORT(X) sorts the along the first non-singleton
% dimension of X.
%   When X is complex, the elements are sorted by ABS(X).  Complex
% matches are further sorted by ANGLE(X).

% Sort poles in ascending order. First real poles and then complex poles
Np     = length(Pi);   % Length of the vector that contains the poles
CPX    = imag(Pi)~=0;  % Put 0 for a real pole and 1 for a complex pole
rp     = 0;            % Initializa the index for real poles
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

RePole = sort(RePole);       % Sort real poles
CxPole = sort(CxPole);       % Sort complex poles
CxPole = (CxPole.')';        % CxPole=CxPole-2*i*imag(CxPole);
Lambda = [RePole CxPole];    % Concentrate the full set of starting poles
I      = diag(ones(1,Np));   % Unit diagonal matrix of ones
A      = [];                 % Poles
B      = ones(Ns,1);         % the weight factor (always one for each pole) 
C      = [];                 % Residues
D      = zeros(1);           % Initialize the variable to store the constant term (is produced if asympflag=2 or 3)
E      = zeros(1);           % Initialize the variable to store proportional term (is produced if asympflag=3)

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

% Compute the outputs matrices:
%   A = Poles (Lambda)
%   C = Residues
%   D = Constant term
%   E = Proportional term

%   This routine build Dk matrix equal to 1./(s-Lambda) where Lambda's are the above
% calculated poles, this matriz contains at the biginning the real poles
% and then the complex ones
Dk=zeros(Ns,Np);                  
for m=1:Np
   if dix(m)==0        % Real pole
      Dk(:,m) = B./(s-Lambda(m));
   elseif dix(m)==1    % Complex pole, 1st part
      Dk(:,m) = B./(s-Lambda(m)) + B./(s-Lambda(m)');
   elseif dix(m)==2    % Complex pole, 2st part
      Dk(:,m) = i.*B./(s-Lambda(m-1)) - i.*B./(s-Lambda(m-1)');
   end
end 

% Creates the space to work for matrix A and matrix b
AA1=Dk;
AA2=B.*ones(Ns,1); 
AA3=B.*s;  

if Ka == 1
    AA = [AA1];          % Strictly proper rational fitting
elseif Ka == 2
    AA = [AA1 AA2];      % Proper rational fitting
elseif Ka == 3
    AA = [AA1 AA2 AA3];  % Improper rational fitting
else
    disp('Ka need to be 1, 2 or 3')
end

bb  = B.*Fs.';

AAre = real(AA);      % Real part of matrix A
AAim = imag(AA);      % Imaginary part of matrix A
bbre = real(bb);      % Real part of matrix b
bbim = imag(bb);      % Imaginary part of matrix b

AAn = [AAre; AAim];   % Real and imaginary part of A
bbn = [bbre; bbim];   % Real and imaginary part of b

[Xmax Ymax] = size(AAn);
for col=1:Ymax
  Eeuclidian(col)=norm(AAn(:,col),2);  % Euclidian norm : NORM(V,P) = sum(abs(V).^P)^(1/P).
  AAn(:,col)=AAn(:,col)./Eeuclidian(col);
end
 
% Solving system  X=inv(A'*A)*A'*b  over-determined system (Ax=b   ===>    x=A\b)                   
Xxn=AAn\bbn;
X=Xxn./Eeuclidian.';

% Put the residues into matrix C
C=X(1:Np);

% Make C complex when the residues are complex
for m=1:Np
   if dix(m)==1
         alfa   = C(m);            % real part of a complex pole
         betta  = C(m+1);          % imag part of a complex pole
         C(m)   = alfa + i*betta;  % the complex pole
         C(m+1) = alfa - i*betta;  % the conjugate of the previous complex pole
   end
end

% Outputs
if Ka == 1
    A  = Lambda.';   % Poles
    C  = C;          % Residues
    D  = 0;          % Constant term
    E  = 0;          % Proportioal term
elseif Ka == 2
    A  = Lambda.';   % Poles
    C  = C;          % Residues
    D  = X(Np+1);    % Constant term
    E  = 0;          % Proportioal term
elseif Ka == 3
    A  = Lambda.';   % Poles
    C  = C;          % Residues
    D  = X(Np+1);    % Constant term
    E  = X(Np+2);    % Proportioal term
end

