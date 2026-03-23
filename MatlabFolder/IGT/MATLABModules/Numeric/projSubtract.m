function dP = projSubtract(oriProj,newProj)
%% dP = projSubtract(oriProj,newProj)
% ------------------------------------------
% FILE   : projSubtract.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-12-01   Created.
%          2014-07-04   Not used anymore. Use imgEqualize instead.
% ------------------------------------------
% PURPOSE
%   Subtract two projection data set (stored in MATLAB) in a more "sensible" way.
%   At the moment, a linear regression method is used.
%
% ------------------------------------------
% INPUT
%
%  oriProj
%   The original projection data set, in the dimension of [x,y,n], where n
%   is the number of projection views.
%
%  newProj
%   The new projection data set, in the dimension of [x,y,n], where n
%   is the number of projection views.
%
% OUTPUT
%  dP
%   dP = oriProj - newProj, after scaling.

%% Some simple input check and calculation

if ~isequal(size(oriProj),size(newProj))
    error('ERROR: The original and new projection data set is not compatible to each other.');
end;

dP = single(zeros(size(newProj)));

for k = 1:size(oriProj,3);
sH = valueEqualize(newProj(end/4:end*3/4,end/4:end*3/4,k),oriProj(end/4:end*3/4,end/4:end*3/4,k),[10,90]);
dP(:,:,k) = oriProj(:,:,k) - sH(newProj(:,:,k));
end;

return;