function GenerateMarkerGroundTruth(markerDir,markerDirRef,sortFile,sortFileRef,tumorDirRef,outputDir)
%% GenerateMarkerGroundTruth(markerDir,markerDirRef,sortFile,sortFileRef,tumorDirRef,outputDir)
% ------------------------------------------
% FILE   : GenerateMarkerGroundTruth.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-03-08
% ------------------------------------------
% PURPOSE
%   Convert the marker data in markerDir to equivalent tumor centroid
%   locations as estimated from markerRefDir and tumorDir.
% ------------------------------------------
% INPUT
%   markerDir:      The marker data directory to be converted.
%   markerDirRef:   The reference marker data used to estimate the distance
%                   from marker to tumor centroid
%   sortFile:       The projection sorting .csv file for the marker data
%                   set to be converted.
%   sortFileRef:    The projection sorting .csv file for the reference
%                   marker dataset.
%   tumorDirRef:    The tumor model directory (containing .mha files).
%   outputDir:      (OPTIONAL) Where the output .csv files will be stored.
%                   Default is the current working directory.

%% Input check
if nargin < 5
    outputDir = pwd;
end

if nargin < 1
    markerDir = uigetdir(pwd,'Select marker data directory to be converted.');
    markerDirRef = uigetdir(pwd,'Select reference marker data directory.');
    sortFile = uigetfilepath('*.csv','Select sorting file for the marker data to be converted.');
    sortFileRef = uigetfilepath('*.csv','Select sorting file for the reference marker data.');
    tumorDirRef = uigetdir(pwd,'Select tumor model directory.');
    outputDir = uigetdir(pwd,'Select output directory.');
end

if ~exist(outputDir,'dir');
    mkdir(outputDir);
end;
%% Reading sorting information
sortInfo_ref = csvread(sortFileRef);
sortInfo = csvread(sortFile);
NBin = max(sortInfo);

%% Read marker data from path*.txt file

% Read reference set
fl_ref = lscp(fullfile(markerDirRef,'path*.txt'));
for k = 1:size(fl_ref,1);
    tmp = textread(fl_ref(k,:));
    markerTrj_ref{k} = tmp(:,6:8);
    markerTrj_ref{k}(:,2) = -markerTrj_ref{k}(:,2);
    
    for k_bin = 1:NBin;
        markerMeanPos_ref{k}(k_bin,:) = ...
            mean(markerTrj_ref{k}(find(sortInfo_ref(:)==k_bin & markerTrj_ref{k}(:,2)~=0),:));
    end;
end;
save(fullfile(outputDir,'MarkerTrj_Ref.mat'),'markerTrj_ref','markerMeanPos_ref');

% Read marker dataset to be converted
fl = lscp(fullfile(markerDir,'path*.txt'));
for k = 1:size(fl,1);
    tmp = textread(fl(k,:));
    markerTrj{k} = tmp(:,6:8);
    markerTrj{k}(:,2) = -markerTrj{k}(:,2);
    
    for k_bin = 1:NBin;
        markerMeanPos{k}(k_bin,:) = ...
            mean(markerTrj{k}(find(sortInfo(:)==k_bin & markerTrj{k}(:,2)~=0),:));
    end;
end;
save(fullfile(outputDir,'MarkerTrj.mat'),'markerTrj','markerMeanPos');

%% Write ground truth .csv file

% Get tumor centroids in reference data first
fl_mha = lscp(fullfile(tumorDirRef,'*.mha'));
for k_bin = 1:NBin
    [info,M] = MhaRead(fl_mha(k_bin,:));
    tumorCentroids(k_bin,:) = GetMHAImageCentroid(info,M);
end

% Calculate ground truth
% First column is frame number
for k = 1:size(fl,1);
    for n = 1:size(markerTrj{k},1)
        groundTruth{k}(n,:) = ...
            [n, markerTrj{k}(n,:) + tumorCentroids(sortInfo(n),:) - markerMeanPos_ref{k}(sortInfo(n),:)];
    end
    groundTruth{k}(:,3) = -groundTruth{k}(:,3);
    empty_ind = find(markerTrj{k}(:,2) == 0);
    groundTruth{k}(empty_ind,:) = [];
    csvwrite(fullfile(outputDir,num2str(k,'Marker3DTrj%02d.csv')),groundTruth{k});
end;