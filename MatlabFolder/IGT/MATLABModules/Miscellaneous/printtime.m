function [timeStr,timeComponents] = printtime(timeVar)
%% [timeStr,timeComponents] = printtime(timeVar)
% ------------------------------------------
% FILE   : printtime.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-06-08   Creation
% ------------------------------------------
% PURPOSE
%   Prtin MATLAB datetime or duration to full ms precision.
%
% ------------------------------------------
% INPUT
%   timeVar:        The MATLAB datetime or duration variable.
% ------------------------------------------
% OUTPUT
%   timeStr:        The printed string.
%   timeComponents: The year, month, day, hour, minute, second of the
%                   datetime varaible and hour, minute, second of the
%                   duration variable.

%%
if isdatetime(timeVar)
    timeComponents = [...
        timeVar.Year, timeVar.Month, timeVar.Day,...
        timeVar.Hour, timeVar.Minute, timeVar.Second...
        ];
    timeStr = datestr(timeVar);
    timeStr(end+1:end+4) = ['.',num2str(round(mod(timeVar.Second,1)*1000),'%03d')];
elseif isduration(timeVar)
    if timeVar < 0
        timeVar_tmp = -timeVar;
    else
        timeVar_tmp = timeVar;
    end;
    tNum = datenum(timeVar_tmp);
    tHour = floor(tNum / datenum(hours(1)));
    tMin = floor(mod(tNum,datenum(hours(1))) / datenum(minutes(1)));
    tSec = mod(tNum,datenum(minutes(1))) / datenum(seconds(1));
    timeComponents = (-2*(timeVar<0) + 1) * [tHour,tMin,tSec];
    timeStr = ...
        num2str([tHour,tMin,floor(tSec),round(mod(tSec,1)*1000)],...
        '%02d:%02d:%02d.%03d');
    if timeVar < 0
        timeStr = ['-',timeStr];
    end
else
    error('ERROR: timeVar must be a datetime or duration variable.');
end
