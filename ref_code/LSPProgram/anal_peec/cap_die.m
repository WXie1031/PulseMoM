function Ce = cap_die(A, len, epr)
%  Function:       cap_die
%  Description:    Calculate capacitance of a delectric cell.
%
%  Calls:          
%
%  Input:          A     --  the area of the cross section
%                  l     --  length of cell 
%  Output:         Ce    --  excess capacitance related to dielectric
%  Others:
%  Author:         Chen Hongcai
%  Email :         hc.chen@live.com
%  Date:           2015-12-16


ep0 = 8.85*1e-12;

Ce = ep0.*(epr-1).*A./len;


end

