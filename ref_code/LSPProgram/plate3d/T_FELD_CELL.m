% T_FIED_CELL calcualte magnetic field from cell current

function [Bx By Bz]=T_FELD_CELL(Plat,Sour,Fied)
Ic=Plat.cur;
Is=Sour.cur;

