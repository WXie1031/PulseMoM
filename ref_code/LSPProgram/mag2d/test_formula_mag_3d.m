

syms x y z x0 y0 
% -y/R^3
syms x y z x0 y0 z0 l1 l2
% f1 = simplify( int(-y/((x-x0)^2 + (y-y0)^2 + (z-z0)^2)^1.5, z) )

f1 = simplify( int(-y/(x^2+y^2 + (l1-l2)^2)^1.5, l1) )
f2 = simplify( int(f1, l2) )

(y*(x^2 + y^2 + (l1 - l2)^2)^(1/2))/(x^2 + y^2)
 
% l1 -> l 0     l2 -> l 0 
% four combinations f(l - l) - f(l - 0) - f(0 - l) + f(0 - 0)


% 
% f2 = simplify(int(-y*(l-z0)/(x^2+y^2)/sqrt(x^2+y^2+(l-z0)^2),z0))
% f3 = simplify(int(-y*z0/(x^2+y^2)/sqrt(x^2+y^2+z0^2),z0))
% 



yms l2 l1 x y l 
% 3x^2/R^5
f1 = simplify(int(3*x^2/(x^2+y^2+l^2-2*l*l1+l1^2)^2.5,l))
f1 = simplify(int((x^2*(l1-l)*(2*l1^2-4*l1*l+2*l^2+3*x^2+3*y^2))/((x^2+y^2)^2*(l1^2-2*l1*l+l^2+x^2+y^2)^1.5),l))
f1 = simplify(int(l*x^2*(2*l^2+3*x^2+3*y^2)/((x^2+y^2)^2*(l^2+x^2+y^2)^1.5),l))

