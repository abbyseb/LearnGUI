function targetVectors = GetCalypsoTargetVectors(calypsoData)
%% targetVectors = GetCalypsoTargetVectors(calypsoData)
% ------------------------------------------
% FILE   : GetCalypsoTargetVectors.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-06-07 Created.
% ------------------------------------------
% PURPOSE
%   Extract the displacement vectors from each individual beacon to the
%   target position in the calypso report.
%
% ------------------------------------------
% INPUT
%   calypsoData:    The Calypso data which is read from the Calypso report
%                   using "ReadCalypsoReport.m"
% ------------------------------------------
% OUTPUT
%   targetVectors:  Three cells each containing the displacement vector
%                   from beacon 1, 2, and 3 to the target position.

%%

for b = 1:3
    targetVectors{b} = [];
end

for k = 1:length(calypsoData.Target.Time);
    targetTime = calypsoData.Target.Time(k);
    for b = 1:3;
        mindT(b) = min(abs(calypsoData.Beacons{b}.Time - targetTime));
        ind_tmp = ...
            find(abs(calypsoData.Beacons{b}.Time - targetTime) == mindT(b));
        ind(b) = ind_tmp(end);
    end
    whichBeacon = find(mindT == min(mindT));
    
    targetVectors{whichBeacon}(end+1,:) = ...
        [calypsoData.Target.LR(k),...
        calypsoData.Target.SI(k),...
        calypsoData.Target.AP(k)] - ...
        [calypsoData.Beacons{whichBeacon}.LR(ind(whichBeacon)),...
        calypsoData.Beacons{whichBeacon}.SI(ind(whichBeacon)),...
        calypsoData.Beacons{whichBeacon}.AP(ind(whichBeacon))];
end