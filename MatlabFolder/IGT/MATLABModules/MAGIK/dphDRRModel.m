function dphModel = dphDRRModel(dphRFile,dphLFile,exhCTFile,volOffset,pcVec,geoFile,projSize,projSpacing,outputDir,verbose,ctDRRFile)
%% dphModel = dphDRRModel(dphRFile,dphLFile,exhCTFile,volOffset,pcVec,geoFile,projSize,projSpacing,outputDir,verbose,ctDRRFile)
% ------------------------------------------
% FILE   : dphDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-02-27  Created.
% ------------------------------------------
% PURPOSE
%   DRR-based diaphragm 2D dphModel.
% ------------------------------------------
% INPUT
%   dphRFile:       MHA file of the 3D dphModel of the right diaphragm
%                   segmented from the end exhale CT.
%   dphLFile:       MHA file of the 3D dphModel of the left diaphragm
%                   segmented from the end exhale CT.
%   exhCTFile:      The end exhale CT file.
%   pcVec:          A 2x3 matrix, with the first row being the dLR, dSI and 
%                   dAP principal component of the right diaphragm, and the
%                   second row corresponding to the second diaphragm.
%   volOffset:      1x3 vector of the offset needed to shift the diaphragm
%                   dphModel and the exhale CT to align with patient position.
%   geoFile:        The RTK geometry file that defines the dphModel geometry.
%   projSize:       1x2 vector of the projection image dimension (int).
%   projSpacing:    1x2 vector of the projection pixel sizes (double).
%   outputDir:      The program will save the dphModel as
%                   DphDRRModel.mat in outputDir.
%   verbose:        Whether or not to display progress.
%                   Default: true
%   ctDRRFile:      (Optional) Default: empty
%                   If the CT DRR file is provided, then the DRR generation
%                   step will be skipped to save time.

% OUTPUT
%   dphModel:          The dphModel in a struct. The struct records the geometric
%                   specificis, geometry matrices, and the 2D diaphragm
%                   points for each angular view.

%% Initialize parameters and output sturct
if nargin < 10
    verbose = true;
end
if nargin < 11
    ctDRRFile = '';
end

% Hard-coded parameters
% Amount to extend the diaphragm DRR in the superior direction (in mm)
siMargin = 10;
% Point object area threshold (mm^2)
areaThreshold = 100;

dphModel.SID = ReadTagFromRTKGeometry(geoFile,'SourceToIsocenterDistance');
dphModel.SDD = ReadTagFromRTKGeometry(geoFile,'SourceToDetectorDistance');
dphModel.Angles = ReadAnglesFromRTKGeometry(geoFile);
dphModel.G = ReadRTKGeometryMatrices(geoFile);
dphModel.VolOffset = volOffset;
dphModel.ProjSize = projSize;
dphModel.ProjSpacing = projSpacing;
dphModel.PCVec = pcVec;

% SI margin in term of pixels
siMarginPix = round(siMargin * dphModel.SDD / dphModel.SID / projSpacing(2));

if ~exist(outputDir,'dir')
    mkdir(outputDir);
end

% We assign 1 to the right diaphragm and 2 to the left.
% This allows us to process the two diaphragms in a loop without repeating
% codes

%% Generating DRR
if verbose
    fprintf('Generating DRR ......');
    tic;
end

% First, we generate DRRs for the diaphragm segmentation
[dphVolHeader{1},dphVol{1}] = MhaRead(dphRFile);
[dphVolHeader{2},dphVol{2}] = MhaRead(dphLFile);
for nside = 1:2
    [dphVolHeader{nside},dphVol{nside}] = cropVolToNonZeroRegion(dphVolHeader{nside},dphVol{nside});
    dphVolHeader{nside}.Offset = dphVolHeader{nside}.Offset + volOffset;
    [dphDRR{nside},dphDRRHeader{nside}] = ...
        rtkForwardProject(dphVol{nside},dphVolHeader{nside},geoFile,projSize,projSpacing);
end

% Then we generate or read the DRR of the exhale CT
if ~isempty(ctDRRFile)
    [ctDRRHeader,ctDRR] = MhaRead(ctDRRFile);
    % We have to shift the DRR based on volOffset
    if any(volOffset ~= 0)
        for k = 1:size(ctDRR,3)
            shift2D = rtk3DTo2D(dphModel.G(:,:,k),volOffset) - ...
                rtk3DTo2D(dphModel.G(:,:,k),[0,0,0]);
            shift2D = round(shift2D ./ ctDRRHeader.PixelDimensions(1:2));
            ctDRR(:,:,k) = imtranslate(ctDRR(:,:,k),...
                round([shift2D(2),shift2D(1)]));
        end
    end
else
    [ctVolHeader,ctVol] = MhaRead(exhCTFile);
    ctVolHeader.Offset = ctVolHeader.Offset + volOffset;
    % Make sure intensity values are in attenuation
    ctVol = single(ctVol);
    if min(ctVol(:)) < -500 % in HU
        ctVol = (ctVol + 1000) / 1000 * 0.013;
    end
    if min(ctVol(:)) >= 0 && max(ctVol(:)) > 1000
        ctVol = ctVol / 1000 * 0.013;
    end
    ctVol(ctVol<0) = 0;
    [ctDRR,ctDRRHeader] = ...
        rtkForwardProject(ctVol,ctVolHeader,geoFile,projSize,projSpacing);
    [~,ctFilename] = fileparts(exhCTFile);
    MhaWrite(ctDRRHeader,ctDRR,fullfile(outputDir,['DRR_',ctFilename,'.mha']));
