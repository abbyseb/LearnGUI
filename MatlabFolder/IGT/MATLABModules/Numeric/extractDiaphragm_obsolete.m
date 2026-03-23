function [dImg_full,dImg_top,d_full,d_top,d_full_cell,d_top_cell] = extractDiaphragm_obsolete(img,spacing,softCutoff)
%% [dImg_full,dImg_top,d_full,d_top,d_full_cell,d_top_cell] = extractDiaphragm_obsolete(img,spacing,softCutoff)
% ------------------------------------------
% FILE   : extractDiaphragm.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-02-18  Created.
% ------------------------------------------
% PURPOSE
%   Extract diaphragm from an input 3D image.
% ------------------------------------------
% INPUT
%   img:            The input 3D image matrix in IEC geometry (LR, SI, AP)
%   spacing:        (Optional) The pixel spacing of the 3D image (1 by 3
%                   vector) in mm.
%                   Default: [1,1,1]
%   softCutoff:     (Optional) Soft tissue intensity cutoff value for
%                   segmenting the lungs.
%                   Default: Auto-detection.
% OUTPUT
%   dImg_full:      The full extracted diaphragm image.
%   dImg_top:       The top of the extracted diaphragm image.
%   d_full:         Binary mask of the whole diaphragm.
%   d_top:          Binary mask of the top part of the diaphragm.
%   d_full_cell:    A cell specifying the full mask for each of the 
%                   diaphragms.
%   d_top_cell:     A cell specifying the top mask for each of the 
%                   diaphragms.

%% Hard coded parameters
filtSize = 2; %mm
minVol = 15000; %mm^3
lungOverlap = 1000; % mm^3
topFrac = 1/2;

%% Input check
if nargin < 2
    spacing = [1,1,1];
end

if nargin < 3
    softCutoff = [];
end

%% Extract body and lung as a reference
% First, find the best tissue cutoff value if not input
if isempty(softCutoff)
    cutoffs = linspace(min(img(:)),max(img(:)),50);
    for k = 1:length(cutoffs)
        numOfPix(k) = sum(img(:) > cutoffs(k));
    end
    dN = numOfPix(3:end) - numOfPix(1:end-2);
    dN = [max(abs(dN)),dN,max(abs(dN))];
    % Cutoff that resulted in less than 10% of pixels being soft tissue is
    % very unlikely to be correct
    dN(numOfPix/numel(img) < 0.1) = max(abs(dN));
    % Then within the likely values, pick the one that is most stable to
    % segmentation
    indBestCutoff = round(mean(find(abs(dN) == min(abs(dN)))));
    softCutoff = cutoffs(indBestCutoff);
end

% Segment the body and lungs using the soft tissue cutoff
body = img > softCutoff;
body = findConnected(body);
ext = findConnected(~body);
lungs = findConnected(~body & ~ext);

%% Find diaphragm boundary
% LR,SI,AP gradient
gradX = abs(img(3:end,:,:) - img(1:end-2,:,:));
gradX(2:end+1,:,:) = gradX; gradX(1,:,:) = 0;   gradX(end+1,:,:) = 0;
gradY = img(:,3:end,:) - img(:,1:end-2,:);
gradY(:,2:end+1,:) = gradY; gradY(:,1,:) = 0;   gradY(:,end+1,:) = 0;
gradZ = abs(img(:,:,3:end) - img(:,:,1:end-2));
gradZ(:,:,2:end+1) = gradZ; gradZ(:,:,1) = 0;   gradZ(:,:,end+1) = 0;

% Apply average filter to smooth gradient
for k = 1:size(gradY,2);
    slice = permute(gradX(:,k,:),[1 3 2]);
    gradX(:,k,:) = permute(imfilter(slice,...
        fspecial('average',ceil([filtSize,filtSize] ./ spacing([1,3])))),...
        [1 3 2]);
    slice = permute(gradY(:,k,:),[1 3 2]);
    gradY(:,k,:) = permute(imfilter(slice,...
        fspecial('average',ceil([filtSize,filtSize] ./ spacing([1,3])))),...
        [1 3 2]);
    slice = permute(gradZ(:,k,:),[1 3 2]);
    gradZ(:,k,:) = permute(imfilter(slice,...
        fspecial('average',ceil([filtSize,filtSize] ./ spacing([1,3])))),...
        [1 3 2]);
end

d_full = gradY > sqrt(gradX.^2+gradZ.^2) & ...
    gradY > 0.5*(mean(img(body(:))) - mean(img(lungs(:))));
d_full = findConnected(d_full,round(minVol/prod(spacing)));

% Find connected sections
conSecs = bwconncomp(d_full);
% If no overlap with lungs, then it is not the diaphragm
indNotDiaphragm = [];
for k = 1:length(conSecs.PixelIdxList)
    thisSec = false(size(d_full));
    thisSec(conSecs.PixelIdxList{k}) = true;
    if sum(sum(sum(thisSec & lungs))) < round(lungOverlap/prod(spacing))
        indNotDiaphragm = [indNotDiaphragm,k];
    end
end
conSecs.PixelIdxList = rmcell1D(conSecs.PixelIdxList,indNotDiaphragm);
conSecs.NumObjects = length(conSecs.PixelIdxList);

% Pick the lowest two sections
meanY = zeros(conSecs.NumObjects,1);
for k = 1:conSecs.NumObjects
    meanY(k) = mean(...
        mod(floor((conSecs.PixelIdxList{k} - 1) / size(d_full,1)),size(d_full,2)) + 1);
end
[~,indSortY] = sort(meanY);
d_full = false(size(d_full));
for k = 1:2
    tmp = false(size(d_full));
    tmp(conSecs.PixelIdxList{indSortY(end-k+1)}) = true;
    d_full_cell{k} = tmp;
    for s = 1:size(d_full_cell{k},3)
        if sum(sum(d_full_cell{k}(:,:,s))) < 1; continue; end;
        d_full_cell{k}(:,:,s) = findConnected(d_full_cell{k}(:,:,s));
    end
    d_full_cell{k} = findConnected(d_full_cell{k});
    d_full = d_full | d_full_cell{k};
end

%% Pick only the top part if specified
d_top = false(size(d_full));
for k = 1:length(d_full_cell)
    bw = bwconncomp(d_full_cell{k});
    x = mod((bw.PixelIdxList{1} - 1),size(d_full,1)) + 1;
    y = mod(floor((bw.PixelIdxList{1} - 1) / size(d_full,1)),size(d_full,2)) + 1;
    z = (bw.PixelIdxList{1} - ...
        (y-1) * size(d_full,1) - x) / size(d_full,1) / size(d_full,2) + 1;
    %indTopPoint = find(y == min(y)); indTopPoint = indTopPoint(1);
    %dist = sqrt((x - x(indTopPoint)).^2 + (z - z(indTopPoint)).^2);
    dist = sqrt((x - mean(x)).^2 + (y - min(y)).^2 + (z - mean(z)).^2);
    [~,indTop] = sort(dist);
    mask_this = false(size(d_full));
    mask_this(bw.PixelIdxList{1}(indTop(1:round(end*topFrac)))) = true;
    d_top_cell{k} = mask_this;
    d_top = d_top | d_top_cell{k};
end

%% Diaphragm image
dImg_full = img .* single(d_full);
dImg_top = img .* single(d_top);
end