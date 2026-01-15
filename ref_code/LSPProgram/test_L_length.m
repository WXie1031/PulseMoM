





ik=6;
LL=zeros(1,20);
for ih=1:20;
    
    [~, LL(ih)] = para_main_self_multi_frq(shape(ik), dim1(ik), dim2(ik), ...
         ih, R_pul(ik), sig(ik),20, frq(1));
    
    LL(ih) = LL(ih)/ih;
end;
figure;
plot(LL/LL(1));


cof=zeros(1,20);
for ih=1:20;
    %cof(ih) = ih.*(1 + log(ih)./(1.2-log(dim1(ik)+dim2(ik))));
    
    cof(ih) = ih.*(1 + log(ih)./(-0.3-log(dim1(ik))));
     cof(ih) = cof(ih)/ih;
end;
figure;
plot(cof);


