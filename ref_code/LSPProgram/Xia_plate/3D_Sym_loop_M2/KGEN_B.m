function [ko1 ko2]=KGEN_B(aa,dw,kkv)
dd=dw;
tp0=aa*dd;

if abs(tp0)>14
   tp1=0.5*tp0*(1-kkv);
   tp2=0.5*tp0*(1+kkv);    
   ko1=exp(tp1)-exp(tp2+tp0);         
   ko2=exp(tp2)-exp(tp1+tp0);             
else
   ko1=sinh(0.5*aa*dd*(1+kkv))/sinh(aa*dd);         
   ko2=sinh(0.5*aa*dd*(1-kkv))/sinh(aa*dd);
end
end
