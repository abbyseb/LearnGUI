function WarpAvgCTToCBCT(ctFile,cbctFile,outputDir,ctRoiMask,ctMotionMask,verbose)
%% WarpAvgCTToCBCT(ctFile,cbctFile,outputDir,ctRoiMask,ctMotionMask,verbose)
% ------------------------------------------
% FILE   : WarpAvgCTToCBCT.m
% AUTHOR : Andy Shieh, ACRD Image X Institute, The University of Sydney
% DATE   : 2018-01-04  Created.
% ------------------------------------------
% PURPOSE
%   Align and deform CT to match with CBCT.
% ------------------------------------------
% INPUT
%   ctFile:         Path to the CT image file.
%   cbctFile:       Path to the CBCT image file.
%   outputDir:      Path to the output directory. If non-existent, a folder
%                   will be created.
%   ctRoiMask:      (Optional) Path to the mask image file for the 
%                   region-of-interest of CT.
%                   If left empty, the ctRoiMask will be calculated
%                   automatically.
%   ctMotionMask:   (Optional) Path to the mask image file for the CT
%                   motion region. For thoracic scans this would be the 
%                   lung mask.
%                   If left empty, the ctMotionMask will be calculated
%                   automatically.
%   verbose:        (Optional) Whether or not to output progress message to
%                   the terminal.
%                   Default: false

%% Input check
if nargin < 4
    ctRoiMask = '';
end

if nargin < 5
    ctMotionMask = '';
end

if nargin < 6
    verbose = false;
end

if ~exist(outputDir,'dir')
    mkdir(outputDir);
end

%% Find roi and motion mask if needed for CT
[ctInfo,ctImg] = MhaRead(ctFile);
ctImg = single(ctImg);

maskInfo = ctInfo;
maskInfo.DataType = 'uchar';
maskInfo.BitDepth = 8;
ctOutputROIMask = fullfile(outputDir,'CTROIMask.mha');
ctOutputMotionMask = fullfile(outputDir,'CTMotionMask.mha');

if isempty(ctRoiMask) || isempty(ctMotionMask)
    if verbose
        disp(['Computing CT masks......']);
    end
    
    % First, find soft tissue value
    [N,x] = hist(ctImg(:),20);
    [pv,pp] = findpeaks(N);
    softVal = x(pp(1));
    % Use midpoint of air and soft tissue to segment body
    threVal = 0.5 * x(1) + 0.5 * softVal;
    % Use connection and filling to find body and lung regions
    threMaskImg = findConnected(ctImg > threVal);
    for k = 1:size(threMaskImg,3)
        ctRoiMaskImg(:,:,k) = imfill(threMaskImg(:,:,k),'holes');
    end
    for k = 1:size(threMaskImg,2)
        ctRoiMaskImg(:,k,:) = permute(imfill(permute(ctRoiMaskImg(:,k,:),[1 3 2]),'holes'),[1 3 2]);
    end
    ctMotionMaskImg = findConnected(ctRoiMaskImg & ~threMaskImg);
    MhaWrite(maskInfo,ctRoiMaskImg,ctOutputROIMask);
    MhaWrite(maskInfo,ctMotionMaskImg,ctOutputMotionMask);
end

clear ctRoiMaskImg; clear ctMotionMaskImg; clear ctImg;

if ~isempty(ctRoiMask)
    [~,~] = system(['copy "',ctRoiMask,'" "',ctOutputROIMask,'"']);
end

if ~isempty(ctMotionMask)
    [~,~] = system(['copy "',ctMotionMask,'" "',ctOutputMotionMask,'"']);
end

%% Do the same for CBCT
[cbctInfo,cbctImg] = MhaRead(cbctFile);
cbctImg = single(cbctImg);

maskInfo = cbctInfo;
maskInfo.DataType = 'uchar';
maskInfo.BitDepth = 8;
cbctOutputROIMask = fullfile(outputDir,'CBCTROIMask.mha');
cbctOutputMotionMask = fullfile(outputDir,'CBCTMotionMask.mha');

if verbose
    disp(['Computing CBCT masks......']);
end

% First, find soft tissue value
softVal = prctile(cbctImg(:),90);
% Use midpoint of air and soft tissue to segment body
threVal = 0.5 * 0 + 0.5 * softVal;
% Use connection and filling to find body and lung regions
threMaskImg = findConnected(cbctImg > threVal);
for k = 1:size(threMaskImg,3)
    cbctRoiMaskImg(:,:,k) = imfill(threMaskImg(:,:,k),'holes');
end
for k = 1:size(threMaskImg,2)
    cbctRoiMaskImg(:,k,:) = permute(imfill(permute(cbctRoiMaskImg(:,k,:),[1 3 2]),'holes'),[1 3 2]);
end
cbctMotionMaskImg = findConnected(cbctRoiMaskImg & ~threMaskImg);
MhaWrite(maskInfo,cbctRoiMaskImg,cbctOutputROIMask);
MhaWrite(maskInfo,cbctMotionMaskImg,cbctOutputMotionMask);

clear cbctRoiMaskImg; clear cbctMotionMaskImg; clear cbctImg;

%% Rigid registration
if verbose
    disp(['Computing rigid registration......']);
end

elastixRegistration(ctFile,cbctFile,...
    fullfile(outputDir),...
    which('Elastix_Rigid.txt'),...
    ctOutputROIMask,cbctOutputROIMask,'',verbose);
rigidTFile = lscell(fullfile(outputDir,'*Elastix_Rigid*.txt'));
rigidTFile = rigidTFile{1};

%% Deformable registration
if verbose
    disp(['Computing deformable registration......']);
end

elastixRegistration(ctFile,cbctFile,...
    fullfile(outputDir),...
    which('Elastix_BSpline_Sliding.txt'),...
    ctOutputROIMask,cbctOutputROIMask,rigidTFile,verbose);

end