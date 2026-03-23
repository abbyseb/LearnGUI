function fileTimes = GetVarianTimeStamps(fl)
%% fileTimes = GetVarianTimeStamps(fl)
% ------------------------------------------
% FILE   : GetVarianTimeStamps.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-06-08 Created.
% ------------------------------------------
% PURPOSE
%   Get Varian HNC/HND time stamps from a list of files.
%
% ------------------------------------------
% INPUT
%   fl:       The list of filenames in cell format.
% ------------------------------------------
% OUTPUT
%   fileTimes:   The file time stamps in MATLAB datetime format.

%% 

for k = 1:length(fl);
    info = ProjRead(fl{k});
    tYear = str2double(info.bCreationDate(1:4));
    tMonth = str2double(info.bCreationDate(5:6));
    tDay = str2double(info.bCreationDate(7:8));
    tHour = str2double(info.bCreationTime(1:2));
    tMin = str2double(info.bCreationTime(4:5));
    tSec = str2double(info.bCreationTime(7:8));
    fileTimes(k) = datetime(tYear,tMonth,tDay,tHour,tMin,tSec);
end

return;