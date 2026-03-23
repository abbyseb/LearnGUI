function [xs,meanXs,invCovXs,gaussFun] = gaussianPDF2DTo3D(trj2D,G,trj2DPrior,GPrior,NPast,verbose)
%% [xs,meanXs,invCovXs,gaussFun] = gaussianPDF2DTo3D(trj2D,G,trj2DPrior,GPrior,NPast,verbose)
% ------------------------------------------
% FILE   : gaussian2DTo3D.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-09-18  Created.
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
%               If set to infinity, the Gaussian model will be calculated
%               using all of the points including the prior points, and
%               then backprojecting onto the 3D distribution. This would be
%               equivalent to retrospective tracking.
%   verbose:    Verbosity. Default is off;
%
% OUTPUT
%   xs:         A N by 3 vector of the final calculated 3D trajectory.
%   meanXs:     The means of the gaussian PDF.
%   invCovXs:   The inverse covariances of the gaussian PDF.
%   gaussFun:   The gaussian distribution function handle.

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

%%
xs = zeros(size(trj2D,1),3);
meanXs = zeros(size(trj2D,1),3);
invCovXs = zeros(3,3,size(trj2D,1));

% If NPast == Inf, do retrospective tracking
if isinf(NPast)
    %GAll = GPrior;  GAll(:,:,end+1:end+size(G,3)) = G;
    %[dummyExitFlag, meanX, invCovX]  =  MLEestimateCovMat(GAll,[trj2D;trj2DPrior]);
    [dummyExitFlag, meanX, invCovX]  =  MLEestimateCovMat(G,trj2D);
    
    % Backproject 2D points
    for k = 1:size(trj2D,1)
        u = trj2D(k,1); v = trj2D(k,2);
        [uMean,lMean] = rtk3DTo2D(G(:,:,k),meanX(:)');
        up = uMean(1); vp = uMean(2);
        H(1,:) = G(1,1:3,k)/lMean - up/lMean * G(3,1:3,k);
        H(2,:) = G(2,1:3,k)/lMean - vp/lMean * G(3,1:3,k);
        P = invCovX^(-1);
        K = P*H' / (H*P*H');
        x3D = meanX(:) + K * ([u;v] - uMean(:));
        xs(k,:) = x3D(:)';
        
        meanXs(k,:) = meanX(:)';
        invCovXs(:,:,k) = invCovX;
    end
    gaussFun = @(x,y,z)...
    (2*pi)^(-3/2) * det(P)^(-1/2) * ...
    exp(-0.5 * ([x;y;z] - meanX(:))' * invCovX * ([x;y;z] - meanX(:)));

else
    for k = 1:NFrame
        if verbose
            tic;
            fprintf('Tracking frame#%05d ......',k);
        end
        
        if k >= NPast
            trj2DNow = trj2D(k,:);
            trj2DPast = trj2D(k-NPast+1:k,:);
            GNow = G(:,:,k);
            GPast = G(:,:,k-NPast+1:k,:);
        else
            trj2DNow = trj2D(k,:);
            begInd = max(1,size(trj2DPrior,1)-NPast+k+1);
            trj2DPast = [trj2DPrior(begInd:end,:);trj2D(1:k,:)];
            GNow = G(:,:,k);
            GPast(:,:,1:(size(GPrior,3)-begInd+1)) = GPrior(:,:,begInd:end);
            GPast(:,:,(size(GPrior,3)-begInd+2):(size(GPrior,3)-begInd+k+1)) = G(:,:,1:k);
        end
        
        if k == 1
            [dummyExitFlag, meanX, invCovX]  =  MLEestimateCovMat(GPast,trj2DPast);
        else
            [dummyExitFlag, meanX, invCovX]  =  MLEestimateCovMat(GPast,trj2DPast,meanX,invCovX);
        end

        % Backproject 2D point
        u = trj2DNow(1); v = trj2DNow(2);
        [uMean,lMean] = rtk3DTo2D(GNow,meanX(:)');
        up = uMean(1); vp = uMean(2);
        H(1,:) = GNow(1,1:3)/lMean - up/lMean * GNow(3,1:3);
        H(2,:) = GNow(2,1:3)/lMean - vp/lMean * GNow(3,1:3);
        P = invCovX^(-1);
        K = P*H' / (H*P*H');
        x3D = meanX(:) + K * ([u;v] - uMean(:));
        xs(k,:) = x3D(:)';
        
        meanXs(k,:) = meanX(:)';
        invCovXs(:,:,k) = invCovX;
        if verbose
            fprintf('COMPLETED using %f seconds\n',toc);
        end
        
        if k == 11
            2+2;
        end
        
    end
    
    gaussFun = @(x,y,z)...
        (2*pi)^(-3/2) * det(P)^(-1/2) * ...
        exp(-0.5 * ([x;y;z] - meanX(:))' * invCovX * ([x;y;z] - meanX(:)));
end

end

function [exitflag, MLEmeanPos, MLEinvCovMatrix] = MLEestimateCovMat(G,trj2D,meanPosGuess,invCovMatGuess)

%*************************************************************************
% OUTPUT:
% exitflag:             Status of the MLE conjugate gradient optimization
% MLEmeanPos:           Mean position in millimeter of the 3D Gaussian PDF (IEC coordinates)
% MLEinvCovMatrix:      Inverse covariance matrix describing the 3D Gaussian PDF
%*************************************************************************

if nargin < 3
    meanPosGuess = [0,0,0];
end

if nargin < 4
    invCovMatGuess = [1 0 0; 0 1 0; 0 0 1];
end

viewAngles = squeeze(mod(atand((G(3,1,:)./G(3,3,:))) + 180*(G(3,3,:)<0),360));
sinAngles = sind(viewAngles);
cosAngles = cosd(viewAngles);

SID = -squeeze(G(3,4,:));
SDD = -squeeze(G(2,2,:));

projectedTrack(:,1) = trj2D(:,1).*SID./SDD + squeeze(G(1,4,:))./SDD;
projectedTrack(:,2) = trj2D(:,2).*SID./SDD + squeeze(G(2,4,:))./SDD;

initPositionGuess = meanPosGuess;
[cost1 leastSquarePosition] = FindLeastSquarePos(initPositionGuess,G,trj2D);
[cost2 leastSquarePosition] = FindLeastSquarePos(leastSquarePosition,G,trj2D);
while cost1 > cost2
    cost1 = cost2;
    [cost2 leastSquarePosition] = FindLeastSquarePos(leastSquarePosition,G,trj2D);
end

initGuessInvMat(1,:,:) = inv([1 0 0; 0 1 0; 0 0 1]);
initGuessInvMat(2,:,:) = inv([1 0 0; 0 4 0; 0 0 1]);
initGuessInvMat(3,:,:) = invCovMatGuess;

optimumCost = 100000000000000000;
optimumCostIndex = 0;

for i = 1:3
    [exitflag(i) cost(i) optimizedMeanPosition(i,:) optimizedInvCovMatrix(i,:,:)] = FindMaxLikelihood(leastSquarePosition,squeeze(initGuessInvMat(i,:,:)),projectedTrack,viewAngles,sinAngles,cosAngles,SID,SDD);

    if cost(i) < optimumCost
        optimumCost = cost(i);
        optimumCostIndex = i;
    end
end

MLEmeanPos = optimizedMeanPosition(optimumCostIndex,:);
MLEinvCovMatrix = squeeze(optimizedInvCovMatrix(optimumCostIndex,:,:));
exitflag = exitflag(optimumCostIndex);

end

function [SqSum, LeastSqrPos] = FindLeastSquarePos(initPositionGuess,G,trj2D)
LeastSqrPos = fminsearch(@sumOfSquareDistances,initPositionGuess);
SqSum = sumOfSquareDistances(LeastSqrPos);

% Embedded objective function:
    function [squareSum] = sumOfSquareDistances(assumedMeanPos)
        assumedMeanPosProjection = rtk3DTo2D(G,ones(size(trj2D,1),1)*assumedMeanPos);
        squareSum = sum((assumedMeanPosProjection(:)-trj2D(:)).^2);
    end
end

function [exitflag,minusLogLike,MaxLikeMeanPos,MaxLikeInvCovMatrix] = FindMaxLikelihood(initGuessMeanPos,initGuessInvMat,projectedTrack,viewAngles,sinAngles,cosAngles,SID,SDD)

initCovPosArrayGuess = [initGuessMeanPos(1) initGuessMeanPos(2)  initGuessMeanPos(3) initGuessInvMat(1,1) initGuessInvMat(2,2) initGuessInvMat(3,3) 2*initGuessInvMat(2,3)];
initCovPosArrayGuess = [initCovPosArrayGuess 2*initGuessInvMat(1,2) 2*initGuessInvMat(1,3)];

options = optimset('Algorithm','active-set');
options = optimset(options,'GradObj','on'); % Utilize gradient of objective function
options = optimset(options,'MaxFunEvals',100000,'MaxIter',20000,'TolX',1e-18);

options = optimset(options,'GradConstr','on','Display','off');

% Sometimes fmincon leads to an NaN/inf error. The following tries to catch
% this situation.

try
    %[MaxLikeCovPosArray,minusLogLike,exitflag] = fmincon(@minusLogLikelihoodWithPosOptimization,initCovPosArrayGuess,[],[],[],[],[],[],@constraints_of_summedLogProbProjection,options);
    [MaxLikeCovPosArray,minusLogLike,exitflag] = fmincon(@minusLogLikelihoodSum,initCovPosArrayGuess,[],[],[],[],[],[],@constraints_of_summedLogProbProjection,options);
catch ME
    try   %Try again with no use of the gradient of the constraint
        options = optimset(options,'GradConstr','off');
        [MaxLikeCovPosArray,minusLogLike,exitflag] = fmincon(@minusLogLikelihoodSum,initCovPosArrayGuess,[],[],[],[],[],[],@constraints_of_summedLogProbProjection,options);
    catch ME  %Try again with no use of the gradient of the objective function
        options = optimset(options,'GradObj','off');
        [MaxLikeCovPosArray,minusLogLike,exitflag] = fmincon(@minusLogLikelihoodSum,initCovPosArrayGuess,[],[],[],[],[],[],@constraints_of_summedLogProbProjection,options);
    end
end

MaxLikeMeanPos(1) = MaxLikeCovPosArray(1);
MaxLikeMeanPos(2) = MaxLikeCovPosArray(2);
MaxLikeMeanPos(3) = MaxLikeCovPosArray(3);

    % Optimize all six independent covariance matrix elements
    MaxLikeInvCovMatrix = [MaxLikeCovPosArray(4)    MaxLikeCovPosArray(8)/2  MaxLikeCovPosArray(9)/2
        MaxLikeCovPosArray(8)/2  MaxLikeCovPosArray(5)    MaxLikeCovPosArray(7)/2
        MaxLikeCovPosArray(9)/2  MaxLikeCovPosArray(7)/2  MaxLikeCovPosArray(6)];

%embedded objective function:
    function [minusLogLike  grad_minusLogLike] = minusLogLikelihoodSum(CovPosArray)
        % This function calculates minus the log of the product of the probabilities to
        % make the actual observation of (Xp,Yp)s as function of the covariance
        % matrix. (minus is for minimization problem solving).
        
        Xmean = CovPosArray(1);
        Ymean = CovPosArray(2);
        Zmean = CovPosArray(3);
        
        
        A = CovPosArray(4); %for A,B,C,D,E,F: see Appendix of Poulsen et al, PMB 2008.
        B = CovPosArray(5);
        C = CovPosArray(6);
        F = CovPosArray(7);
        D = CovPosArray(8);
        E = CovPosArray(9);
        
        Fx = SID.*sinAngles - projectedTrack(:,1).*cosAngles;
        Fy = projectedTrack(:,2);
        Fz = SID.*cosAngles + projectedTrack(:,1).*sinAngles;
        
        %vector from image point (with SDD = SID = 1000 mm) to Gaussian mean position:
        Mx = -(projectedTrack(:,1).*cosAngles - Xmean*ones(size(projectedTrack(:,1),1),1));
        My = -(projectedTrack(:,2) - Ymean*ones(size(projectedTrack(:,1),1),1));
        Mz = -(-projectedTrack(:,1).*sinAngles - Zmean*ones(size(projectedTrack(:,1),1),1));
        
        invCovMatrix = [A  D/2   E/2; D/2  B  F/2; E/2  F/2  C];
        determinantB = det(invCovMatrix);
        
        minusLogLike = -summedLogProbProjection(A,B,C,D,E,F,Xmean,Ymean,Zmean,Fx,Fy,Fz,Mx,My,Mz,invCovMatrix,determinantB);
        temp_grad_minusLogLike = gradient_of_summedLogProbProjection(A,B,C,D,E,F,Xmean,Ymean,Zmean,Fx,Fy,Fz,Mx,My,Mz,invCovMatrix,determinantB);
        grad_minusLogLike = temp_grad_minusLogLike;
    end

    function [summedLogP] = summedLogProbProjection(A,B,C,D,E,F,Xmean,Ymean,Zmean,Fx,Fy,Fz,Mx,My,Mz,invCovMatrix,determinantB)
        
        % Returns the probability of having the projection (Xp,Yp) given the target
        % distribution is Gaussian and described by coefficients A,B,C,D,E,F
        % The calculations follow the formula in the paper manuscript
        % *********************************************************************** %
        invCovMatrix = [A  D/2   E/2; D/2  B  F/2; E/2  F/2  C];
        determinantB = det(invCovMatrix);
        
        % Reshaped if in order to save time:
        if A <= 0   % Invalid CovMatrix
            summedLogP = -100000000000000000000;      % dummy value in order not to go further with this calculation
        else
            if determinantB <= 0  % Invalid CovMatrix
                summedLogP = -100000000000000000000;
            else
                if det(invCovMatrix(1:2,1:2)) <= 0  %if A*B <= D^2/4   % Invalid CovMatrix if det(invCovMatrix(1:2,1:2)) <= 0
                    summedLogP = -100000000000000000000;
                else  % valid CovMatrix
                    sqrtDeterminantB = sqrt(determinantB);
                    
                    sigma = SID./sqrt(A*Fx.^2 + B*Fy.^2 +C*Fz.^2 + D*Fx.*Fy + E*Fx.*Fz + F*Fz.*Fy);
                    
                    myOVERsigma = sigma .* (A*Fx.*Mx + B*Fy.*My  + C*Fz.*Mz + D/2*(Fx.*My + Fy.*Mx) + E/2*(Fx.*Mz + Fz.*Mx) + F/2*(Fz.*My  + Fy.*Mz))./SID;
                    logP = (log(sqrtDeterminantB*sigma/(2*pi).*sqrt(projectedTrack(:,1).^2 + projectedTrack(:,2).^2 + SID.^2)./SID) - 0.5*((A*Mx + D*My + E*Mz).*Mx + (B*My + F*Mz).*My + C*Mz.^2 - myOVERsigma.^2));
                    
                    summedLogP = sum(logP);
                end
            end
        end
    end

    function [grad_summedLogP] = gradient_of_summedLogProbProjection(A,B,C,D,E,F,Xmean,Ymean,Zmean,Fx,Fy,Fz,Mx,My,Mz,invCovMatrix,determinantB)
        % Calculated the gradient of the objective function
        % Adapted from the C# code for real-time DMLC tracking
        % *********************************************************************** %
        
        sigmaOverSIDsquared = 1./(A*Fx.^2 + B*Fy.^2 +C*Fz.^2 + D*Fx.*Fy + E*Fx.*Fz + F*Fz.*Fy);
        myOverSID = sigmaOverSIDsquared .* (A*Fx.*Mx + B*Fy.*My  + C*Fz.*Mz + D/2*(Fx.*My + Fy.*Mx) + E/2*(Fx.*Mz + Fz.*Mx) + F/2*(Fz.*My  + Fy.*Mz));
        myOverSIDsquared = myOverSID.^2;
        
        derivXmean =  A * Mx + 0.5 * (D * My + E * Mz - myOverSID .* (2 * A * Fx + D * Fy + E * Fz));
        derivYmean =  B * My + 0.5 * (D * Mx + F * Mz - myOverSID .* (2 * B * Fy + D * Fx + F * Fz));
        derivZmean =  C * Mz + 0.5 * (E * Mx + F * My - myOverSID .* (2 * C * Fz + E * Fx + F * Fy));
        derivA =  0.5 * ((F * F / 4 - B * C) / determinantB + Mx .* Mx + (sigmaOverSIDsquared + myOverSIDsquared) .* Fx .* Fx) - myOverSID .* Fx .* Mx;
        derivB =  0.5 * ((E * E / 4 - A * C) / determinantB + My .* My + (sigmaOverSIDsquared + myOverSIDsquared) .* Fy .* Fy) - myOverSID .* Fy .* My;
        derivC =  0.5 * ((D * D / 4 - A * B) / determinantB + Mz .* Mz + (sigmaOverSIDsquared + myOverSIDsquared) .* Fz .* Fz) - myOverSID .* Fz .* Mz;
        derivD =  0.5 * ((D * C / 2 - E * F / 4) / determinantB + Mx .* My + (sigmaOverSIDsquared + myOverSIDsquared) .* Fx .* Fy - myOverSID .* (Fx .* My + Fy .* Mx));
        derivE =  0.5 * ((B * E / 2 - D * F / 4) / determinantB + Mx .* Mz + (sigmaOverSIDsquared + myOverSIDsquared) .* Fx .* Fz - myOverSID .* (Fx .* Mz + Fz .* Mx));
        derivF =  0.5 * ((A * F / 2 - D * E / 4) / determinantB + My .* Mz + (sigmaOverSIDsquared + myOverSIDsquared) .* Fy .* Fz - myOverSID .* (Fy .* Mz + Fz .* My));
        
        
        grad_summedLogP(1) = sum(derivXmean);
        grad_summedLogP(2) = sum(derivYmean);
        grad_summedLogP(3) = sum(derivZmean);
        grad_summedLogP(4) = sum(derivA);
        grad_summedLogP(5) = sum(derivB);
        grad_summedLogP(6) = sum(derivC);
        grad_summedLogP(7) = sum(derivF);
        grad_summedLogP(8) = sum(derivD);
        grad_summedLogP(9) = sum(derivE);
    end

%embedded constraint function:
    function [c ceq DC DCeq] = constraints_of_summedLogProbProjection(CovPosArray)
        % For constrainted optimization.
        % The inverse cov-matrix must be positive infinite
        A = CovPosArray(4);
        B = CovPosArray(5);
        C = CovPosArray(6);
        F = CovPosArray(7);
        D = CovPosArray(8);
        E = CovPosArray(9);
        invCovMatrix = [A  D/2   E/2; D/2  B  F/2; E/2  F/2  C];
        
        c(1) = - A;
        c(2) = D*D/4 - A*B;
        c(3) = -det(invCovMatrix);
        ceq = [];
        
        
        
        % Gradient of the constraints
        if nargout > 2
            DC = [ 0,  0,         0;
                0,  0,         0;
                -1, -B, F*F/4-B*C;
                0, -A, E*E/4-A*C;
                0,  0, D*D/4-A*B;
                0,  0, A*F/2-D*E/4;
                0,D/2, D*C/2-E*F/4;
                0,  0, B*E/2-D*F/4;
                0,  0,         0];
            DCeq = [];
        end
        
    end

end