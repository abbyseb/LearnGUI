function corners = GetBoundingBoxCorners(mask)
%% corners = GetBoundingBoxCorners(mask)
% ------------------------------------------
% FILE   : GetBoundingBoxCorners.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2015-04-07  Created.
% ------------------------------------------
% PURPOSE
%   Get bounding box corners of a binary mask.
% ------------------------------------------
% INPUT
%   mask:       Input binary mask.
%
% OUTPUT
%   result:     The corners of the bounding box.

%%
mask = mask > 0;

is2D = false;
if size(mask,3) == 1
    mask(:,:,2) = mask;
    is2D = true;
end

rp = regionprops(mask);

for k = 1:length(rp)
    bb = rp(k).BoundingBox;
    corners1(k,:) = [bb(2),bb(1),bb(3)];
    corners2(k,:) = corners1(k,:) + [bb(5),bb(4),bb(6)];
end
if length(rp) > 1
    corners(1,:) = min(corners1);
    corners(2,:) = max(corners2);
else
    corners(1,:) = corners1;
    corners(2,:) = corners2;
end
corners = round(corners);

if is2D
    corners(:,3) = [];
end