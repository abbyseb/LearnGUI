function outImg = rotateIECImageAxial(inImg,angle,bgValue)
%% outImg = rotateIECImageAxial(inImg,angle,bgValue)
% ------------------------------------------
% FILE   : rotateIECImageAxial.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-02-13  Created.
% ------------------------------------------
% PURPOSE
%   Axially rotate a 3D image in IEC geometry using the angle specified.
% ------------------------------------------
% INPUT
%   inImg:          The input 3D image in IEC geometry as a MATLAB matrix.
%   angle:          The angle by which the image is to be rotated.
%   bgValue:        The value assigned to the background due to image
%                   cropping in the rotation process. Default is 0.
%
% OUTPUT
%   outImg:         The rotated image.

%% Rotation
outImg = permute(inImg,[1 3 2]);
for p = 1:size(outImg,3);
    outImg(:,:,p) = imrotate(outImg(:,:,p),angle,'bicubic','crop');
end;
outImg = permute(outImg,[1 3 2]);

if nargin >= 3
    outImg(outImg==0) = bgValue;
end

return;