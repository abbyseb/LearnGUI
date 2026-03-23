function [dSI,dAP,dRot,rotCtrL,rotCtrR,newMask] = dph3DRigidReg(dphL,dphR,img,header,rSI,rAP,rRot)
%% [dSI,dAP,dRot,rotCtrL,rotCtrR,newMask] = dph3DRigidReg(dphL,dphR,img,header,rSI,rAP,rRot)
% ------------------------------------------
% FILE   : dph3DRigidReg.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-08-13 Created.
% ------------------------------------------
% PURPOSE
%  Calculate SI shift, AP shift, and rotatoin (around the LR axis) of the
%  diaphragm model to match with the input image. The rotation center is at
%  the anterior tip of the diaphragm, and is applied before SI and AP
%  translations. Positive angle corresponds to the posterior part of the
%  diaphragm rotating into the inferior direction.
% ------------------------------------------
% INPUT
%   dphL:           The binary mask the left 3D diaphragm model.
%   dphR:           The binary mask the right 3D diaphragm model.
%   img:            The 3D image to be matched to. Must have the same
%                   dimension and occupy the same physical space as the
%                   dphMask.
%   header:         MHA header struct of either dphMask or img.
%   rSI:            (Optional) Search vector for SI shift (mm). Default: -10:10 mm
%   rAP:            (Optional) Search vector for AP shift (mm). Default: -10:10 mm
%   rRot:           (Optional) Search vector for rotation (deg). Default: 0 deg
%                   Set rRot = 0 to exclude rotation in the model.
% OUTPUT
%   dSI:            SI shift in mm.
%   dAP:            AP shift in mm.
%   dRot:           Rotation in degrees.
%   rotCtrL:        The SI and AP index of the rotation center of the left
%                   diaphragm.
%   rotCtrR:        The SI and AP index of the rotation center of the right
%                   diaphragm.
%   newMask:        The translated and rotated (combined) diaphragm mask.

%% Input check and hard-coded paramters
if ~isequal(size(dphL),size(img)) || ~isequal(size(dphR),size(img))
    error('ERROR: dphL, dphR, and img must have the same dimensions.');
end

if nargin < 5
    rSI = -10:10;
end

if nargin < 6
    rAP = -10:10;
end

if nargin < 7
    rRot = 0;
end

% Margins used to calculate mean pixel intensity above and below the
% diaphragm (5 mm)
w = round(5 / header.PixelDimensions(2));

%% Fitting the diaphragm
% Convert search vector to pixel index
rpixSI = unique(round(rSI / header.PixelDimensions(2)));
rpixAP = unique(round(rAP / header.PixelDimensions(3)));

% Pixel index list of the diaphragm mask
[dphLIdx(:,1),dphLIdx(:,2),dphLIdx(:,3)] = ind2sub(size(dphL),find(dphL));
[dphRIdx(:,1),dphRIdx(:,2),dphRIdx(:,3)] = ind2sub(size(dphR),find(dphR));
% The most anterior point, and we record its SI and AP pixel index here
idxRotCtrL = find(dphLIdx(:,3) == max(dphLIdx(:,3)));
rotCtrL = [round(mean(dphLIdx(idxRotCtrL,2))), round(mean(dphLIdx(idxRotCtrL,3)))];
idxRotCtrR = find(dphRIdx(:,3) == max(dphRIdx(:,3)));
rotCtrR = [round(mean(dphRIdx(idxRotCtrR,2))), round(mean(dphRIdx(idxRotCtrR,3)))];

% Precalculate the w-pixel mean image
imgAvgAbv = zeros(size(img));
imgAvgBlw = zeros(size(img));
for kw = 1:w
    imgAvgAbv(:,w:end,:) = imgAvgAbv(:,w:end,:) + img(:,kw:end-w+kw,:) / w;
    imgAvgBlw(:,1:end-w+1,:) = imgAvgBlw(:,1:end-w+1,:) + img(:,kw:end-w+kw,:) / w;
    % Boundary pixels
    if kw < w
        for kb = 1:kw
            imgAvgAbv(:,kw,:) = imgAvgAbv(:,kw,:) + img(:,kw-kb+1,:) / kw;
            imgAvgBlw(:,end-kw+1,:) = imgAvgAbv(:,end-kw+1,:) + img(:,end-kw+kb,:) / kw;
        end
    end
