function [xs,meanXs,covXs,gaussFun,numConds] = gaussian2DTo3D(trj2D,G,trj2DPrior,GPrior,NPast,method)
%% [xs,meanXs,covXs,gaussFun,numConds] = gaussian2DTo3D(trj2D,G,trj2DPrior,GPrior,NPast,method)
% ------------------------------------------
% FILE   : gaussian2DTo3D.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-02-09  Created.
% ------------------------------------------
% PURPOSE
%   Convert 2D trajectory to 3D using the Gaussian PDF method described in
%   Poulsen 2008 paper.
% ------------------------------------------
% INPUT
%   trj2D:      The input 2D trajectory (N by 2). Positive x and y
%               corresponds to the [right,down] direction of the 2D image.
%   G:          The geometry matrices read from a RTK geometry file.
%               3 by 4 by N
%   trj2DPrior: Prior 2D points (e.g. during CBCT)
%   GPrior:     The geometry matrices for the prior 2D points.
%   NPast:      Number of past data points used to update the PDF.
%               Default is 100.
%   method:     The method to calculate joint probability of Gaussians.
%               Options: 'ClosedForm','KalmanGain','Iterative'(Default)
%
% OUTPUT
%   xs:         A N by 3 vector of the final calculated 3D trajectory.
%   meanXs:     The most updated mean of the gaussian PDF.
%   covX:       The most updated covariance of the gaussian PDF.
%   gaussFun:   The gaussian distribution function handle.
%   numConds:   The numeric condition for each frame.

%% Input check
if size(trj2D,2) ~= 2 || size(trj2DPrior,2) ~= 2
	error('ERROR: trj2D and trj2DPrior must be a N by 2 matrix.');
end

NFrame = size(trj2D,1);
NFramePrior = size(trj2DPrior,1);

if ~isequal(size(G),[3,4,NFrame])
    error('ERROR: The size of G should be 3 by 4 by N');
end

if ~isequal(size(GPrior),[3,4,NFramePrior])
    error('ERROR: The size of GPrior should be 3 by 4 by N');
end

if nargin < 5
    NPast = 200;
end

if nargin < 6
    method = 'Iterative';
end

%% Estimate initial PDF
%optoptions = optimoptions('quadprog','Display','off');
% Get a good PDF first
[~,initMean,initCov,pts3D_prior] = gaussian3DFrom2DPts(trj2DPrior,GPrior);
% Use the latest NPast points to update it again
if size(pts3D_prior,1) > NPast
    [~,initMean,initCov,pts3D_prior] = gaussian3DFrom2DPts(...
        trj2DPrior(end-NPast+1:end,:),GPrior(:,:,end-NPast+1:end));
end

xPast = pts3D_prior;
meanX = initMean;
covX = initCov;

for k = 1:NFrame
    % Backproject 2D point
    u = trj2D(k,1); v = trj2D(k,2);
    if strcmpi(method,'ClosedForm')
        % Express the problem as a quadratic programming problem with
        % cost function being the exponent of the Gaussian, and constraint
        % Ax = beta. x is the 3D point
        A = [ u*G(3,1,k) - G(1,1,k),   u*G(3,2,k) - G(1,2,k),   u*G(3,3,k) - G(1,3,k); ...
            v*G(3,1,k) - G(2,1,k),   v*G(3,2,k) - G(2,2,k),   v*G(3,3,k) - G(2,3,k)];
        beta = [G(1,4,k) - u*G(3,4,k); G(2,4,k) - v*G(3,4,k)];
        % We do (covX + SMALLNUMBER*I)^-1 here to avoid numeric instability
        % The SMALLNUMBER*I basically avoids singularity
        numConds(k) = cond(covX);
        H = covX^(-1);
        % Analytic solution to this is [H,E';E,0]*[x;lambda] = [-c;d]
        LP = [H, A'; A, zeros(2,2)];
        d_aug = LP \ [zeros(3,1);beta - A*meanX(:)];
        d = d_aug(1:3,1);
        %d = quadprog(H,[],[],[],A,beta - A*meanX,[],[],meanX,optoptions);
        x3D = d(:) + meanX(:);
    elseif strcmpi(method,'KalmanGain')
        g = G(:,:,k);
        [uMean,lMean] = rtk3DTo2D(g,meanX');
        up = uMean(1); vp = uMean(2);
        H(1,:) = g(1,1:3)/lMean - up/lMean * g(3,1:3);
        H(2,:) = g(2,1:3)/lMean - vp/lMean * g(3,1:3);
        P = covX;
        K = P*H' / (H*P*H');
        numConds(k) = cond(H*P*H');
        x3D = meanX + K * ([u;v] - uMean(:));
    elseif strcmpi(method,'Iterative')
        if k >= NPast
            trj2DNow = trj2D(k-NPast+1:k,:);
            GNow = G(:,:,k-NPast+1:k);
        else
            begInd = max(1,size(trj2DPrior,1)-NPast+k+1);
            trj2DNow = [trj2DPrior(begInd:end,:);trj2D(1:k,:)];
            GNow(:,:,1:(size(GPrior,3)-begInd+1)) = GPrior(:,:,begInd:end);
            GNow(:,:,(size(GPrior,3)-begInd+2):(size(GPrior,3)-begInd+k+1)) = G(:,:,1:k);
        end
        [~,meanX,covX,xNow] = gaussian3DFrom2DPts(...
            trj2DNow,GNow,meanX,covX,50);
        x3D = xNow(end,:)';
        numConds(k) = cond(covX);
    else
        error('ERROR: Unsupported method option.');
    end
    xs(k,:) = x3D(:)';
    
    xPast = [xPast; x3D(:)'];
    if size(xPast,1) > NPast
        xPast = xPast(end-NPast+1:end,:);
    end
    
    meanXs(k,:) = meanX(:)';
    covXs(:,:,k) = covX;
    
    % Update mean and covariance
    meanX = mean(xPast,1)';
    covX = cov(xPast);
end

    meanXs(end+1,:) = meanX(:)';
    covXs(:,:,end+1) = covX;

gaussFun = @(x,y,z)...
    (2*pi)^(-3/2) * det(covX)^(-1/2) * ...
    exp(-0.5 * ([x;y;z] - meanX)' * covX^(-1) * ([x;y;z] - meanX));
end
