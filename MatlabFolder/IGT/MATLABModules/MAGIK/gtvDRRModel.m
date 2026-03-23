function gtvModel = gtvDRRModel(exhGTVFile,inhGTVFile,gtvPosCov,exhCTFile,volOffset,geoFile,projSize,projSpacing,outputDir,verbose,ctDRRFile)
%% gtvModel = gtvDRRModel(exhGTVFile,inhGTVFile,gtvPosCov,exhCTFile,volOffset,geoFile,projSize,projSpacing,outputDir,verbose,ctDRRFile)
% ------------------------------------------
% FILE   : gtvDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-06  Created.
% ------------------------------------------
% PURPOSE
%   DRR-based GTV 2D gtvModel.
% ------------------------------------------
% INPUT
%   exhGTVFile:     The end-exhale GTV mask volume file.
%   inhGTVFile:     The end-inhale GTV mask volume file.
%   gtvPosCov:      The 3x3 matrix of GTV position covariance.
%   exhCTFile:      The end-exhale CT file.
%   volOffset:      1x3 vector of the offset needed to shift the GTV
%                   gtvModel and the exhale CT to align with patient position.
%   geoFile:        The RTK geometry file that defines the gtvModel geometry.
%   projSize:       1x2 vector of the projection image dimension (int).
%   projSpacing:    1x2 vector of the projection pixel sizes (double).
%   outputDir:      The program will save the gtvModel as
%                   GTVDRRModel.mat in outputDir.
%   verbose:        Whether or not to display progress.
%                   Default: true
%   ctDRRFile:      (Optional) Default: empty
%                   If the CT DRR file is provided, then the DRR generation
%                   step will be skipped to save time.

% OUTPUT
%   gtvModel:          The gtvModel in a struct. The struct records the geometric
%                   specificis, geometry matrices, and the 2D GTV
%                   points for each angular view.

%% Initialize parameters and output sturct
if nargin < 10
    verbose = true;
end
if nargin < 11
    ctDRRFile = '';
end

% Hard-coded parameters
% Expand all DRR to this size unless the DRR is bigger than this
defModelLength = 60; % mm

gtvModel.SID = ReadTagFromRTKGeometry(geoFile,'SourceToIsocenterDistance');
gtvModel.SDD = ReadTagFromRTKGeometry(geoFile,'SourceToDetectorDistance');
gtvModel.Angles = ReadAnglesFromRTKGeometry(geoFile);
gtvModel.G = ReadRTKGeometryMatrices(geoFile);
gtvModel.VolOffset = volOffset;
gtvModel.ProjSize = projSize;
gtvModel.ProjSpacing = projSpacing;
gtvModel.GTVPosCov = gtvPosCov;
gtvModel.ViewScoreThreshold = 0.5;

defModelPixLength = round(defModelLength * gtvModel.SDD / gtvModel.SID ./ projSpacing);

if ~exist(outputDir,'dir')
    mkdir(outputDir);
end

%% Generating DRR
if verbose
    fprintf('Generating DRR ......');
    tic;
end

% First, we apply volume offset and store GTV positions
[exhGTVVolHeader,exhGTVVol] = MhaRead(exhGTVFile);
[inhGTVVolHeader,inhGTVVol] = MhaRead(inhGTVFile);
exhGTVVolHeader.Offset = exhGTVVolHeader.Offset + volOffset;
inhGTVVolHeader.Offset = inhGTVVolHeader.Offset + volOffset;
gtvModel.GTVExhPos = GetMHAImageCentroid(exhGTVVolHeader,exhGTVVol>0);
gtvModel.GTVInhPos = GetMHAImageCentroid(inhGTVVolHeader,inhGTVVol>0);

% Calculate contour and mask points of the GTV
[contourPts,maskPts,gtvDRR] = volMaskTo2DContour(exhGTVVol,exhGTVVolHeader,geoFile,projSize,projSpacing,false);

% Read the exhale CT as we need to use it later for calculating view score
[ctVolHeader,ctVol] = MhaRead(exhCTFile);

