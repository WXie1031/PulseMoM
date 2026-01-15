frq = [1e3 2e3 5e3 1e4 2e4 5e4 1e5 2e5 5e5 1e6];

mur = 40;

sig = 1e7;
epr = 1;

rout = 1e-3/2;
rin = 0;
len = 1;

d=rout;

[R, Lin, Zin] = zin_cir_ac(rout, rin, sig, mur, len, frq);

Lext = 2*1e-7*len*(log(len/d+sqrt((len/d)^2+1))-sqrt(1+(d/len)^2)+d/len);
L = Lin+Lext;


Rpul = 1/(sig*pi*rout^2);
pt_2d = [0,0];
[~,~,~,~,~,RmeshMF,LmeshMF] = main_mesh2d_cmplt(1, pt_2d, rout,rin, ...
    Rpul, sig,mur,epr, len, frq);
LmeshMF = LmeshMF(:);
RmeshMF = RmeshMF(:);


figure(5)
hold on
plot(frq,R,'k-');
plot(frq,RmeshMF(:),'r--')
legend('Skin','PEEC')


figure(6)
hold on
plot(frq,L,'k-');
plot(frq,LmeshMF(:),'r--')
legend('Skin','PEEC')


