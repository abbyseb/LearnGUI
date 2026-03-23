function IEC2DICOM(inputFile,outputFile,matchpts)
%% IEC2DICOM(inputFile,outputFile,matchpts)
% ------------------------------------------
% FILE   : IEC2DICOM.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2015-04-09  Created.
% ------------------------------------------
% PURPOSE
%   Convert a mha volume from IEC geometry to Dicom geometry.
%   If matchpts are specified, conversion to HU will also be performed.
% ------------------------------------------
% INPUT
%   inputFile:      File path to the input MHA file.
%   outputFile:     Output file path.
%   matchpts:       A 1 by 2 vector defining the percentile match points.
%                   e.g. [10,90]
%                   Default: If not given, no conversion is performed
%                   The user can also pass a function handle and the code
%                   will directly use the function handle for scaling.
%% Convert from IEC to DICOM geometry standard

[info,M] = MhaRead(inputFile);

M = permute(M,[1 3 2]);
% Sagittal and axial dimension needs to be reversed
M = M(:,end:-1:1,end:-1:1);

% Change header information
info.Dimensions = info.Dimensions([1 3 2]);
info.PixelDimensions = info.PixelDimensions([1 3 2]);
info.Offset = -(info.Dimensions-1)/2 .* info.PixelDimensions;
    
%% Convert to HU if matchpts is specified

if nargin > 2
    if length(matchpts) == 1
        M = matchpts(M);
    else
        Ma = prctile(M(:),matchpts(1));
        Mb = prctile(M(:),matchpts(2));
        M = (M - Ma) / (Mb - Ma) * 1000 - 1000;
    end;
end;

%% Write the image
MhaWrite(info,M,outputFile);

return;