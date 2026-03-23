function fileTimes = GetFileTimeStamps(fl)
%% fileTimes = GetFileTimeStamps(fl)
% ------------------------------------------
% FILE   : GetFileTimeStamps.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-04-28 Created.
% ------------------------------------------
% PURPOSE
%   Get file time stamps from a list of files.
%
% ------------------------------------------
% INPUT
%   fl:       The list of filenames in cell format.
% ------------------------------------------
% OUTPUT
%   fileTimes:   The file time stamps in MATLAB datetime format.

%% 

for k = 1:length(fl);
    fileInfo = dir(fl{k});
    fileTimes(k) = datetime(fileInfo.date);
end

return;