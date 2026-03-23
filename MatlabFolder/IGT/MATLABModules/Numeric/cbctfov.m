function mask = cbctfov(info,geoxml)
%% mask = cbctfov(info,geoxml)
% ------------------------------------------
% FILE   : cbctfov.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-01-06  Created.
% ------------------------------------------
% PURPOSE
%   Read in the Varian xml file and the metaimage header MATLAB struct to
%   determin which pixels in the image are inside (which will have a value
%   of one in the mask) and outside (value of zero) the FOV. 
%   The complete FOV is a cylinder with two cones at the top and bottom.
% ------------------------------------------
% INPUT
%   info:       The metaimage info struct. The dimensions specified in the
%               header is in the sequence of [coronal, axial, sagittal]
%   geoxml:     The full path to the general geometry xml file.
% ------------------------------------------
% OUTPUT
%   mask:   The mask which is to be element-by-element multiplied to the
%           3D metaimage.
% ------------------------------------------

%% Reading the xmlfile and calculate necessary geometry factors

fullxml = fileread(geoxml);

% The following are in mm

% The radius in the lateral direction
R = regexp(fullxml,'PatientSizeLat','split');
R = R{2};       R = R(2:end-2);     R = str2double(R)/2;

% The SAD and SID
SAD = regexp(fullxml,'CalibratedSAD','split');
SAD = SAD{2};   SAD = SAD(2:end-2); SAD = str2double(SAD);
SID = regexp(fullxml,'CalibratedSID','split');
SID = SID{2};   SID = SID(2:end-2); SID = str2double(SID);

% Detector SizeY in
LY = regexp(fullxml,'DetectorSizeY','split');
LY = LY{2};     LY = LY(2:end-2);   LY = str2double(LY);

% The vertical length of the FOV
H = LY * SAD/SID;
% The height of the cone
h = H/2 * R/SAD;

%% Assign each pixel in the mask a physical coordinate in mm
% The isocenter is (0,0,0)
dim = info.Dimensions;
offset = info.Offset;
d = info.PixelDimensions;

x = offset(1):d(1):(offset(1)+dim(1)*d(1)-d(1));
y = offset(2):d(2):(offset(2)+dim(2)*d(2)-d(2));
z = offset(3):d(3):(offset(3)+dim(3)*d(3)-d(3));

[xx,zz] = meshgrid(x,z);    xx = xx'; zz = zz';


%% Constructing the mask

mask = zeros(dim(1),dim(3),dim(2));

for ky = 1:dim(2)
    tmpslice = zeros(dim(1),dim(3));
    if ( abs(y(ky)) > (H/2 - h) ) && ( abs(y(ky)) <= H/2 )
        r = (H/2 - abs(y(ky))) * 2*SAD/H;
        tmpslice((xx.^2 + zz.^2) <= r^2) = 1;
    elseif ( abs(y(ky)) <= (H/2 - h) ) && ( abs(y(ky)) <= H/2 )
        tmpslice((xx.^2 + zz.^2) <= R^2) = 1;
    end
    mask(:,:,ky) = tmpslice;
end

mask = logical(permute(mask,[1 3 2]));

return