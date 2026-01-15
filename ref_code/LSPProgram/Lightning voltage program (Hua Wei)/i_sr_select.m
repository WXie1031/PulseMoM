function [i_sr] = i_sr_select(Imax,t1,t2,k0,n,alpha1,alpha2,i_sr_type,t_ob,dt)

Nt=round(t_ob./dt);


if i_sr_type == 1  %% Heidler model
    for i=1:Nt
        I_temp(i)=(i*dt/t1)^n/(1+(i*dt/t1)^n);
        i_sr(i)=Imax/k0*(I_temp(i))*exp(-i*dt/t2);
    end
elseif i_sr_type ==2 %%  Double Exp model
    for i=1:Nt
        i_sr(i)=Imax*(exp(-alpha1*i*dt)-exp(-alpha2*i*dt)); 
    end  
else
   i_sr=0; 
end
end