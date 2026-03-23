function DelUselessTBProj(dirPath)
%% DelUselessTBProj(dirPath)
% ------------------------------------------
% FILE   : DelUselessTBProj.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-12-13  Created.
% ------------------------------------------
% PURPOSE
%   Delete Truebeam projection files with very small file sizes that
%	are not used in image reconstructions.
%   DO NOT USE ON YOUR ONLY RAW DATA COPY.
% ------------------------------------------
% INPUT
%   dirPath:	The projection directory path.
%
%% 
    
if nargin < 1
    dirPath = pwd;
end

fileList = dir(fullfile(dirPath,'*.xim'));
for k = 1:length(fileList);
    fileBytes(k) = fileList(k).bytes;
end

[~,indOutliers] = hampel(fileBytes);
indOutliers = find(indOutliers);

for k = 1:length(indOutliers)
    delete(fullfile(dirPath,fileList(indOutliers(k)).name));
end
