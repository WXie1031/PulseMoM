function [ki1 ki2]=KGEN_A(aa,dw,zi)
dd=dw;
yi=zi;
tp0=aa*dd;

if abs(tp0)>14
    tp1=0.5*tp0*(1-yi/dd*2);
    tp2=0.5*tp0*(1+yi/dd*2);    
    ki2=exp(tp1)-exp(tp2+tp0);         
    ki1=exp(tp2)-exp(tp1+tp0);             
else
    ki2=sinh(0.5*aa*dd+aa*yi)/sinh(aa*dd);
    ki1=sinh(0.5*aa*dd-aa*yi)/sinh(aa*dd); 
end
end