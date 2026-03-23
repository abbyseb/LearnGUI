function dP = projTruncCorrect(dP0)
%% dP = projTruncCorrect(dP0)
% ------------------------------------------
% FILE   : projTruncCorrect.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-10-06   Created.
% ------------------------------------------
% PURPOSE
%   Correct for truncation artifacts and intensity mismatch caused by
%   objects outside of FOV in CBCT difference projections.
%
% ------------------------------------------
% INPUT
%
%  dP0
%   The original difference projection
%
% OUTPUT
%  dP
%   The corrected difference projection

%% Some simple input check and calculation

% X and Y imensions must be even numbers
if mod(size(dP0,1),2)~=0 || mod(size(dP0,2),2)~=0
    error('ERROR: Projection x and y dimensions must be even numbers.');
end;

dP = dP0;

% Do it in one dimension first
for k = 1:size(dP0,3);
    tmp1 = dP0(end/2,:,k); a = mean(tmp1(:));
    tmp2 = dP0(end/2+1,:,k); b = mean(tmp2(:));
    
    for p = 1:size(dP0,1)/2;
        tmp1 = dP0(p,:,k); tmp2 = dP0(end-p+1,:,k);
        dP(p,:,k) = dP0(p,:,k) + (a - mean(tmp1(:)));
        dP(end-p+1,:,k) = dP0(end-p+1,:,k) + (b - mean(tmp2(:)));
    end;
end;

% Then the other dimension
for k = 1:size(dP0,3);
    tmp1 = dP(:,end/2,k); a = mean(tmp1(:));
    tmp2 = dP(:,end/2+1,k); b = mean(tmp2(:));
    
    for p = 1:1:size(dP0,2)/2;
        tmp1 = dP(:,p,k); tmp2 = dP(:,end-p+1,k);
        dP(:,p,k) = dP(:,p,k) + (a - mean(tmp1(:)));
        dP(:,end-p+1,k) = dP(:,end-p+1,k) + (b - mean(tmp2(:)));
    end;
end;

return;