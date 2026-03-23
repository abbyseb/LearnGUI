function RotateMHAImageAxial(inputList,angles,outputDir,bgValue)
%% RotateMHAImageAxial(inputList,angles,outputDir,bgValue)
% ------------------------------------------
% FILE   : Build4DDVF.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2016-05-12  Created.
% ------------------------------------------
% PURPOSE
%   Axially rotate a list of mha images using the angles specified.
% ------------------------------------------
% INPUT
%   inputList:      The list of input image files in cells.
%   angles:         The angles by which the images are to be rotated.
%   outputDir:      The path to the output directory. Default is
%                   "AxialRotation".
%   bgValue:        The value assigned to the background due to image
%                   cropping in the rotation process. Default is 0.
%% Input check

if length(angles) ~= length(inputList)
    error('ERROR: Length of angles must match the length of inputList.');
end;

if nargin < 3
    outputDir = fullfile(pwd,'AxialRotation');
end

if ~exist(outputDir,'dir');
    mkdir(outputDir);
end;

%% Rotation
for k = 1:length(inputList);
    [~,name] = fileparts(inputList{k});
    [info,M] = MhaRead(inputList{k});
    M = permute(M,[1 3 2]);
    for p = 1:size(M,3);
        M(:,:,p) = imrotate(M(:,:,p),angles(k),'bicubic','crop');
    end;
    M = permute(M,[1 3 2]);
    
    if nargin >= 4
        M(M==0) = bgValue;
    end
    
    MhaWrite(info,M,fullfile(outputDir,[name,'_AxialRotation.mha']));
end;