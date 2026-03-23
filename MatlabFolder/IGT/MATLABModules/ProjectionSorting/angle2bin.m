function [binVector,binInfo] = angle2bin(angles,binCenter,binWidth)
%% [binVector,binInfo] = angle2bin(angles,binCenter,binWidth)
% ------------------------------------------
% FILE   : angle2bin.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2015-09-08   Created
% ------------------------------------------
% PURPOSE
%   Convert angles to binInfo for NanoX image reconstruction.
%
% ------------------------------------------
% INPUT
%
%  angle
%   The input angles in degrees.
%
%  binCenter
%   The center angle values of each bin.
%   e.g. 0,30,60,...
%
%  binWidth
%   The width of each bin.
%   If a single value is input, then all binWidth is set to that value.
%   Otherwise, a binWidth vector matching the length of binCenter assigns
%   binWidth(k) as the width of the bin at binCenter(k)
%
% OUTPUT
%
%  binVector
%   The output binning vector. 0 means projection is not used.
%
%  binInfo
%   The output binning matrix which allows a projection to be used in
%   multiple bins.

%% Check input
if ~isvector(angles)
    error('ERROR: The phase signal has to be a vector.');
end;

if length(binWidth) == 1
    binWidth = binWidth(1) * ones(size(angles));
elseif length(binWidth) ~= length(binCenter)
    error('ERROR: The length of binWidth must either be 1 or match the length of binCenter.');
end;

%% Convert angles to binInfo

% Ideally we want to output a bin matrix, which allows a projection to be
% used in multiple bins
% We do that first, but collapse this matrix into the binVector for now in
% order to make it compatible with the current sorting implementation in
% RTK.
for k = 1:length(binCenter)
    angleDiff = mod(angles - binCenter(k),360);
    angleDiff = angleDiff - (angleDiff>180)*360;
    binInfo(:,k) = abs(angleDiff(:)) <= binWidth(k)/2;
end

% Collapse binInfo matrix to a vector
binVector = zeros(length(angles),1);
for k = 1:length(binCenter)
    binVector(find(binInfo(:,k))) = k;
end

return;