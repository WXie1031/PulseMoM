Nsample = 2500;

Exp3_I=[0	0
    0.01    0
0.1	6
0.2	13.1
0.3	19.6
0.4	25
0.5	28.5
0.6	31
0.7	32.5
0.8	33
0.85 33.2
0.9	33.1
1	32.3
1.1	31.5
1.2	30.8
1.3	29.7
1.4	28.8
1.5	28
1.6	27.5
1.7	27.3
1.8	27
1.9	27
2	26.9
2.1	26.8
2.2	26.7
2.3	26.6
2.4	26.5
2.5	26.4
2.6	26.3
2.7	26.2
2.8	26.1
2.9	26
3	26
];

tx3 = linspace(0,Exp3_I(end,1),Nsample);
itmp3 = interp1(Exp3_I(:,1),Exp3_I(:,2),tx3,'PCHIP');
dt3 = tx3(2)-tx3(1);
ts3 = dt3*(0:Nsample*2-1);

win3 = window_cos(0,Nsample*2,dt3,max(tx3),max(ts3)*0.98 );
ist3 = [itmp3 itmp3(end)*ones(1,Nsample)].*win3;

ts3 = ts3*1e-6;

fpath=['E:\ProjectFiles\'];
fname=['exp3'];
write_source_txt(ts3, ist3, fpath, fname)
figure(10)
plot(ts3,ist3);

Exp4_I=[0	0
        0.005  0
0.025	1.3
0.05	5.1
0.075	10
0.1	15
0.125	17.5
0.15	21
0.175	24
0.2	26.5
0.225	28
0.25	30
0.275	31
0.3	32
0.325	32.3
0.35	33
0.375	33.5
0.4	34.1
0.425	34.5
0.45	34.6
0.475	35
0.5	35.1
0.525	35.1
0.55	35.1
0.575	35.2
0.6	35.3
0.625	35.1
0.65	35
0.675	34.9
0.7	34.9
];


tx4 = linspace(0,Exp4_I(end,1),Nsample);
itmp4 = interp1(Exp4_I(:,1),Exp4_I(:,2),tx4,'PCHIP');
dt4 = tx4(2)-tx4(1);
ts4 = dt4*(0:Nsample*2-1);

win4 = window_cos(0,Nsample*2,dt4,max(tx4),max(ts4)*0.98 );
ist4 = [itmp4 itmp4(end)*ones(1,Nsample)].*win4;

ts4 = ts4*1e-6;

fpath=['E:\ProjectFiles\'];
fname=['exp4'];
write_source_txt(ts4, ist4, fpath, fname)
figure(11)
plot(ts4,ist4);