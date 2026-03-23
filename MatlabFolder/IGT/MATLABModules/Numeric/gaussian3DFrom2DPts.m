function [gausfun,meanPos,covMat,pts3D] = gaussian3DFrom2DPts(pts2D,geoMats,initMean,initCovMat,N)
%% [gausfun,meanPos,covMat,pts3D] = gaussian3DFrom2DPts(pts2D,geoMats,initMean,initCovMat,N)
% ------------------------------------------
% FILE   : gaussian3DFrom2DPts.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-12-02  Created.
% ------------------------------------------
% PURPOSE
%   Construct a 3D gaussian distribution from a set of 2D projected points.
% ------------------------------------------
% INPUT
%   pts2D:      A set of 2D projected points ([x1,y1;x2,y2;...])
%   geoMats:    3 by 4 by N RTK geometry matrices.
%   initMean:   Initial guess of the mean of the gaussian (column vector)
%   initCovMat: Initial guess of the covariance matrix.
%               (Default: eye(3))
%   N:          The maximum number of iterations. Default: until
%               convergence.
% ------------------------------------------
% OUTPUT
%   gausfun:    Handle to the output 3D Gaussian function.
%   meanPos:    The final mean position of the Gaussian.
%   covMat:     The final covariance matrix.
%   pts3D:      The 3D points backprojected onto the PDF.
% ------------------------------------------

%% Input check
COVMAXCOND = 1e3;
MAXDEVFROMMEAN = 10;

if size(geoMats,3) < size(pts2D,1)
    error('ERROR: The number of geometry matrices is smaller than the number of points to be backprojected.');
end

if nargin < 3
    initMean = [0;0;0];
end
if ~isequal(size(initMean),[3,1])
    error('ERROR: initMean must be a 3 by 1 column vector.');
end

if nargin < 4
    initCovMat = eye(3);
end
if ~isequal(size(initCovMat),[3,3])
    error('ERROR: initCovMat must be a 3 by 3 square matrix.');
end

if nargin < 5
    N = Inf;
end

% Initilize output
pts3D = zeros(size(pts2D,1),3);
%% Do an initial backprojection of the points

%optoptions = optimoptions('quadprog','Display','off');

cost_prev = realmax;
cost = realmax * 0.99;
n = 0;
while (cost < cost_prev) && (n < N)
    n = n +1;
    % Update PDF
    if n == 1
        meanPos = initMean;
        covMat = initCovMat;
    else
        meanPos_prev = meanPos;
        covMat_prev = covMat;
        meanPos = mean(pts3D)';
        covMat = cov(pts3D);
    end
    if cond(covMat) > COVMAXCOND
        if n == 1;
            covMat = eye(3);
        else
            break;
        end
    end;

    % Backproject points
    cost_prev = cost;
    cost = 0;
    pts3D_prev = pts3D;
    for k = 1:size(pts2D,1)
        u = pts2D(k,1); v = pts2D(k,2);
        g = geoMats(:,:,k);
        [uMean,lMean] = rtk3DTo2D(g,meanPos');
        up = uMean(1); vp = uMean(2);
        H(1,:) = g(1,1:3)/lMean - up/lMean * g(3,1:3);
        H(2,:) = g(2,1:3)/lMean - vp/lMean * g(3,1:3);
        P = covMat;
        K = P*H' / (H*P*H');
        x3D = meanPos + K * ([u;v] - uMean(:));
        
        if norm(x3D([1,3]) - meanPos([1,3])) > MAXDEVFROMMEAN
            if n == 1
                covMat = eye(3);
                P = covMat;
                K = P*H' / (H*P*H');
                x3D = meanPos + K * ([u;v] - uMean(:));
            else
                break;
            end
        end
        
        pts3D(k,:) = x3D';
        cost = cost + 0.5 * (x3D - meanPos)' * covMat^(-1) * (x3D - meanPos);
    end
end

if cost > cost_prev && ~isinf(N)
    meanPos = meanPos_prev;
    covMat = covMat_prev;
    pts3D = pts3D_prev;
end

gausfun = @(x,y,z)...
    (2*pi)^(-3/2) * det(covMat)^(-1/2) * ...
    exp(-0.5 * ([x;y;z] - meanPos)' * covMat^(-1) * ([x;y;z] - meanPos));
return;