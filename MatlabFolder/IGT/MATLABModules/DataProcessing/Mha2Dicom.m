function Mha2Dicom(mhaFile,outputDir,convFun)
%% Mha2Dicom(mhaFile,outputDir,convFun)
% ------------------------------------------
% FILE   : Mha2Dicom.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-20   Created.
% ------------------------------------------
% PURPOSE
%   Convert .mha RTK images (in attenuation) to dicom slices (in Hounsfield
%   unit).
%
% ------------------------------------------
% INPUT
%   mhaFile:        File path to the mha file.
%   outputDir:      Path to the output directory storing the dicom slices.
%   convFun:        (Optional) A function handle used to convert from
%                   attenuation to Hounsfield unit. If not input, default
%                   will be used:

%%

% MU_WATER
MU_WATER = 1.545e-2;

% Default conversion function handle
if nargin < 3
    convFun = @(x) 1000 * x/MU_WATER - 1000;
end
    
% Read mha file
[info,M] = MhaRead(mhaFile);
M = permute(M,[1 3 2]);
M = M(:,end:-1:1,end:-1:1);

% Convert
M = convFun(M);

% Create outputdir if it doesn't exist
if exist(outputDir,'dir') == 0
    mkdir(outputDir);
end

for k = 1:size(M,3)
    dicomwrite(M(:,:,k),fullfile(outputDir,num2str(k,'%04d.dcm')),...
        'PixelSpacing',[info.PixelDimensions(1);info.PixelDimensions(3)],...
        'SliceThickness',info.PixelDimensions(2));
end;

return;