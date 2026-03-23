function [trj3D,theta] = idc2DTo3D(trj2D,G,ts,trj2D_Prior,G_Prior,ts_Prior,NPast,verbose,globMinCheck)
%% [trj3D,theta] = poly2DTo3D(trj2D,G,trj2D_Prior,G_Prior,NPast,verbose,globMinCheck)
% ------------------------------------------
% FILE   : poly2DTo3D.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-09-19  Created.
% ------------------------------------------
% PURPOSE
%   Convert 2D trajectory to 3D using the IDC method described in the
%   Chung et al 2016 MedPhys paper.
% ------------------------------------------
% INPUT
%   trj2D:      The input 2D trajectory (N by 2). Positive x and y
%               corresponds to the [right,down] direction of the 2D image.
%   G:          The geometry matrices read from a RTK geometry file.
%               3 by 4 by N
%   ts:         Timestamp for trj2D. ts(1) should be larger than
%               ts_Prior(end)
%   trj2D_Prior:The 2D trajectory during the CBCT session as a prior to
%               initaite the model.
%   G_Prior:    The geometry matrices for trj2D_Prior
%   ts_Prior:   Timestamp for trj2D_Prior.
%   NPast:      Number of past data points used to update the model.
%               Default is 100.
%               If NPast = Inf, trackking will be done in a retrospective
%               manner.
%   verbose:    Whether to display images for current progress.
%               Default is false.
%   globMinCheck: Whether to start optimization with an additional initial
%                 guess to increase the chance of landing on the global
%                 minimum. Default is false.
%
% OUTPUT
%   trj3D:      The output 3D trajectory.
%   theta:      The most updated parameters for the IDC model.
%               x = theta(1) * y + theta(2) * y(t-tau) + theta(3)
%               z = theta(4) * y + theta(5) * y(t-tau) + theta(6)

%% Input check
if nargin < 6
    error('ERROR: trj2D, G, ts, trj2D_Prior, G_Prior, ts_Prior must be provided.');
end

if nargin < 7
    NPast = 100;
end

if nargin < 8
    verbose = false;
end

if nargin < 9
    globMinCheck = false;
end;

% Fixed phase shift amount as suggested in paper
tau = 0.6;

%%
theta = zeros(6,1);
theta([1,4]) = 1;

trj3D = zeros(size(trj2D,1),3);

ts_All = [ts_Prior(:);ts(:)];
trj2D_All = [trj2D_Prior; trj2D];
G_All = G_Prior; G_All(:,:,end+1:end+size(G,3)) = G;
SID_All = -squeeze(G_All(3,4,:));
SDD_All = -squeeze(G_All(2,2,:));

if isinf(NPast)
    
    trj2DPast = trj2D;
    tsPast = ts;
    GPast = G;
    SIDPast = -squeeze(GPast(3,4,:));
    SDDPast = -squeeze(GPast(2,2,:));
    yPast = trj2DPast(:,2) .* SIDPast ./ SDDPast;
    yPast_Shift = interp1(ts_All,trj2D_All(:,2).*SID_All./SDD_All,tsPast-tau);
    yPast_Shift(isnan(yPast_Shift)) = yPast(isnan(yPast_Shift));
    trj3D_Est = zeros(length(yPast),3);
    
    options = optimset('MaxFunEvals',100000,'MaxIter',20000);
    [theta,cost] = fminsearch(@leastSquareCost, theta,options);
    if globMinCheck
        [theta_candidate,cost_candidate] = fminsearch(@leastSquareCost, zeros(6,1),options);
        if cost_candidate < cost
            theta = theta_candidate;
        end
    end
    
    for k = 1:size(trj2D,1)
        trj2DNow = trj2D(k,:);
        tsNow = ts(k);
        GNow = G(:,:,k);
        SIDNow = -squeeze(GNow(3,4,:));
        SDDNow = -squeeze(GNow(2,2,:));
        yNow = trj2DNow(:,2) .* SIDNow ./ SDDNow;
        yNow_Shift = interp1(ts_All,trj2D_All(:,2).*SID_All./SDD_All,tsNow-tau);
        yNow_Shift(isnan(yNow_Shift)) = yNow(isnan(yNow_Shift));
        
        trj3D(k,1) = theta(1) * yNow + theta(2) * yNow_Shift + theta(3);
        trj3D(k,2) = yNow;
        trj3D(k,3) = theta(4) * yNow + theta(5) * yNow_Shift + theta(6);
    end
