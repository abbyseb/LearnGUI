function [interpNums,interpDatetimes] = interpolateDatetime(dateTimes)
%% [interpNums,interpDatetimes] = interpolateDatetime(dateTimes)
% ------------------------------------------
% FILE   : interpolateDatetime.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-04-28 Created.
% ------------------------------------------
% PURPOSE
%   Interpolate to produce a strictly monotonically increasing or decreasing 
%   datenum array. The first output will be in datenum format and the second
%	in datetime format.
%
% ------------------------------------------
% INPUT
%   dateTimes:   The original datetime array.
% ------------------------------------------
% OUTPUT
%   interpNums:  		The interpolated datenum array.
%	interpDatetimes:	The interpolated datetime array.

%% 

dateNums = datenum(dateTimes);
x = 1:length(dateNums);
stepPos = find(diff(dateNums) ~= 0);
stepPos = [1; stepPos(:) + 1;length(dateNums)+1];
values = dateNums(stepPos(1:end-1));
values(end+1) = values(end) + ...
    (values(end) - values(end-1)) * ...
     (stepPos(end) - stepPos(end-1)) / (stepPos(end-1) - stepPos(end-2));
interpNums = interp1(stepPos,values,x);
interpDatetimes = datetime(interpNums,'ConvertFrom','datenum');

return;