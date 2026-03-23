function GenerateMAGIKModel(imgList,gtvList,outputDir,bgValue)
%% GenerateMAGIKModel(imgList,gtvList,outputDir)
% ------------------------------------------
% FILE   : GenerateMAGIKModel.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-11-03  Created.
% ------------------------------------------
% PURPOSE
%   Generate the background and target models from a list of image and
%   gtv files.
% ------------------------------------------
% INPUT
%   imgList:        List of image files (cell).
%   gtvList:        List of GTV files (cell).
%   outputDir:      The output path.

%% Input check

if length(imgList) ~= length(gtvList)
    error('ERROR: The number of image files must match with the number of GTV files.');
end

if nargin < 3
    outputDir = pwd;
end

if nargin < 4
    bgValue = 0.002;
end

%% Generate models
mkdir(fullfile(outputDir,'BGModel'));
mkdir(fullfile(outputDir,'TumorModel'));

for k = 1:length(imgList)
    [infoBG,M] = MhaRead(imgList{k});
    infoTumor = infoBG;
    [~,G] = MhaRead(gtvList{k});
    
    % BG
    BG = M;
    BG(G>0) = bgValue;
    MhaWrite(infoBG,BG,fullfile(outputDir,'BGModel',num2str(k,'Model_%02d.mha')));
    
    % Tumor
    corner = GetBoundingBoxCorners(G>0);
    Tumor = ExtractViaCorners(M .* (G > 0), corner);
    infoTumor.Dimensions = size(Tumor);
    infoTumor.Offset = (corner(1,:) - [1,1,1]) .* infoTumor.PixelDimensions + infoBG.Offset;
    MhaWrite(infoTumor, Tumor,fullfile(outputDir,'TumorModel',num2str(k,'Model_%02d.mha')));
end

return