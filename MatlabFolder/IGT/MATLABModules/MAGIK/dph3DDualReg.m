function [pcVec,valMask] = dph3DDualReg(dphR,dphL,img,header,rLI,rSI,rAP)
%% [pcVec,valMask] = dph3DDualReg(dphR,dphL,img,header,rLI,rSI,rAP)
% ------------------------------------------
% FILE   : dph3DDualReg.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-02 Created.
% ------------------------------------------
% PURPOSE
%  Calculate SI shift and AP shift of the diaphragm model to match with the
%  input image. This is often used to calculate the principal motion vector
%  of the diaphragm by registering the end-exhale model to the end-inhale
%  image. The shifts are calculated separately for the left and right
%  diaphragm.
% ------------------------------------------
% INPUT
%   dphR:           The binary mask the right 3D diaphragm model.
%   dphL:           The binary mask the left 3D diaphragm model.
%   img:            The 3D image to be matched to. Must have the same
%                   dimension and occupy the same physical space as the
%                   dphMask.
%   header:         MHA header struct of either dphMask or img.
%   rLR:            (Optional) Search vector for SI shift (mm). Default: 0 mm
%   rSI:            (Optional) Search vector for SI shift (mm). Default: -10:20 mm
%   rAP:            (Optional) Search vector for AP shift (mm). Default: -10:10 mm
%
% OUTPUT
%   pcVec:          A 2x2 matrix. The first row is the principal motion
%                   vector (SI and AP shift in mm) for the right diaphragm,
%                   and the second row is for the left diaphragm.
%   valMask:        A 3D matrix that can be visualized using imshow3D to
%                   validate that the match was done correctly.

%% Input check and hard-coded paramters
if ~isequal(size(dphL),size(img)) || ~isequal(size(dphR),size(img))
    error('ERROR: dphL, dphR, and img must have the same dimensions.');
end

if nargin < 5
    rLR = 0;
end

if nargin < 6
    rSI = -10:20;
end

if nargin < 7
    rAP = -10:10;
end

% Margins used to calculate mean pixel intensity above and below the
% diaphragm (5 mm)
w = round(5 / header.PixelDimensions(2));

%% Fitting the diaphragm
% Convert search vector to pixel index
rpixLR = unique(round(rLR / header.PixelDimensions(1)));
rpixSI = unique(round(rSI / header.PixelDimensions(2)));
rpixAP = unique(round(rAP / header.PixelDimensions(3)));

% Pixel index list of the diaphragm mask
[dphLIdx(:,1),dphLIdx(:,2),dphLIdx(:,3)] = ind2sub(size(dphL),find(dphL));
[dphRIdx(:,1),dphRIdx(:,2),dphRIdx(:,3)] = ind2sub(size(dphR),find(dphR));

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

% Loop through search vectors. We do this for each side of the diaphragm
metricVals_L = zeros([length(rpixSI),length(rpixAP)]);
metricVals_R = zeros([length(rpixSI),length(rpixAP)]);
for kLR = 1:length(rpixLR)
    for kSI = 1:length(rpixSI)
        for kAP = 1:length(rpixAP)
            xR = dphRIdx(:,1) + rpixLR(kLR);
            yR = dphRIdx(:,2) + rpixSI(kSI);
            zR = dphRIdx(:,3) + rpixAP(kAP);
            xL = dphLIdx(:,1) + rpixLR(kLR);
            yL = dphLIdx(:,2) + rpixSI(kSI);
            zL = dphLIdx(:,3) + rpixAP(kAP);
            % y > 1 instead of >=1 because we need at least a pixel above
            % to calculate difference
            idxValid_R = xR >= 1 & xR <= size(img,1) & yR > 1 & yR <= size(img,2) & zR >= 1 & zR <= size(img,3);
            idxValid_L = xL >= 1 & xL <= size(img,1) & yL > 1 & yL <= size(img,2) & zL >= 1 & zL <= size(img,3);
            xR = xR(idxValid_R);
            yR = yR(idxValid_R);
            zR = zR(idxValid_R);
            xL = xL(idxValid_L);
            yL = yL(idxValid_L);
            zL = zL(idxValid_L);
            
            idxBlw_R = sub2ind(size(img),xR,yR,zR);
            idxAbv_R = sub2ind(size(img),xR,yR-1,zR);
            idxBlw_L = sub2ind(size(img),xL,yL,zL);
            idxAbv_L = sub2ind(size(img),xL,yL-1,zL);
            
            metricVals_R(kLR,kSI,kAP) = mean(imgAvgBlw(idxBlw_R) - imgAvgAbv(idxAbv_R));
            metricVals_L(kLR,kSI,kAP) = mean(imgAvgBlw(idxBlw_L) - imgAvgAbv(idxAbv_L));
        end
    end
end

[idxBest_R(:,1),idxBest_R(:,2),idxBest_R(:,3)] = ...
    ind2sub(size(metricVals_R),find(metricVals_R == max(metricVals_R(:))));
[idxBest_L(:,1),idxBest_L(:,2),idxBest_L(:,3)] = ...
    ind2sub(size(metricVals_L),find(metricVals_L == max(metricVals_L(:))));

pcVec(1,1) = mean(rpixLR(idxBest_R(:,1))) * header.PixelDimensions(1);
pcVec(1,2) = mean(rpixSI(idxBest_R(:,2))) * header.PixelDimensions(2);
pcVec(1,3) = mean(rpixAP(idxBest_R(:,3))) * header.PixelDimensions(3);
pcVec(2,1) = mean(rpixLR(idxBest_L(:,1))) * header.PixelDimensions(1);
pcVec(2,2) = mean(rpixSI(idxBest_L(:,2))) * header.PixelDimensions(2);
pcVec(2,3) = mean(rpixAP(idxBest_L(:,3))) * header.PixelDimensions(3);

% Generate the new mask
xR = dphRIdx(:,1) + pcVec(1,1) / header.PixelDimensions(1);
yR = dphRIdx(:,2) + pcVec(1,2) / header.PixelDimensions(2);
zR = dphRIdx(:,3) + pcVec(1,3) / header.PixelDimensions(3);
xL = dphLIdx(:,1) + pcVec(2,1) / header.PixelDimensions(1);
yL = dphLIdx(:,2) + pcVec(2,2) / header.PixelDimensions(2);
zL = dphLIdx(:,3) + pcVec(2,3) / header.PixelDimensions(3);

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
idxNew = sub2ind(size(img),x,y,z);

valMask = false(size(img));
valMask(idxNew) = true;
