function Merge4DDVF(dvfList,outputFile,useParallel)
%% Merge4DDVF(dvfList,outputFile,useParallel)
% ------------------------------------------
% FILE   : Merge4DDVF.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2016-06-27  Created.
% ------------------------------------------
% PURPOSE
%   Combine a list of DVF files into a 4D DVF mha file.
% ------------------------------------------
% INPUT
%   dvfList:        The list of dvf mha files in cell format.
%   outputFile:     Output file path.
%   useParallel:    (Optional) Whether or not to use parallel computing.
%                   Default: false
%% Create output directory

if nargin < 2
    outputFile = fullfile(pwd,'4DDVF.mha');
end

if nargin < 3
    useParallel = false;
end

%% Header
dvfInfo = MhaRead(dvfList{1});
dvfInfo.Filename = outputFile;
dvfInfo.NumberOfDimensions = dvfInfo.NumberOfDimensions + 1;
imat = eye(dvfInfo.NumberOfDimensions);
dvfInfo.TransformMatrix = imat(:)';
dvfInfo.Offset = [dvfInfo.Offset, 0];
dvfInfo.CenterOfRotation = [dvfInfo.CenterOfRotation, 0];
dvfInfo.PixelDimensions = [dvfInfo.PixelDimensions 1];
dvfInfo.Dimensions = [dvfInfo.Dimensions, length(dvfList)];

%% Reading DVF and writing it as a 4DDVF file
if useParallel
    parfor k = 1:length(dvfList);
        [~,D(:,:,:,:,k)] = MhaRead(dvfList{k});
    end;
else
    for k = 1:length(dvfList);
        [~,D(:,:,:,:,k)] = MhaRead(dvfList{k});
    end;
end
MhaWrite(dvfInfo,permute(D,[1 2 3 5 4]),outputFile);

return;