function m = mFinder(Mmax, Href, Theta, currPoint)
epsilon = Mmax*1e-10;
err = Mmax;
H = currPoint(1);
mSup = 1;
mInf = 0;
while abs(err) > epsilon
    m = (mSup+mInf)/2;
    if m == mSup || m == mInf
        err = 0;
    else
        M = Mmax*((1-m)*2/pi.*atan(Theta*...
            (1-m).*(H/Href-1))+m);
        err = M-currPoint(2);
        if err < 0
            mInf = m;
        else
            mSup = m;
        end
    end
end
end