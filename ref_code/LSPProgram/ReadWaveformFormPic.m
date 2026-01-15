clear
clc
%% read the picture
I = imread('1.jpg');
%% restore to date
r = I(:,:,1);
[m, n] = size(r);
x = 0;y = zeros(1,n);
comp = (256/2)*ones(m,n);
c = bsxfun(@gt,r,comp);
r(c) = 255; r(~c) = 0;
for ii = 3:(n-3)   %remove bundary
    for jj = 3:(m-2)
        if(r(jj,ii) == 0)
            x = x+1;
            y(x) =m - jj;
            break;
        end
    end
end
y = y(1:x);
x = 1:x;
%% plot the new line
figure(2);
plot(x,y);
