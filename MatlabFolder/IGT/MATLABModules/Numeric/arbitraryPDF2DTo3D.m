function xs = arbitraryPDF2DTo3D(trj2D,G,trj2DPrior,GPrior,NPast,p,verbose)
%% xs = arbitraryPDF2DTo3D(trj2D,G,trj2DPrior,GPrior,NPast,p,verbose)
% ------------------------------------------
% FILE   : arbitraryPDF2DTo3D.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-02-16  Created.
% ------------------------------------------
% PURPOSE
%   Convert 2D trajectory to 3D using the arbitrary shape PDF method
%   described in Li 2011 Med Phys "A Bayesian approach to real-time 3D
%   tumor localization via monoscopic x-ray imaging during treatment
%   delivery"
% ------------------------------------------
% INPUT
%   trj2D:      The input 2D trajectory (N by 2). Positive x and y
%               corresponds to the [right,down] direction of the 2D image.
%   G:          The geometry matrices read from a RTK geometry file.
%               3 by 4 by N
%   GPrior:     The geometry matrices for the prior 2D points.
%   trj2DPrior: Prior 2D points (e.g. during CBCT)
%   NPast:      Number of past data points used for the prediction model.
%               Default is 100.
%               If NPast = Inf, tracking will be done in a retrospective
%               manner.
%   p:          This method uses the p-norm of distance.
%               Default: 2 (Gaussian)
%   verbose:    To display progress or not.
%               Default: false
%
% OUTPUT
%   xs:         A N by 3 vector of the final calculated 3D trajectory.

%% Input check
%SearchVector_t = -10:0.1:10;

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
    p = 2;
end

if nargin < 7
    verbose = false;
end

%%

xs = zeros(NFrame,3);
if isinf(NPast)
    trj2DPast = trj2D;
    GPast = G;
    
    % Ax = beta for past frames
    for n = 1:size(trj2DPast,1)
        u = trj2DPast(n,1);    v = trj2DPast(n,2);
        APast{n} = [ u*GPast(3,1,n) - GPast(1,1,n),   u*GPast(3,2,n) - GPast(1,2,n),   u*GPast(3,3,n) - GPast(1,3,n); ...
            v*GPast(3,1,n) - GPast(2,1,n),   v*GPast(3,2,n) - GPast(2,2,n),   v*GPast(3,3,n) - GPast(2,3,n)];
        betaPast{n} = [GPast(1,4,n) - u*GPast(3,4,n); GPast(2,4,n) - v*GPast(3,4,n)];
        %Aw = APast{n} * w;
    end
    
    for k = 1:NFrame
        % Ax = beta for this frame
        trj2DNow = trj2D(k,:);
        GNow = G(:,:,k);
        u = trj2DNow(1);    v = trj2DNow(2);
        A = [ u*GNow(3,1) - GNow(1,1),   u*GNow(3,2) - GNow(1,2),   u*GNow(3,3) - GNow(1,3); ...
            v*GNow(3,1) - GNow(2,1),   v*GNow(3,2) - GNow(2,2),   v*GNow(3,3) - GNow(2,3)];
        beta = [GNow(1,4) - u*GNow(3,4); GNow(2,4) - v*GNow(3,4)];
        % Express x = x0 + tw, where w = A(1,:) X A(2,:), x0 = pinv(A) * beta
        x0 = pinv(A) * beta;
        w = cross(A(1,:)',A(2,:)'); w = w / norm(w);
        
        % Search within the line Ax = beta
        options = optimset('MaxFunEvals',100000,'MaxIter',20000);
        [st_Optimized(1),costV(1)] = fminsearch(@costOnBackprojectedLine,0,options);
        [st_Optimized(2),costV(2)] = fminsearch(@costOnBackprojectedLine,20/norm(w),options);
        [st_Optimized(3),costV(3)] = fminsearch(@costOnBackprojectedLine,-20/norm(w),options);
        bestInd = find(costV == min(costV));
        x3D = x0 + st_Optimized(bestInd(1)) * w;
        xs(k,:) = x3D(:)';
    end
