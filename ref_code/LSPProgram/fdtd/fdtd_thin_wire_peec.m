function m = fdtd_thin_wire_peec(len, ds, dim1, dim2)

if nargin == 3
    Lold = induct_cir_ext(dim1, len);
elseif nargin ==4
    Lold = induct_bar_ext(dim1,dim2, len);
end

%Lrec = induct_cir_ext(r./0.23, len);
%Lrec = induct_bar_ext(ds*2,ds*2, len);

Lrec_dc = induct_bar_Grover(ds*2, ds*2, len, 1);
% in paper "Internal Impedance of Conductors of Rectangular Cross Section"
Lrec_in = 48e-9*len;
% in paper "DC Internal Inductance for a Conductor of Rectangular Cross Section"
%Lrec_in = 48.3e-9*len;
Lrec = Lrec_dc-Lrec_in;

m = Lrec./Lold;

end

