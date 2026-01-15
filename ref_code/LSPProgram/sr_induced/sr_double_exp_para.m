%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% SR_DExp_sub sub program of SR_DExp. Calculation Alpha and Beta and other
%               parameters according rise and fall time.
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% INPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% tr            rise time of waveform (us)
% tf            fall time of waveform (us)
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% OUTPUT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% valores       parameters for double exponential function setting
% %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% revised by Chen Hongcai 2014.8
function [amp, alpfa, beta] = sr_double_exp_para(amp, tr, tf)

% Unit(us)

t03=tr*0.3;
t09=tr*0.9;

% PUNTOS INICIALES E ITERACION
if (tr==8) && (tf==20)
    kini=2.11653466667;
    alfaini=0.05776227;
    betaini=0.2310491;
    valores=[kini alfaini betaini t03 t09];
else
    if (tf>0) && (tf<10)
        kini=1.5;
        alfaini=0.15;
        betaini=2;
        valores = fminsearch(@(x) sr_double_exp_para_sub(x,tr,tf),[kini alfaini betaini t03 t09]);
    end
    if (tf>=10) && (tf<30)
        kini=2;
        alfaini=0.05;
        betaini=0.2;
        %valores=fminsearch('RAYO',[kini alfaini betaini t03 t09],t1,t2);
        valores = fminsearch(@(x) sr_double_exp_para_sub(x,tr,tf),[kini alfaini betaini t03 t09]);
    end
    if (tf<=50) && (tf>=30)
        kini=3;
        alfaini=0.01;
        betaini=0.1;
        valores = fminsearch(@(x) sr_double_exp_para_sub(x,tr,tf),[kini alfaini betaini t03 t09]);
    end
    if (tf>50) && (tf<2000)
        kini=1.1;
        alfaini=0.003;
        betaini=0.1;
        valores = fminsearch(@(x) sr_double_exp_para_sub(x,tr,tf),[kini alfaini betaini t03 t09]);
    end
    if  tf>=2000
        kini=1.2;
        alfaini=0.00043;
        betaini=0.009;
        valores = fminsearch(@(x) sr_double_exp_para_sub(x,tr,tf),[kini alfaini betaini t03 t09]);
    end  
end


amp = amp * valores(1);
alpfa = valores(2);
beta = valores(3); 

end


