function WarpDVF(dvfFile,transformFile,outputFile,useParallel,verbose)
%% WarpDVF(dvfFile,transformFile,outputFile,useParallel,verbose)
% ------------------------------------------
% FILE   : WarpDVF.m
% AUTHOR : Andy Shieh, ACRD Image X Institute, The University of Sydney
% DATE   : 2018-01-05  Created.
% ------------------------------------------
% PURPOSE
%   Warp DVF by splitting it into scalar volume, transforming each
%   separately, and then combining.
% ------------------------------------------
% INPUT
%   dvfFile:        Path to the DVF .mha file.
%   transformFile:  Path to transformation file.
%   outputFile:     Path to the output DVF file.
%   useParallel:    (Optional) Whether or not to use parallel computing.
%                   Default: false
%   verbose:        (Optional) Whether or not to output progress message to
%                   the terminal.
%                   Default: false

%% Input check
if nargin < 3
    error('ERROR: dvfFile, transformFile, and outputFile must be specified.');
end

if nargin < 4
    useParallel = false;
end

if nargin < 5
    verbose = false;
end

%% Split and warp
[dvfInfo,dvfImg] = MhaRead(dvfFile);
NDim = str2double(dvfInfo.ElementNumberOfChannels);
imgInfo = rmfield(dvfInfo,'ElementNumberOfChannels');
outputCell = {};

if useParallel
    parfor dim = 1:NDim
        [outputInfo{dim},outputCell{dim}] = saveAndWarp(imgInfo,...
            reshape(dvfImg(((dim-1)*prod(imgInfo.Dimensions)+1):dim*prod(imgInfo.Dimensions)),imgInfo.Dimensions),...
            transformFile,verbose);
    end
else
    for dim = 1:NDim
        [outputInfo{dim},outputCell{dim}] = saveAndWarp(imgInfo,...
            reshape(dvfImg(((dim-1)*prod(imgInfo.Dimensions)+1):dim*prod(imgInfo.Dimensions)),imgInfo.Dimensions),...
            transformFile,verbose);
    end
end

outputImg = outputCell{1};
for dim = 2:NDim
    outputImg = cat(length(size(dvfImg))+1,outputImg,outputCell{dim});
end

outputInfo{1}.ElementNumberOfChannels = dvfInfo.ElementNumberOfChannels;
MhaWrite(outputInfo{1},outputImg,outputFile);

end

function [outputInfo,outputImg] = saveAndWarp(info,M,transformFile,verbose)
tempFile = [tempname,'.mha'];
outputTempFile = [tempname,'.mha'];
MhaWrite(info,M,tempFile);
elastixTransform(transformFile,tempFile,verbose,outputTempFile);
[outputInfo,outputImg] = MhaRead(outputTempFile);
system(['del "',tempFile,'"']);
system(['del "',outputTempFile,'"']);
end

