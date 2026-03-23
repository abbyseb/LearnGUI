function ShiftMhaImage(imgFile,shift,outputFile)
%% ShiftMhaImage(imgFile,shift,outputFile)
% ------------------------------------------
% FILE   : ShiftMhaImage.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2016-08-02  Created.
% ------------------------------------------
% PURPOSE
%   Shift Mha image by a shift vector.
% ------------------------------------------
% INPUT
%   imgFile:    The path of the mha image to be shifted.
%   shift:      The 1*N shift vector where N is the image dimension.
%   outputFile: The path of the output file.
%               (default: imgFile_Shifted.mha at the same directory as the 
%               input file)

%% Input check

if nargin < 3
    [path,name,ext] = fileparts(imgFile);
    outputFile = fullfile(path,[name,'_Shifted',ext]);
end

%%
[info,M] = MhaRead(imgFile);
info.Offset = info.Offset + shift;
MhaWrite(info,M,outputFile);
return