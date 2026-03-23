function Build4DDVF(inputList,refImgFile,outputDir,dvfOrder)
%% Build4DDVF(inputList,refImgFile,outputDir,dvfOrder)
% ------------------------------------------
% FILE   : Build4DDVF.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2016-05-12  Created.
% ------------------------------------------
% PURPOSE
%   Build 4D DVF from a list of input image files in mha format.
% ------------------------------------------
% INPUT
%   inputList:      The list of input image files in cells.
%   refImgFile:     The reference image file used as the "fixed image" in
%                   DIR.
%   outputDir:      The path to the output directory. Default is "4DDVF" in
%                   the current directory.
%   dvfOrder:       The order of which the DVFs are combined. Default is 
%                   [1,2,3,4,5,...,N]
%% Create output directory

if nargin < 4
    dvfOrder = 1:length(inputList);
end

if nargin < 3
    outputDir = fullfile(pwd,'4DDVF');
end

if ~exist(outputDir,'dir');
    mkdir(outputDir);
end;

%% DIR using elastix

elastix = which('elastix.exe');
if isempty(elastix);    elastix = 'elastix';   end
elastix = ['"',elastix,'"'];

transformix = which('transformix.exe');
if isempty(transformix);    transformix = 'transformix';   end
transformix = ['"',transformix,'"'];

% Initialize info struct of 4D DVF
imgInfo = MhaRead(inputList{1});
info = struct;
info.Filename = '4DDVF.mha';
info.Format = 'MHA';
info.CompressedData = 'false';
info.ObjectType = 'image';
info.NumberOfDimensions = 4;
info.BinaryData = 'true';
info.ByteOrder = 'false';
info.TransformMatrix = [1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1];
info.Offset = [imgInfo.Offset, 0];
info.CenterOfRotation = [imgInfo.CenterOfRotation, 0];
info.AnatomicalOrientation = 'RAI';
info.PixelDimensions = [imgInfo.PixelDimensions 1];
info.Dimensions = [imgInfo.Dimensions, length(dvfOrder)];
info.ElementNumberOfChannels = '3';
info.DataType = 'float';
info.DataFile = 'LOCAL';
info.BitDepth = 32;
info.HeaderSize = 380;

tmpDir = tempname;
mkdir(tmpDir);
for k = 1:length(inputList);
    [~,name] = fileparts(inputList{k});
    system([elastix,' -f "',refImgFile,'" -m "',inputList{k},'" -p "',which('Elastix_BSpline.txt'),'" -out "',tmpDir,'" >nul 2>&1']);
    system(['move "',fullfile(tmpDir,'result.0.mha'),'" "',fullfile(outputDir,[name,'_DIR.mha']),'" >nul 2>&1']);
    system([transformix,' -def all -tp "',fullfile(tmpDir,'TransformParameters.0.txt'),'" -out "',tmpDir,'" >nul 2>&1']);
    system(['move "',fullfile(tmpDir,'deformationField.mha'),'" "',fullfile(outputDir,[name,'_DVF.mha']),'" >nul 2>&1']);
    system(['move "',fullfile(tmpDir,'TransformParameters.0.txt'),'" "',fullfile(outputDir,[name,'_Transform.txt']),'" >nul 2>&1']);
end;

% Reading DVF and writing it as a 4DDVF file
dvfList = lscell(fullfile(outputDir,'*_DVF.mha'));
for k = 1:length(dvfList);
    [~,D(:,:,:,:,k)] = MhaRead(dvfList{k});
end;
MhaWrite(info,permute(D(:,:,:,:,dvfOrder),[1 2 3 5 4]),fullfile(outputDir,'4DDVF.mha'));

if exist(tmpDir,'dir'); rmdir(tmpDir,'s');  end;