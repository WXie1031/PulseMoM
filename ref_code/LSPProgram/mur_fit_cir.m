function mur = mur_fit_cir(r_out, r_in, R_pul, sig, Rsurge, fmur)
%  Function:       mur_fit_cir
%  Description:    Calculate AC resistance of the circular conductor using
%                  fitting method using Arnold's formulas (solid) and 
%                  Dwight (hollow). Reference: "The current distribution,
%                  resistance and internal inductance of linear power
%                  system conductors - a review of explicit equations"
%
%  Calls:
%
%  Input:          r_out  --  outer radius of conductors (N*1) (m)
%                  r_in   --  inner radius of conductors (N*1) (m)
%                  Rdc    --  DC resistancee of conductors
%                  sig    --  conductivity of conductors (N*1) (S/m)
%                  len    --  length of conductors (N*1)
%                  fmur   --  fitted frequency (1*1)
%  Output:         mur    --  obtained relative permeability
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2018-01-22
%  History:

mu0 = 4*pi*1e-7;

r_in = abs(r_in);
r_out = abs(r_out);

Nc = size(r_out,1);
mur = ones(Nc,1);
ksk = Rsurge./R_pul;

for ik = 1:Nc
    mcof = sqrt(1./(2*pi*fmur*mu0*sig(ik)))./r_out(ik);
    
    if r_in(ik) == 0
        m_rs1 = (192./(1./(ksk(ik)-1)-0.8)).^(-1/4);
        m_rs2_1 = (0.0177+sqrt(0.0177^2-4*0.0563*(0.864-ksk(ik))))./(2*0.864);
        m_rs2_2 = (0.0177-sqrt(0.0177^2-4*0.0563*(0.864-ksk(ik))))./(2*0.864);
        m_rs3 = 2*sqrt(2)*(ksk(ik)-4/15);
        
        if isreal(m_rs1) && (m_rs1>0) && (m_rs1<=2.8)
            mur(ik) = m_rs1*mcof;
        elseif isreal(m_rs2_1) && (m_rs2_1>2.8) && (m_rs2_1<=3.8)
            mur(ik) = m_rs2_1*mcof;
        elseif isreal(m_rs2_2) && (m_rs2_2>2.8) && (m_rs2_2<=3.8)
            mur(ik) = m_rs2_2*mcof;
        elseif m_rs3>3.8
            mur(ik) = m_rs3*mcof;
        end
        
        mur(ik) = mur(ik)*mur(ik);
    else
        
        m_rs = fminsearch(@(m_rs) abs(m_rs*(r_out(ik)^2-r_in(ik)^2)/(2*sqrt(2)*r_out(ik)^2) ...
            * ( 1 + 1/sqrt(2)/m_rs + 3/8/m_rs^2) - ksk(ik)), 20 );
        
        mur(ik) = m_rs*mcof;
        mur(ik) = mur(ik)*mur(ik);
    end
    
    
    mur(ik) = max(mur(ik),1);
    
end


end



