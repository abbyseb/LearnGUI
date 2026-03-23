function ProjBin2Att(inputFile,outputFile,projSize,projSpacing,ang)
%% ProjBin2Att(inputFile,outputFile,projSize,projSpacing,ang)
% ------------------------------------------
% FILE   : ProjBin2Att.m
% AUTHOR : Andy Shieh, ACRF ImageX Institute, University of Sydney
% ------------------------------------------
% PURPOSE
%   Convert a projection file in binary format into .att file, which
%   contains both header and image body.
% ------------------------------------------
% INPUT
%   intputFile:    The input binary file.
%   outputFile:    The output path for the att file.
%   projSize:      The projection dimensions. e.g. [1024,768]
%   projSpacing:   The projection spacing. e.g. [0.388,0.388]
%   ang:           The projection angle value.
% ------------------------------------------

%% Prepare header

load(which('attHeaderTemplate.mat'));
info = attHeaderTemplate;
info.uiSizeX = projSize(1);
info.uiSizeY = projSize(2);
info.dIDUResolutionX = projSpacing(1);
info.dIDUResolutionY = projSpacing(2);
info.dCTProjectionAngle = ang;
info.dCBCTPositiveAngle = mod(ang + 270, 360);
info.uiFileLength = 512 + 4 * prod(projSize);
info.uiActualFileLength = info.uiFileLength;

%% Read bin file and write to Att
fid = fopen(inputFile,'r');
P = fread(fid,prod(projSize),'single');
fclose(fid);

P = single(P);
P = reshape(P,projSize);

AttWrite(info,P,outputFile);
return;