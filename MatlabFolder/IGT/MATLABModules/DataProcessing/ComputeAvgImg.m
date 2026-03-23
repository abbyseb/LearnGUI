function ComputeAvgImg(inputList,outputFile)
%% ComputeAvgImg(inputList,outputFile)
% ------------------------------------------
% FILE   : ComputeAvgImg.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-01-04  Created.
% ------------------------------------------
% PURPOSE
%   Compute and save the average image of multiple input images.
% ------------------------------------------
% INPUT
%   inputList:      The list of input image filenames in cell format.
%   outputFile:     The output filename.
%% Input check

if nargin ~= 2
    error('ERROR: inputList and outputFile must be provided.');
end

%% Do stuff
MAvg = 0;
for k = 1:length(inputList)
    [info,M{k}] = MhaRead(inputList{k});
    MAvg = MAvg + M{k}/length(inputList);
end
MhaWrite(info,MAvg,outputFile);