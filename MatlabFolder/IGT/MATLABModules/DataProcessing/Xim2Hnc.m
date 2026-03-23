function Xim2Hnc(inputDir,outputDir)
%% Xim2Hnc(inputDir,outputDir)
% ------------------------------------------
% FILE   : Xim2Hnc.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-10-19  Created.
% ------------------------------------------
% PURPOSE
%   Convert Varian Xim files to Hnc files.
% ------------------------------------------
% INPUT
%   inputDir  : The input folder.
%   outputDir : The output folder.
% ------------------------------------------

%% 
if nargin < 1
    inputDir = pwd;
end;

if nargin < 2
    [backDir,dirName,dirName2] = fileparts(inputDir);
    outputDir = fullfile(backDir,[dirName,dirName2,'_Hnc']);
end

if ~exist(outputDir,'dir')
    mkdir(outputDir);
end;

inputFiles = lscell(fullfile(inputDir,'*.xim'));

load hncHeaderTemplate;
iHnc = hncHeaderTemplate;
clear hncHeaderTemplate;
for k = 1:length(inputFiles);
    [~,name] = fileparts(inputFiles{k});
    [iXim,P] = XimRead(inputFiles{k});
    P(P<0) = 0; P(P>65535) = 65535;
    P = uint16(P);
    iHnc.uiSizeX = iXim.image_width;
    iHnc.uiSizeY = iXim.image_height;
    iHnc.dIDUResolutionX = iXim.properties.PixelWidth;
    iHnc.dIDUResolutionY = iXim.properties.PixelHeight;
    iHnc.uiFileLength = 512 + 2 * iHnc.uiSizeX * iHnc.uiSizeY;
    iHnc.uiActualFileLength = iHnc.uiFileLength;
    iHnc.dCTProjectionAngle = mod(-iXim.properties.KVSourceRtn + 180,360);
    iHnc.dCBCTPositiveAngle = mod(iHnc.dCTProjectionAngle + 270, 360);
    HncWrite(iHnc,P,fullfile(outputDir,[name,'.hnc']));
end;