else
    for k = 1:size(trj2D,1)
        if verbose
            tic;
        end
        
        if k >= NPast
            trj2DNow = trj2D(k,:);
            trj2DPast = trj2D(k-NPast+1:k,:);
            tsNow = ts(k);
            tsPast = ts(k-NPast+1:k);
            GNow = G(:,:,k);
            GPast = G(:,:,k-NPast+1:k,:);
            SIDPast = -squeeze(GPast(3,4,:));
            SDDPast = -squeeze(GPast(2,2,:));
            SIDNow = -squeeze(GNow(3,4,:));
            SDDNow = -squeeze(GNow(2,2,:));
            yPast = trj2DPast(:,2) .* SIDPast ./ SDDPast;
            yNow = trj2DNow(:,2) .* SIDNow ./ SDDNow;
            yNow_Shift = interp1(ts_All,trj2D_All(:,2).*SID_All./SDD_All,tsNow-tau);
            yPast_Shift = interp1(ts_All,trj2D_All(:,2).*SID_All./SDD_All,tsPast-tau);
        else
            trj2DNow = trj2D(k,:);
            tsNow = ts(k);
            begInd = max(1,size(trj2D_Prior,1)-NPast+k+1);
            trj2DPast = [trj2D_Prior(begInd:end,:);trj2D(1:k,:)];
            tsPast = [ts_Prior(begInd:end), ts(1:k)];
            GNow = G(:,:,k);
            GPast(:,:,1:(size(G_Prior,3)-begInd+1)) = G_Prior(:,:,begInd:end);
            GPast(:,:,(size(G_Prior,3)-begInd+2):(size(G_Prior,3)-begInd+k+1)) = G(:,:,1:k);
            SIDPast = -squeeze(GPast(3,4,:));
            SDDPast = -squeeze(GPast(2,2,:));
            SIDNow = -squeeze(GNow(3,4,:));
            SDDNow = -squeeze(GNow(2,2,:));
            yPast = trj2DPast(:,2) .* SIDPast ./ SDDPast;
            yNow = trj2DNow(:,2) .* SIDNow ./ SDDNow;
            yNow_Shift = interp1(ts_All,trj2D_All(:,2).*SID_All./SDD_All,tsNow-tau);
            yPast_Shift = interp1(ts_All,trj2D_All(:,2).*SID_All./SDD_All,tsPast-tau);
        end
        yNow_Shift(isnan(yNow_Shift)) = yNow(isnan(yNow_Shift));
        yPast_Shift(isnan(yPast_Shift)) = yPast(isnan(yPast_Shift));
        trj3D_Est = zeros(length(yPast),3);
        
        options = optimset('MaxFunEvals',100000,'MaxIter',20000);
        [theta,cost] = fminsearch(@leastSquareCost, theta,options);
        if globMinCheck
            [theta_candidate,cost_candidate] = fminsearch(@leastSquareCost, zeros(6,1),options);
            if cost_candidate < cost
                theta = theta_candidate;
            end
        end
        
        trj3D(k,1) = theta(1) * yNow + theta(2) * yNow_Shift + theta(3);
        trj3D(k,2) = yNow;
        trj3D(k,3) = theta(4) * yNow + theta(5) * yNow_Shift + theta(6);
        
        if verbose
            disp([num2str(k,'Frame%05d'),' took ',num2str(toc,'%f seconds')]);
        end
    end
end

    function result = leastSquareCost(theta)
        trj3D_Est(:,1) = theta(1) * yPast(:) + theta(2) * yPast_Shift(:) + theta(3);
        trj3D_Est(:,2) = yPast;
        trj3D_Est(:,3) = theta(4) * yPast(:) + theta(5) * yPast_Shift(:) + theta(6);
        trj2D_Est = rtk3DTo2D(GPast,trj3D_Est);
        result = sum((trj2D_Est(:,1) - trj2DPast(:,1)).^2 + (trj2D_Est(:,2) - trj2DPast(:,2)).^2);
    end

end