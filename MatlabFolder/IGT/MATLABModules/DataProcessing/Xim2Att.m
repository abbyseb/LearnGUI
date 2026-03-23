function Xim2Att(ximDir,calibDir,outputDir,useLowMem)
%% Xim2Att(ximDir,calibDir,outputDir,useLowMem)
% ------------------------------------------
% FILE   : Xim2Att.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-12-13   Created.
% ------------------------------------------
% PURPOSE
%   Convert XIM to Att projections with calibration if calibDir is
%   provided.
%
% ------------------------------------------
% INPUT
%   ximDir:      The Xim projection directroy.
%   calibDir:    The calibration image directory. If not used, put '';
%   outputDir:   The output directory. Default: "CalibratedAtt"
%   useLowMem:   If memory is an issue, use "true" for this field and the
%                Xim projections will be converted one by one. Default is
%                true.

%% Input check

if nargin < 2
    calibProj = '';
end

topDir = fileparts(ximDir);
if nargin < 3
    outputDir = fullfile(topDir,'CalibratedAtt');
end

if nargin < 4
    useLowMem = true;
end;

%% Read calibration images
if ~isempty(calibDir);
    calibFile = lscell(fullfile(calibDir,'*FilterBowtie.xim'));
    [calibHead,calibImg] = XimRead(calibFile{1});
    calibNorm = single(calibHead.properties.KVNormChamber);
end

%% Read Xim at one go
if ~useLowMem
    tmpFile = [tempname,'.mha'];
    system(['rtkprojections -p "',ximDir,'" -r xim --attenuation off -o "',tmpFile,'"']);
    [~,P] = MhaRead(tmpFile);
    delete(tmpFile);
end

% Xim header
fl = lscell(fullfile(ximDir,'*.xim'));
for k = 1:length(fl);
    ximHead{k} = XimRead(fl{k});
end;

%% Write to Att
if ~exist(outputDir,'dir');
    mkdir(outputDir);
end

load attHeaderTemplate;
info = attHeaderTemplate;
info.uiSizeX = ximHead{1}.image_width;
info.uiSizeY =ximHead{1}.image_height;
info.dIDUResolutionX = 10 * ximHead{1}.properties.PixelWidth;
info.dIDUResolutionY = 10 * ximHead{1}.properties.PixelWidth;
info.uiFileLength = 512 + 4 * info.uiSizeX * info.uiSizeY;
info.uiActualFileLength = info.uiFileLength;

for k = 1:length(fl)
    info.dCTProjectionAngle = mod(-ximHead{k}.properties.KVSourceRtn + 180,360);
    projNorm = single(ximHead{k}.properties.KVNormChamber);
    
    if useLowMem
        tmpFile = [tempname,'.mha'];
        [~,currentFile] = fileparts(fl{k});
        system(['rtkprojections -p "',ximDir,...
            '" -r "',[currentFile,'.xim'],...
            '" --attenuation off -o "',tmpFile,'"']);
        [~,P_this] = MhaRead(tmpFile);
        delete(tmpFile);
        P_this = log(projNorm ./ single(P_this));
    else
        P_this = log(projNorm ./ single(P(:,:,k)));
    end
    
    if ~isempty(calibDir)
        P_this = P_this - log(calibNorm ./ single(calibImg));
    end
    P_this(isnan(P_this)) = 0;
    P_this(isinf(P_this)) = 0;
    P_this(P_this < 0) = 0;
    [~,name] = fileparts(fl{k});
    AttWrite(info, P_this, fullfile(outputDir,[name,'.att']));
end