end

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

%% Process DRR into relevant points
if verbose
    fprintf('Building the diaphragm DRR dphModel ......');
    tic;
end

% Identify if the model represents a point object
isPointObject = true;
for k = 1:length(dphModel.Angles)
    isPointObject = isPointObject &&  ...
        sum(sum(dphDRR{1}(:,:,k) > 0)) + sum(sum(dphDRR{2}(:,:,k) > 0)) * projSpacing(1) * projSpacing(2) < areaThreshold;
end

% Convert the diaphragm DRR into point indices and density values
for nside = 1:2
    for k = 1:length(dphModel.Angles)
        slice = dphDRR{nside}(:,:,k);
        sliceBinary = slice > 0;
        % We need to expand the relevant points in the superior direction
        % to cover some contrast
        % Then we only keep 10 mm in the superior direction and 20 mm in
        % the inferior direction
        % However, we do not do this if the model represents a point object
        if (~isPointObject)
            for u = 1:size(sliceBinary,1)
                nonZeroV = find(sliceBinary(u,:));
                if ~isempty(nonZeroV)
                    sliceBinary(u,:) = false;
                    sliceBinary(u,max(1,nonZeroV(1)-siMarginPix):...
                        min(size(sliceBinary,2),nonZeroV(1)+siMarginPix))...
                        = true;
                end
            end
            % We also do expansion in the horizontal dircetion
            sliceBinary = imdilate(sliceBinary,true(2*ceil(siMarginPix/2)+1,1));
        end
        
        % Convert slice to indices
        idx1D{nside}{k} = uint32(find(sliceBinary & ctDRR(:,:,k) > 0));
        [idx2D{nside}{k}(:,1),idx2D{nside}{k}(:,2)] = ...
            ind2sub(size(slice),idx1D{nside}{k});
        idx2D{nside}{k} = uint16(idx2D{nside}{k});
        dstWght{nside}{k} = single(slice(idx1D{nside}{k}));
        maxDst{nside}(k,1) = single(max([dstWght{nside}{k}(:);0]));
        drr{nside}{k} = ctDRR(:,:,k);
        drr{nside}{k} = drr{nside}{k}(idx1D{nside}{k});
        if ~isempty(drr{nside}{k}) && max(drr{nside}{k}) - min(drr{nside}{k}) > 0
            drr{nside}{k} = (drr{nside}{k} - min(drr{nside}{k})) / ...
                (max(drr{nside}{k}) - min(drr{nside}{k}));
        end
        drr{nside}{k} = single(drr{nside}{k});
    end
end

dphModel.Idx2D_R = idx2D{1};
dphModel.Idx1D_R = idx1D{1};
dphModel.DstWght_R = dstWght{1};
dphModel.MaxDst_R = maxDst{1};
dphModel.DRR_R = drr{1};
dphModel.Idx2D_L = idx2D{2};
dphModel.Idx1D_L = idx1D{2};
dphModel.DstWght_L = dstWght{2};
dphModel.MaxDst_L = maxDst{2};
dphModel.DRR_L = drr{2};

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

%% Saving
if verbose
    fprintf('Saving the 2D dphModel ......');
    tic;
end

save(fullfile(outputDir,'DphDRRModel.mat'),'dphModel','-v7.3');

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

end

function [headerNew,imgNew] = cropVolToNonZeroRegion(header,img)
headerNew = header;
if sum(img(:)>0) == 0
    headerNew.Dimensions = [1,1,1];
    imgNew = 0;
    return;
end
c = GetBoundingBoxCorners(img>0);
for dim = 1:3
    c(1,dim) = max(c(1,dim),1);
    c(2,dim) = min(c(2,dim),size(img,dim));
end
imgNew = img(c(1,1):c(2,1),c(1,2):c(2,2),c(1,3):c(2,3));
headerNew.Dimensions = size(imgNew);
headerNew.Offset = header.Offset + (c(1,:) - 1) .* header.PixelDimensions;
end

function writeTempCTFile(inputFile,volOffset,outputFile)
[info,M] = MhaRead(inputFile);
info.Offset = info.Offset + volOffset;
M = single(M);
if min(M(:)) < -500 % in HU
    M = (M + 1000) / 1000 * 0.013;
end
if min(M(:)) >= 0 && max(M(:)) > 1000
    M = M / 1000 * 0.013;
end
info.DataType = 'float';
info.BitDepth = 32;
MhaWrite(info,M,outputFile);
end

function writeTempSegmentationFile(inputFile,volOffset,outputFile)
[info,M] = MhaRead(inputFile);
[info,M] = cropVolToNonZeroRegion(info,M);
info.Offset = info.Offset + volOffset;
info.DataType = 'uchar';
info.BitDepth = 8;
M = M > 0;
MhaWrite(info,M,outputFile);
end