function [h_field, h_ang] = h_rec_loop(a, b, h)


h_field = 2*a.*b.*(a.^2+b.^2+8*h.^2)./( pi*(a.^2+4*h.^2).*(b.^2+4*h.^2).*sqrt(a.^2+b.^2+4*h.^2) );

h_ang = 0;


end
