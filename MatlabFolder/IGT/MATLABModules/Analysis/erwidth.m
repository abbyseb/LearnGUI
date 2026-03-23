function result = erwidth(lprof,d,frac,automode)
%% result = erwidth(lprof,d,frac,automode)
% ------------------------------------------
% FILE   : erwidth.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-02-04  Created. Does not support automatic detection, yet.
%          2013-02-21  Auto mode supported.
% ------------------------------------------
% PURPOSE
%   Calculate the edge-response width using the linear profiles given. The
%   code promts the user with ginput to specify the max and min values near
%   the edge for each profile given.
%
%   [result1;result2;...] = erwidth(lprof,d,frac,automode)
% ------------------------------------------
% INPUT
%   lprof:          The linear profile in the format of
%                   [lprof1;lprof2;lprof3;...]
%   d:              The unit length (in your desired unit) corresponding to
%                   one pixel in the linear profile.
%   frac:           A value between 0 and 0.5, specifying how the width is
%                   calculated. The width is calculated from the pixel
%                   corresponding to (1-frac) of the maximum and (frac) of
%                   the minimum. e.g. frac=0.1 will give you the width
%                   corresponding to 10% to 90% in the intensity space.
%   automode:       If the user input "auto" for this field, the
%                   code will work in auto mode and does not require the
%                   user to select the boundary points. Input "manual" for
%                   this field if the user wants to select the points
%                   manually.
% ------------------------------------------
% OUTPUT
%   result:         The edge response width values in the desired unit.

%% Checking input

if (frac < 0) | (frac >= 0.5)
    error('ERROR: The value of frac must be between 0 and 0.5');
    return;
end

nprof = size(lprof);    nprof = nprof(1);

%% Loop through each lprof

result = zeros(nprof,1);

for kprof = 1:nprof
    
    if strcmp(automode,'auto')
        l_tmp = lprof(kprof,:);
        mean_tmp = mean(l_tmp);
        mnm(1,2) = mean(l_tmp(l_tmp<mean_tmp));
        mnm(1,2) = mean(l_tmp(l_tmp<mnm(1,2)));
        mnm(2,2) = mean(l_tmp(l_tmp>mean_tmp));
        mnm(2,2) = mean(l_tmp(l_tmp>mnm(2,2)));
        if mean(find(l_tmp>mean_tmp)) < mean(find(l_tmp<mean_tmp))
            l_tmp = l_tmp(end:-1:1);
        end
        lpts = find(l_tmp<=mnm(1,2));   mnm(1,1) = max(lpts);
        hpts = find(l_tmp>=mnm(2,2));   mnm(2,1) = min(hpts);
    else
        plot(lprof(kprof,:));
        mnm = ginput(2);
    end
    
    edgef = lprof(kprof,floor(min(mnm(:,1))):ceil(max(mnm(:,1))));
    if mean(diff(edgef)) < 0
        edgef = edgef(end:-1:1);
    end
    h = mnm(2,2) - mnm(1,2);
    b(1) = mnm(1,2) + h*frac;
    b(2) = mnm(2,2) - h*frac;
    
    % MATLAB's built in "interp1.m" cannot deal with non-monotonic data,
    % which is often the case with noisy image. Hence we have to do this
    % our own way.
    edgefp = edgef; edgefp(1:end-1) = edgef(2:end);
    for p = 1:2
        ind = find(edgef <= b(p) & edgefp >= b(p));
        ind = ind(1);
        x(p) = interp1([edgef(ind),edgef(ind+1)],[ind,ind+1],b(p));
    end
        
    result(kprof) = abs(x(2)-x(1)) * d;
end

return;
    