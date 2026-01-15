function [h_field, h_ang] = h_square_loop(a, h)


h_field = 4./( (pi*a) .* ( 1+4*h.^2./a.^2).*sqrt(2+4*h.^2/a.^2) );

h_ang = 0;


end
