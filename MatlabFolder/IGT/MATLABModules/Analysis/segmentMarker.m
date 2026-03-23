function [markerLoc,segMap] = segmentMarker(img,priorLoc,segPrc,r,nMax)
%% [markerLoc,segMap] = segmentMarker(img,priorLoc,segPrc,r,nMax)
% ------------------------------------------
% FILE   : segmentMarker.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-10-05  Created.
% ------------------------------------------
% PURPOSE
%   Segment and find the centroid of marker.
% ------------------------------------------
% INPUT
%   img:      	The input image array (3D)
%   priorLoc:   Prior knowledge of where the marker might be in terms of
%               pixel index. Should be a 3 by 1 vector.
%   segPrc:     Segmentation percentile level. Default is 80. If segPrc =
%               0, the N-maximum-intensity method will be used
%   r:          If the N-maximum-intensity method is used, r is the
%               width of the window (in number of pixels) within which the 
%               max intensity will be searched.
%               Default: 3
%   nMax:       if the N-maximum-intensity method is used, nMax is the
%               number of highest intensity pixels considered.
%               Default: 5
% OUTPUT
%   markerLoc:  The segmented marker location in terms of pixel index, but
%               are not integers so that it can represent sub-pixel
%               accuracy due to centroid averaging.
%   segMap:     A logical map same size as "img" showing the segmented
%               marker.

%% Input check
if nargin < 2
    error('ERROR: img and priorLoc must be provided.');
end

if length(size(img)) ~= 3
    error('ERROR: img must be a 3D matrix.');
end

if length(priorLoc) ~= 3
    error('ERROR: priorLoc must be a 3 by 1 or 1 by 3 vector.');
end

if nargin < 3
    segPrc = 80;
end
if segPrc < 0 || segPrc > 100
    error('ERROR: segPrc must be between 0 and 100.');
end

if nargin < 4
    r = 3;
end

if nargin < 5
    nMax = 5;
end
if nMax > (r+1)^3
    nMax = round((r+1)^3/2);
end

%% Segment marker
if segPrc > 0
    segMap = img > prctile(img(:),segPrc);
    conn = bwconncomp(segMap);
    mean3DLocs = zeros(length(conn.PixelIdxList),3);
    for k = 1:length(conn.PixelIdxList)
        mean3DLocs(k,:) = mean(get3DIndexFrom1D(conn.PixelIdxList{k}));
    end
    distToPrior = mean3DLocs - ones(size(mean3DLocs,1),1) * priorLoc(:)';
    distToPrior = sqrt(sum((distToPrior.^2)')');
    bestPixel1DInd = conn.PixelIdxList{find(distToPrior == min(distToPrior))};
    
    segMap(:) = false;
    segMap(bestPixel1DInd) = true;
    
    % Weight the location using image intensity to be more accurate
    bestPixel3DInd_weighted = get3DIndexFrom1D(bestPixel1DInd) .* ( img(bestPixel1DInd) * ones(1,3) );
    markerLoc = sum(bestPixel3DInd_weighted) / sum(img(bestPixel1DInd));
else
    for dim = 1:3
        wInd(dim,1) = max(priorLoc(dim) - r, 1);
        wInd(dim,2) = min(priorLoc(dim) + r, size(img,dim));
    end
    w = img(wInd(1,1):wInd(1,2),wInd(2,1):wInd(2,2),wInd(3,1):wInd(3,2));
    [sortedInt,ind] = sort(w(:));
    [indMax3D(:,1),indMax3D(:,2),indMax3D(:,3)] = ind2sub(size(w),ind(end-nMax+1:end));
    bestPixel3DInd_weighted = indMax3D + ones(nMax,1) * (wInd(:,1)' - [1,1,1]);
    bestPixel3DInd_weighted = bestPixel3DInd_weighted .* ( w(ind(end-nMax+1:end)) * ones(1,3) );
    markerLoc = sum(bestPixel3DInd_weighted) / sum(w(ind(end-nMax+1:end)));
    segMap = false(size(img));
    w = false(size(w));
    w(ind(end-nMax+1:end)) = true;
    segMap(wInd(1,1):wInd(1,2),wInd(2,1):wInd(2,2),wInd(3,1):wInd(3,2)) = w;
end

%% Internal functions
    function result = get3DIndexFrom1D(ind)
        result = zeros(length(ind),3);
        [result(:,1),result(:,2),result(:,3)] = ind2sub(size(img),ind);
    end

end