end

% Loop through search vectors
metricVals = zeros([length(rRot),length(rpixSI),length(rpixAP)]);
% Vector from the most anterior point to each point in the model
apVecL = dphLIdx(:,3) - rotCtrL(2);
siVecL = dphLIdx(:,2) - rotCtrL(1);
apVecR = dphRIdx(:,3) - rotCtrR(2);
siVecR = dphRIdx(:,2) - rotCtrR(1);
for kRot = 1:length(rRot)
    for kSI = 1:length(rpixSI)
        for kAP = 1:length(rpixAP)
            xL = dphLIdx(:,1);
            yL = round(rotCtrL(1) + (siVecL * cosd(rRot(kRot)) - apVecL * sind(rRot(kRot))) + rpixSI(kSI));
            zL = round(rotCtrL(2) + (siVecL * sind(rRot(kRot)) + apVecL * cosd(rRot(kRot))) + rpixAP(kAP));
            xR = dphRIdx(:,1);
            yR = round(rotCtrR(1) + (siVecR * cosd(rRot(kRot)) - apVecR * sind(rRot(kRot))) + rpixSI(kSI));
            zR = round(rotCtrR(2) + (siVecR * sind(rRot(kRot)) + apVecR * cosd(rRot(kRot))) + rpixAP(kAP));
            % Combined index
            x = [xL;xR];
            y = [yL;yR];
            z = [zL;zR];
            % y > 1 instead of >=1 because we need at least a pixel above
            % to calculate difference
            idxValid = x >= 1 & x <= size(img,1) & y > 1 & y <= size(img,2) & z >= 1 & z <= size(img,3);
            x = x(idxValid);
            y = y(idxValid);
            z = z(idxValid);
            idxBlw = sub2ind(size(img),x,y,z);
            idxAbv = sub2ind(size(img),x,y-1,z);
            metricVals(kRot,kSI,kAP) = mean(imgAvgBlw(idxBlw) - imgAvgAbv(idxAbv));
        end
    end
end
[idxBest(:,1),idxBest(:,2),idxBest(:,3)] = ind2sub(size(metricVals),find(metricVals == max(metricVals(:))));

dRot = mean(rRot(idxBest(:,1)));
dSI = mean(rpixSI(idxBest(:,2))) * header.PixelDimensions(2);
dAP = mean(rpixAP(idxBest(:,3))) * header.PixelDimensions(3);

% Generate the new mask
xL = dphLIdx(:,1);
yL = round(rotCtrL(1) + (siVecL * cosd(dRot) - apVecL * sind(dRot)) + dSI / header.PixelDimensions(2));
zL = round(rotCtrL(2) + (siVecL * sind(dRot) + apVecL * cosd(dRot)) + dAP / header.PixelDimensions(3));
xR = dphRIdx(:,1);
yR = round(rotCtrR(1) + (siVecR * cosd(dRot) - apVecR * sind(dRot)) + dSI / header.PixelDimensions(2));
zR = round(rotCtrR(2) + (siVecR * sind(dRot) + apVecR * cosd(dRot)) + dAP / header.PixelDimensions(3));
% Combined index
x = [xL;xR];
y = [yL;yR];
z = [zL;zR];
% y > 1 instead of >=1 because we need at least a pixel above
% to calculate difference
idxValid = x >= 1 & x <= size(img,1) & y > 1 & y <= size(img,2) & z >= 1 & z <= size(img,3);
x = x(idxValid);
y = y(idxValid);
z = z(idxValid);
idxNew = sub2ind(size(dphL),x,y,z);

newMask = false(size(dphL));
newMask(idxNew) = true;

% Output the real physical coordinate for the rotation center
rotCtrL = (rotCtrL - 1) .* header.PixelDimensions(2:3) + header.Offset(2:3);
rotCtrR = (rotCtrR - 1) .* header.PixelDimensions(2:3) + header.Offset(2:3);

% Save output - NH 16/10/2019
save('dph3DRigidReg','dSI','dAP','dRot','rotCtrL','rotCtrR')
