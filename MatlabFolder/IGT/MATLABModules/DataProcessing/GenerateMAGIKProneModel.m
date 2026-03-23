function GenerateMAGIKProneModel(inputDir,outputDir)
%% GenerateMAGIKProneModel(inputDir,outputDir)
% ------------------------------------------
% FILE   : GenerateMAGIKProneModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-06-25  Created.
% ------------------------------------------
% PURPOSE
%   Generate prone model for MAGIK.
% ------------------------------------------
% INPUT
%   inputDir:       The directory containing the original models (with the
%                   TumorModel and BGModel folders).
%   outputDir:      The output directory.

%% 
if ~exist(fullfile(outputDir,'TumorModel'),'dir')
    mkdir(fullfile(outputDir,'TumorModel'));
end
if ~exist(fullfile(outputDir,'BGModel'),'dir')
    mkdir(fullfile(outputDir,'BGModel'));
end

fl = lscell(fullfile(inputDir,'TumorModel','*.mha'));
for k = 1:length(fl)
    [~,name,ext] = fileparts(fl{k});
    rotateMHAAxial180(fl{k},fullfile(outputDir,'TumorModel',[name,ext]));
end

fl = lscell(fullfile(inputDir,'BGModel','*.mha'));
for k = 1:length(fl)
    [~,name,ext] = fileparts(fl{k});
    rotateMHAAxial180(fl{k},fullfile(outputDir,'BGModel',[name,ext]));
end
