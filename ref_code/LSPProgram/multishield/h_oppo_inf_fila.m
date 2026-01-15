function [h_field, h_ang] = h_oppo_inf_fila(d, h)


h_field = 1/(2*pi) * ( d./((d/2).^2+h.^2) );

h_ang = 0;


end
