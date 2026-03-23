function rotateMHAAxial180(mhaFile,outputFile)
%% rotateMHA180(mhaFile,outputFile)
% ------------------------------------------
% FILE   : rotateMHA180.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-06-25  Created.
% ------------------------------------------
% PURPOSE
%   Rotate MHA image by 180 in the axial plane while preserving the
%   isocenter location.
% ------------------------------------------
% INPUT
%   mhaFile:        The input MHA file.
%   outputFile:     The output file path.

%% 
[info,M] = MhaRead(mhaFile);
M = M(end:-1:1,:,end:-1:1);
info.Offset([1,3]) = -info.Offset([1,3]) - info.Dimensions([1,3]) .* info.PixelDimensions([1,3]);
MhaWrite(info,M,outputFile);