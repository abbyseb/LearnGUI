function Build4DDVFFrom4DScan(scanList,refScan,outputDir,roiMask,motionMask,useParallel,verbose)
%% Build4DDVFFrom4DScan(scanList,refScan,outputDir,roiMask,motionMask,useParallel,verbose)
% ------------------------------------------
% FILE   : Build4DDVFFrom4DScan.m
% AUTHOR : Andy Shieh, ACRD Image X Institute, The University of Sydney
% DATE   : 2018-01-03  Created.
% ------------------------------------------
% PURPOSE
%   Build 4D-DVF from a 4D scan and save the results in outputDir.
% ------------------------------------------
% INPUT
%   scanList:       The list of 4D scan files in cell format (can be
%                   obtained from "lscell")
%   refScan:        The path to the reference scan for registration.
%   outputDir:      Path to the output directory. If non-existent, a
%                   folder will be created.
%   roiMask:        (Optional) Path to the mask image file for the 
%                   region-of-interest on the reference scan.
%                   If left empty, the roiMask will be calculated
%                   automatically.
%   motionMask:     (Optional) Path to the mask image file for the motion
%                   region of the reference scan. For thoracic scans this 
%                   would be the lung mask.
%                   If left empty, the motionMask will be calculated
%                   automatically.
%   useParallel:    (Optional) Whether or not to use parallel computing.
%                   Default: false
%   verbose:        (Optional) Whether or not to output progress message to
%                   the terminal.
%                   Default: false

%% Input check
if nargin < 4
    roiMask = '';
end

if nargin < 5
    motionMask = '';
end

if nargin < 6
    useParallel = false;
end

if nargin < 7
    verbose = false;
end

if ~exist(outputDir,'dir')
    mkdir(outputDir);
end

%% Find roi and motion mask if needed
[refInfo,refImg] = MhaRead(refScan);
refImg = single(refImg);

maskInfo = refInfo;
maskInfo.DataType = 'uchar';
maskInfo.BitDepth = 8;
outputROIMask = fullfile(outputDir,'ROIMask.mha');
outputMotionMask = fullfile(outputDir,'MotionMask.mha');

if isempty(roiMask) || isempty(motionMask)
    if verbose
        disp(['Computing masks......']);
    end
    
    % First, find soft tissue value
    [N,x] = hist(refImg(:),20);
    [pv,pp] = findpeaks(N);
    softVal = x(pp(1));
    % Use midpoint of air and soft tissue to segment body
    threVal = 0.5 * x(1) + 0.5 * softVal;
    % Use connection and filling to find body and lung regions
    threMaskImg = findConnected(refImg > threVal);
    for k = 1:size(threMaskImg,3)
        roiMaskImg(:,:,k) = imfill(threMaskImg(:,:,k),'holes');
    end
    for k = 1:size(threMaskImg,2)
        roiMaskImg(:,k,:) = permute(imfill(permute(roiMaskImg(:,k,:),[1 3 2]),'holes'),[1 3 2]);
    end
    motionMaskImg = findConnected(roiMaskImg & ~threMaskImg);
    MhaWrite(maskInfo,roiMaskImg,outputROIMask);
    MhaWrite(maskInfo,motionMaskImg,outputMotionMask);
end

clear roiMaskImg; clear motionMaskImg; clear refImg;

if ~isempty(roiMask)
    [~,~] = system(['copy "',roiMask,'" "',outputROIMask,'"']);
end

if ~isempty(motionMask)
    [~,~] = system(['copy "',motionMask,'" "',outputMotionMask,'"']);
end

%% Registration
if verbose
    disp(['Computing registration......']);
end

mkdir(fullfile(outputDir,'Registration'));
if useParallel
    parfor k = 1:length(scanList)
        elastixRegistration(scanList{k},refScan,...
            fullfile(outputDir,'Registration'),...
            which('Elastix_BSpline_Sliding.txt'),...
            outputROIMask,outputROIMask,'',verbose,...
            ['-labels "',outputMotionMask,'"']);
    end
else
    for k = 1:length(scanList)
        elastixRegistration(scanList{k},refScan,...
            fullfile(outputDir,'Registration'),...
            which('Elastix_BSpline_Sliding.txt'),...
            outputROIMask,outputROIMask,'',verbose,...
            ['-labels "',outputMotionMask,'"']);
    end
end

%% Wirte DVFs
disp(['Computing DVFs......']);

mkdir(fullfile(outputDir,'DVF'));
tl = lscell(fullfile(outputDir,'Registration','*.txt'));
if useParallel
    parfor k = 1:length(tl)
        elastixTransform(tl{k},'',verbose,fullfile(outputDir,'DVF',num2str(k+1,'DVF_%02d.mha')));
    end
else
    for k = 1:length(tl)
        elastixTransform(tl{k},'',verbose,fullfile(outputDir,'DVF',num2str(k+1,'DVF_%02d.mha')));
    end
end

end