function centroid = GetMHAImageCentroid(info,M)
%% centroid = GetMHAImageCentroid(info,M)
% ------------------------------------------
% FILE   : GetMHAImageCentroid.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-03-06  Created.
% ------------------------------------------
% PURPOSE
%   Get the centroid of a mha image
% ------------------------------------------
% INPUT
%   info:       mha image header struct.
%   M:          The image matrix.
%
% OUTPUT
%   centroid:   The image centroid (in mha coordinate system)

%%

origin = info.Offset;
dim = info.Dimensions;
spacing = info.PixelDimensions;

if length(dim) == 2
    dim(3) = 1;
    spacing(3) = 1;
    origin(3) = 0;
end

centroid = [0,0,0];
corners = GetBoundingBoxCorners(M~=0);
for dim = 1:3
    corners(1,dim) = max(1,corners(1,dim));
    corners(2,dim) = min(size(M,dim),corners(2,dim));
end
for x = corners(1,1):corners(2,1);
    for y = corners(1,2):corners(2,2);
        for z = corners(1,3):corners(2,3);
            centroid = centroid + single(M(x,y,z)) * [x,y,z];
        end
    end
end
centroid = centroid / ...
    sum(sum(sum(single(M(corners(1,1):corners(2,1),corners(1,2):corners(2,2),corners(1,3):corners(2,3))))));
centroid = (centroid - [1,1,1]) .* spacing + origin;

return;