else
    for k = 1:NFrame
        if verbose
            tic;
        end
        if k >= NPast
            trj2DNow = trj2D(k,:);
            trj2DPast = trj2D(k-NPast+1:k-1,:);
            GNow = G(:,:,k);
            GPast = G(:,:,k-NPast+1:k-1,:);
        else
            trj2DNow = trj2D(k,:);
            begInd = max(1,size(trj2DPrior,1)-NPast+k+1);
            trj2DPast = [trj2DPrior(begInd:end,:);trj2D(1:k-1,:)];
            GNow = G(:,:,k);
            GPast(:,:,1:(size(GPrior,3)-begInd+1)) = GPrior(:,:,begInd:end);
            GPast(:,:,(size(GPrior,3)-begInd+2):(size(GPrior,3)-begInd+k)) = G(:,:,1:k-1);
        end
        
        % Ax = beta for this frame
        u = trj2DNow(1);    v = trj2DNow(2);
        A = [ u*GNow(3,1) - GNow(1,1),   u*GNow(3,2) - GNow(1,2),   u*GNow(3,3) - GNow(1,3); ...
            v*GNow(3,1) - GNow(2,1),   v*GNow(3,2) - GNow(2,2),   v*GNow(3,3) - GNow(2,3)];
        beta = [GNow(1,4) - u*GNow(3,4); GNow(2,4) - v*GNow(3,4)];
        % Express x = x0 + tw, where w = A(1,:) X A(2,:), x0 = pinv(A) * beta
        x0 = pinv(A) * beta;
        w = cross(A(1,:)',A(2,:)'); w = w / norm(w);
        
        % Ax = beta for past frames
        for n = 1:size(trj2DPast,1)
            u = trj2DPast(n,1);    v = trj2DPast(n,2);
            APast{n} = [ u*GPast(3,1,n) - GPast(1,1,n),   u*GPast(3,2,n) - GPast(1,2,n),   u*GPast(3,3,n) - GPast(1,3,n); ...
                v*GPast(3,1,n) - GPast(2,1,n),   v*GPast(3,2,n) - GPast(2,2,n),   v*GPast(3,3,n) - GPast(2,3,n)];
            betaPast{n} = [GPast(1,4,n) - u*GPast(3,4,n); GPast(2,4,n) - v*GPast(3,4,n)];
            Aw = APast{n} * w;
        end
        
        % Search within the line Ax = beta
        options = optimset('MaxFunEvals',100000,'MaxIter',20000);
        [st_Optimized(1),costV(1)] = fminsearch(@costOnBackprojectedLine,0,options);
        [st_Optimized(2),costV(2)] = fminsearch(@costOnBackprojectedLine,10/norm(w),options);
        [st_Optimized(3),costV(3)] = fminsearch(@costOnBackprojectedLine,-10/norm(w),options);
        bestInd = find(costV == min(costV));
        x3D = x0 + st_Optimized(bestInd(1)) * w;
        clear APast; clear betaPast;
        
        xs(k,:) = x3D(:)';
        if verbose
            disp([num2str(k,'Frame%05d'),' took ',num2str(toc,'%f seconds')]);
        end
    end
end

%% Sub-functions
    function result = costOnBackprojectedLine(st)
        result = asPDFLH(x0 + st * w);
    end

    function result = asPDFLH(x)
        result = 0;
        for nsum = 1:length(APast)
            if isinf(NPast)
                result = result + ...
                    norm(APast{nsum}*x - betaPast{nsum})^p;
            else
                result = result + ...
                    norm(APast{nsum}*x - betaPast{nsum})^p / (size(trj2DPast,1) - nsum + 1);
            end
        end
    end

    function result = asPDFLHFirstDeriv(x)
        result = 0;
        for nsum = 1:length(APast)
            result = result + ...
                p * Aw' * (APast{nsum}*x - betaPast{nsum}) * norm(APast{nsum}*x - betaPast{nsum})^(p-2) ...
                / (size(trj2DPast,1) - nsum + 1);
        end
    end

    function result = asPDFLHSecondDeriv(x)
        result = 0;
        for nsum = 1:length(APast)
            result = result + ...
                p * norm(APast{nsum}*x - betaPast{nsum})^(p-2) + (Aw)' * (Aw) + ...
                p * (p-2) * (Aw' * (APast{nsum}*x - betaPast{nsum}))^2 * norm(APast{nsum}*x - betaPast{nsum})^(p-4) ...
                / (size(trj2DPast,1) - nsum + 1);
        end
    end

end