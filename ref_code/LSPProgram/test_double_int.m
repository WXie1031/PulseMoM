clc;clear;
%   compute int_a^b [int_c)^d    f(x,y)]
%   (x,y) \in [a,b] X [c,d]
%%  setup the integral interval and gauss point and weight
a = 1.;    b = 2;
c = 1;      d =5;
fun = @(x,y)    x.*y./(x+y+3).^1.5.*log(x+y.^0.5);
fprintf('***********************************************\n')
for gauss = 2:3 %    m points rule in 2 dimensional case
    if gauss == 2
        fprintf('*******  2X2 points gauss rule result *******')
        gpt=1/sqrt(3);
        s(1) = -gpt;  t(1) = -gpt;
        s(2) =  gpt;  t(2) = -gpt;
        s(3) =  gpt;  t(3) =  gpt;
        s(4) = -gpt;  t(4) =  gpt;
        wt = [1 1 1 1];        
    elseif gauss == 3
        gpt=sqrt(0.6);
        fprintf('*******   3X3 points gauss rule     *******')
        s(1) = -gpt; t(1) = -gpt; wt(1)=25/81;
        s(2) =  gpt; t(2) = -gpt; wt(2)=25/81;
        s(3) =  gpt; t(3) =  gpt; wt(3)=25/81;
        s(4) = -gpt; t(4) =  gpt; wt(4)=25/81;
        s(5) =  0.0; t(5) = -gpt; wt(5)=40/81;
        s(6) =  gpt; t(6) =  0.0; wt(6)=40/81;
        s(7) =  0.0; t(7) =  gpt; wt(7)=40/81;
        s(8) = -gpt; t(8) =  0.0; wt(8)=40/81;
        s(9) =  0.0; t(9) =  0.0; wt(9)=64/81;        
    end
    %%   区间变换到   [-1,1] X [-1,1]
    jac = (b-a)*(d-c)/4;
    x = (b+a+(b-a)*s)/2;
    y = (d+c+(d-c)*t)/2;
    f = fun(x,y);
    comp = wt(:) .* f(:) .* jac;%无论一个向量是行还是列，写成x(:)都会变成列向量
    
    format long
    comp = sum(comp)
    exact = integral2(fun,a,b,c,d);
    
    fprintf('the error is norm(comp-exact)=%10.6e\n\n',norm(comp-exact))
    
end
fprintf('******************************************\n')
fprintf('matlab  built-in function ''integral2''\n')
exact
format short


%%
Nint1 = 5;
Nint2 = 6;
[TT1, AA1] = gauss_int_coef(Nint1);
[TT2, AA2] = gauss_int_coef(Nint2);
a1 = a;    a2 = b;
r1 = c;      r2 =d;
x = (a2+a1)/2*ones(1,Nint1) + (a2-a1)/2*TT1;
y = (r2+r1)/2*ones(1,Nint2) + (r2-r1)/2*TT2;

finDxz = zeros(1,Nint2);
for k = 1:Nint2
    y1 =  y(:,k)*ones(1,Nint1);
    Sx =  fun(x,y1);
    
    finDxz(:,k)= (a2-a1)/2.* sum(AA1.*(Sx),2);
end

Dxz = (r2-r1)/2.*sum(AA2.*finDxz,2)

