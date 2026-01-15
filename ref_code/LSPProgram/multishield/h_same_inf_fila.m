function [h_field, h_ang] = h_same_inf_line(d, h)


h_field = h/pi * ( 1/((d/2).^2+h.^2) );

h_ang = pi/2;


end
