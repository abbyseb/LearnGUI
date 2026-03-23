function data = ReadCalypsoReport(filename)
%% data = ReadCalypsoReport(filename)
% ------------------------------------------
% FILE   : ReadCalypsoReport.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-04-28 Created.
% ------------------------------------------
% PURPOSE
%   Read Calypso .xlsb report. The positions are recorded in mm.
%
% ------------------------------------------
% INPUT
%   filename:       Path to the Calypso .xlsb report file.
% ------------------------------------------
% OUTPUT
%   data:           The data read into a struct.

%% Input check
if nargin < 1
    filename = uigetfilepath(...
        {'*.xlsb;*.XLSB;','XLSB file (*.xlsb,*.XLSB)'},...
        'Select the Calypso report file to read', ...
        pwd);
    
    if isnumeric(filename)
        error('ERROR: No file was selected.');
    end
end

%% Read
data = struct;

data.Target = struct;
data.Target.LR = xlsread(filename,'RawTrackingData','A:A');
data.Target.LR = data.Target.LR * 10;
data.Target.SI = xlsread(filename,'RawTrackingData','B:B');
data.Target.SI = data.Target.SI * 10;
data.Target.AP = xlsread(filename,'RawTrackingData','C:C');
data.Target.AP = data.Target.AP * 10;

% Each individual beacon
beaconNo = xlsread(filename,'RawTrackingData','G:G');

beaconX = xlsread(filename,'RawTrackingData','H:H');

N = min([length(data.Target.LR),length(beaconNo),length(beaconX)]);

beaconX = beaconX(1:N);
data.Target.LR = data.Target.LR(1:N);
data.Target.SI = data.Target.SI(1:N);
data.Target.AP = data.Target.AP(1:N);
beaconNo = beaconNo(1:N);

beaconY = xlsread(filename,'RawTrackingData','I:I');
beaconY = beaconY(1:N);
beaconZ = xlsread(filename,'RawTrackingData','J:J');
beaconZ = beaconZ(1:N);

for k = 1:3
    data.Beacons{k} = struct;
    data.Beacons{k}.LR = beaconX(beaconNo == k) * 10;
    data.Beacons{k}.SI = beaconY(beaconNo == k) * 10;
    data.Beacons{k}.AP = beaconZ(beaconNo == k) * 10;
    data.Beacons{k}.TimeZone = hours([]);
    data.Beacons{k}.Time = datetime([],[],[]);
end

[~,timeStrs] = xlsread(filename,'RawTrackingData','D:D');
for k = 2:(length(data.Target.LR) + 1)
    data.Target.TimeZone(k-1) = ...
        hours(str2double(timeStrs{k}(end-5:end-3))) + ...
        minutes(str2double(timeStrs{k}(end-1:end)));
    data.Target.Time(k-1) = datetime(...
        str2double(timeStrs{k}(1:4)),...
        str2double(timeStrs{k}(6:7)),...
        str2double(timeStrs{k}(9:10)),...
        str2double(timeStrs{k}(12:13)),...
        str2double(timeStrs{k}(15:16)),...
        str2double(timeStrs{k}(18:19)) ...
        + 0.001 * str2double(timeStrs{k}(21:23)));
end

[~,timeStrs_beacons] = xlsread(filename,'RawTrackingData','K:K');
for k = 2:(length(data.Target.LR) + 1)
    data.Beacons{beaconNo(k-1)}.TimeZone(end+1) = ...
        hours(str2double(timeStrs_beacons{k}(end-5:end-3))) + ...
        minutes(str2double(timeStrs_beacons{k}(end-1:end)));
    data.Beacons{beaconNo(k-1)}.Time(end+1) = datetime(...
        str2double(timeStrs_beacons{k}(1:4)),...
        str2double(timeStrs_beacons{k}(6:7)),...
        str2double(timeStrs_beacons{k}(9:10)),...
        str2double(timeStrs_beacons{k}(12:13)),...
        str2double(timeStrs_beacons{k}(15:16)),...
        str2double(timeStrs_beacons{k}(18:19)) ...
        + 0.001 * str2double(timeStrs_beacons{k}(21:23)));
end

% Beam on info
[~,timeStrs_Beam] = xlsread(filename,'BeamOn Data','B:B');
[~,boolStrs_Beam] = xlsread(filename,'BeamOn Data','A:A');
for k = 1:min(length(timeStrs_Beam)-1,length(boolStrs_Beam)-2);
    data.Beam.TimeZone(k) = ...
        hours(str2double(timeStrs_Beam{k+1}(end-5:end-3))) + ...
        minutes(str2double(timeStrs_Beam{k+1}(end-1:end)));
    data.Beam.Time(k) = datetime(...
        str2double(timeStrs_Beam{k+1}(1:4)),...
        str2double(timeStrs_Beam{k+1}(6:7)),...
        str2double(timeStrs_Beam{k+1}(9:10)),...
        str2double(timeStrs_Beam{k+1}(12:13)),...
        str2double(timeStrs_Beam{k+1}(15:16)),...
        str2double(timeStrs_Beam{k+1}(18:19)) ...
        + 0.001 * str2double(timeStrs_Beam{k+1}(21:23)));
    data.Beam.Status(k) = strcmpi('On',boolStrs_Beam{k+2});
end

%% Infer missing target position from beacon information
badBeacons = [];
for b = 1:length(data.Beacons)
    if isempty(data.Beacons{b}.LR) || all(data.Beacons{b}.LR == 0) || all(isnan(data.Beacons{b}.LR))
        badBeacons(end+1) = b;
    end
end
data.Beacons = rmcell1D(data.Beacons,badBeacons);

data.Target.InferredPositions = ...
    [data.Target.LR,data.Target.SI,data.Target.AP];
indNaN = find(isnan(data.Target.LR));
indOK = find(~isnan(data.Target.LR));
for k = 1:length(indNaN);
    indLastOK = indOK(indOK < indNaN(k));
    indLastOK = indLastOK(end);
    t = data.Target.Time(indNaN(k));
    tLast = data.Target.Time(indLastOK);
    data.Target.InferredPositions(indNaN(k),:) = data.Target.InferredPositions(indLastOK,:);
    for b = 1:length(data.Beacons)
        tmpInd = find(abs(data.Beacons{b}.Time - t) == min(abs(data.Beacons{b}.Time - t)));
        indB(b) = tmpInd(1);
        while isnan(data.Beacons{b}.LR(indB(b))); indB(b) = indB(b) - 1; end;
        indBLast(b) = find(abs(data.Beacons{b}.Time - tLast) == min(abs(data.Beacons{b}.Time - tLast)));
        data.Target.InferredPositions(indNaN(k),:) = data.Target.InferredPositions(indNaN(k),:) ...
            + 1/length(data.Beacons) * [data.Beacons{b}.LR(indB(b)), data.Beacons{b}.SI(indB(b)), data.Beacons{b}.AP(indB(b))] ...
            - 1/length(data.Beacons) * [data.Beacons{b}.LR(indBLast(b)), data.Beacons{b}.SI(indBLast(b)), data.Beacons{b}.AP(indBLast(b))];
    end;
end

return;