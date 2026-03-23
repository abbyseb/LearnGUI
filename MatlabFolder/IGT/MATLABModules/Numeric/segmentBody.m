function body = segmentBody(img,spacing)
%% body = segmentBody(img,spacing)
% ------------------------------------------
% FILE   : segmentBody.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-03-19  Created.
% ------------------------------------------
% PURPOSE
%   Segment the body from an input 3D image.
% ------------------------------------------
% INPUT
%   img:            The input 3D image matrix in IEC geometry (LR, SI, AP)
%   spacing:        (Optional) The pixel spacing of the 3D image (1 by 3
%                   vector) in mm.
%                   Default: [1,1,1]
%
% OUTPUT
%   body:           The calculated body mask.

%% Input check
if nargin < 2
    spacing = [1,1,1];
end

if nargin < 3
    lungConnect = true;
end

%% Find the best soft tissue threshold intensity value
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

%% Segment the body and lungs using the soft tissue cutoff
body = img > softCutoff;
body = findConnected(body);
ext = findConnected(~body);
end