% Then we generate or read the DRR of the exhale CT
if ~isempty(ctDRRFile)
    [ctDRRHeader,ctDRR] = MhaRead(ctDRRFile);
    % We have to shift the DRR based on volOffset
    if any(volOffset ~= 0)
        for k = 1:size(ctDRR,3)
            shift2D = rtk3DTo2D(gtvModel.G(:,:,k),volOffset) - ...
                rtk3DTo2D(gtvModel.G(:,:,k),[0,0,0]);
            shift2D = round(shift2D ./ ctDRRHeader.PixelDimensions(1:2));
            ctDRR(:,:,k) = imtranslate(ctDRR(:,:,k),...
                round([shift2D(2),shift2D(1)]));
        end
    end
else
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
    fprintf('Building the GTV DRR model ......');
    tic;
end

for k = 1:length(gtvModel.Angles)
    % If the GTV DRR model is outside or touching the FOV boundaries, we
    % ignore it for this view
    if isempty(contourPts{k}) || ...
            any(contourPts{k}(:,1) == 1) || any(contourPts{k}(:,1) == gtvModel.ProjSize(1)) || ...
            any(contourPts{k}(:,2) == 1) || any(contourPts{k}(:,2) == gtvModel.ProjSize(2))
        gtvModel.Corners(:,:,k) = [2,2;1,1];
        gtvModel.DRRMin(k,1) = 0;
        gtvModel.DRRMax(k,1) = 0;
        gtvModel.DRRStd(k,1) = 0;
        gtvModel.DRRMean(k,1) = 0;
        gtvModel.DRR{k} = [];
        gtvModel.ContourIdx1D{k} = [];
        gtvModel.ViewScore(k,1) = 0;
        continue;
    end
    corners = [min(contourPts{k}(:,1)),min(contourPts{k}(:,2));...
        max(contourPts{k}(:,1)),max(contourPts{k}(:,2))];
    defCorners = round([mean(corners) - defModelPixLength / 2;...
        mean(corners) + defModelPixLength / 2]);
    corners(1,1) = max(min(corners(1,1),defCorners(1,1)),1);
    corners(1,2) = max(min(corners(1,2),defCorners(1,2)),1);
    corners(2,1) = min(max(corners(2,1),defCorners(2,1)),projSize(1));
    corners(2,2) = min(max(corners(2,2),defCorners(2,2)),projSize(2));
    
    gtvModel.Corners(:,:,k) = corners;
    drrROI = single(ctDRR(corners(1,1):corners(2,1),corners(1,2):corners(2,2),k));
    gtvModel.DRRMin(k,1) = single(min(drrROI(:)));
    gtvModel.DRRMax(k,1) = single(max(drrROI(:)));
    gtvModel.DRRStd(k,1) = single(std(drrROI(:)));
    gtvModel.DRRMean(k,1) = single(mean(drrROI(:)));
    %gtvModel.DRR{k} = (drrROI - gtvModel.DRRMin(k,1)) / (gtvModel.DRRMax(k,1) - gtvModel.DRRMin(k,1));
    gtvModel.DRR{k} = (drrROI - gtvModel.DRRMean(k,1)) / gtvModel.DRRStd(k,1);
    gtvModel.ContourIdx1D{k} = uint32(sub2ind(projSize,contourPts{k}(:,1),contourPts{k}(:,2)));
    
    % Calculating view score by looking at the ratio between tumor's own
    % DRR contribution and the overall DRR intensity
    maskIdx1D = sub2ind(size(drrROI),maskPts{k}(:,1) - corners(1,1) + 1,maskPts{k}(:,2) - corners(1,2) + 1);
    sliceGTV = gtvDRR(corners(1,1):corners(2,1),corners(1,2):corners(2,2),k);
    gtvModel.ViewScore(k,1) = ...
        mean(sliceGTV(maskIdx1D)) * mean(ctVol(exhGTVVol>0)) / mean(drrROI(maskIdx1D));
end
gtvModel.Corners = uint16(gtvModel.Corners);

gtvModel.ViewScore = single(gtvModel.ViewScore / max(gtvModel.ViewScore));

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

%% Saving
if verbose
    fprintf('Saving the model ......');
    tic;
end

save(fullfile(outputDir,'GTVDRRModel.mat'),'gtvModel','-v7.3');

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

end