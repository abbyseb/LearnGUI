function GenerateTrackingGroundTruth(trackingFile,trackingFileRef,sortFile,sortFileRef,tumorDirRef,outputFile)
%% GenerateTrackingGroundTruth(trackingFile,trackingFileRef,sortFile,sortFileRef,tumorDirRef,outputFile)
% ------------------------------------------
% FILE   : GenerateTrackingGroundTruth.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-06-01
% ------------------------------------------
% PURPOSE
%   Convert tracking data into tumor model equivalent locations.
% ------------------------------------------
% INPUT
%   trackingFile:     The tracking data file to be converted.
%   trackingFileRef:  The reference tracking data used to estimate the distance
%                     from target to tumor centroid
%   sortFile:         The projection sorting .csv file for the tracking data
%                     set to be converted.
%   sortFileRef:      The projection sorting .csv file for the reference
%                     tracking dataset.
%   tumorDirRef:      The tumor model directory (containing .mha files).
%   outputFile:       Output filename. Default is "TrackingGroundTruth.csv"
%                     at current directory. (OPTIONAL)

%% Input check
if nargin < 5
    outputDir = pwd;
end

if nargin < 1
    trackingFile = uigetfilepath('*.csv','Select tracking data file to be converted.');
    if isnumeric(trackingFile); error('ERROR: No file was selected.'); end;
    trackingFileRef = uigetfilepath('*.csv','Select reference tracking data file.');
    if isnumeric(trackingFileRef); error('ERROR: No file was selected.'); end;
    sortFile = uigetfilepath('*.csv','Select sorting file for the tracking data to be converted.');
    if isnumeric(sortFile); error('ERROR: No file was selected.'); end;
    sortFileRef = uigetfilepath('*.csv','Select sorting file for the reference tracking data.');
    if isnumeric(sortFileRef); error('ERROR: No file was selected.'); end;
    tumorDirRef = uigetdir(pwd,'Select tumor model directory.');
    if isnumeric(tumorDirRef); error('ERROR: No directory was selected.'); end;
end

if nargin < 6
    outputFile = fullfile(pwd,'TrackingGroundTruth.csv');
end

%% Reading sorting information
sortInfo_ref = csvread(sortFileRef);
sortInfo = csvread(sortFile);
NBin = max(sortInfo);

%% Read marker data from path*.txt file

% Read reference set
tracking_ref = csvread(trackingFileRef);
tracking_ref(:,3) = -tracking_ref(:,3);
for k_bin = 1:NBin;
    trackingMeanPos_ref(k_bin,:) = ...
        mean(tracking_ref(find(sortInfo_ref(:)==k_bin & ~isnan(tracking_ref(:,3))),2:4),1);
end;

% Read tracking data to be converted
tracking = csvread(trackingFile);
tracking(:,3) = -tracking(:,3);
for k_bin = 1:NBin;
    trackingMeanPos(k_bin,:) = ...
        mean(tracking(find(sortInfo(:)==k_bin & ~isnan(tracking(:,3))),2:4),1);
end;
%% Write ground truth .csv file

% Get tumor centroids in reference data first
fl_mha = lscp(fullfile(tumorDirRef,'*.mha'));
for k_bin = 1:NBin
    [info,M] = MhaRead(fl_mha(k_bin,:));
    tumorCentroids(k_bin,:) = GetMHAImageCentroid(info,M);
end

% Calculate ground truth
% First column is frame number
for n = 1:size(tracking,1)
    groundTruth(n,:) = ...
        [n, tracking(n,2:4) + tumorCentroids(sortInfo(n),:) - trackingMeanPos_ref(sortInfo(n),:)];
end
groundTruth(:,3) = -groundTruth(:,3);
empty_ind = find(tracking(:,3) == 0);
groundTruth(empty_ind,:) = [];
[~,name] = fileparts(trackingFile);
csvwrite(fullfile(outputDir,['GroundTruth_',name,'.csv']),groundTruth);