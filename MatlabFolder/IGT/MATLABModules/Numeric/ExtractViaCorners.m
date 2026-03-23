function result = ExtractViaCorners(Img,corners)
%% result =  ExtractViaCorners(Img,corners)
% ------------------------------------------
% FILE   : ExtractViaCorners.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2015-04-07  Created.
% ------------------------------------------
% PURPOSE
%   Extract a subset of an image via corner indices.
% ------------------------------------------
% INPUT
%   Img:        The image to be extracted from.
%   corners:    The corner coordinates.
%
% OUTPUT
%   result:     The extracted sub-image.

%%
if size(corners,2) == 2
    result = Img(corners(1,1):corners(2,1),corners(1,2):corners(2,2));
elseif size(corners,2) == 3
    result = Img(corners(1,1):corners(2,1),corners(1,2):corners(2,2),corners(1,3):corners(2,3));
elseif size(corners,2) == 4
    result = Img(corners(1,1):corners(2,1),corners(1,2):corners(2,2),corners(1,3):corners(2,3),corners(1,4):corners(2,4));
else
    error('ERROR: Unsupported number of dimensions');
end
