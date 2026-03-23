function [xs,Ps,xps,Pps,numConds,pnRatios,unstableTags] = kalman2DTo3D(trj2D,G,phase,priorPoints,priorPhase,R,NPast)
%% [xs,Ps,xps,Pps,numConds,pnRatios,unstableTags] = kalman2DTo3D(trj2D,G,phase,priorPoints,priorPhase,R,NPast)
% ------------------------------------------
% FILE   : kalman2DTo3D.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-01-10  Created.
% ------------------------------------------
% PURPOSE
%   Convert 2D trajectory to 3D using the Kalman filter method as described
%   by Shieh et al "A Bayesian method for 3D markerless tumor tracking for
%   lung cancer radiotherapy".
% ------------------------------------------
% INPUT
%   trj2D:      The input 2D trajectory (N by 2). Positive x and y
%               corresponds to the [right,down] direction of the 2D image.
%   G:          The geometry matrices read from a RTK geometry file.
%               3 by 4 by N
%   phase:      The respiratory phase of each frame in trj2D. If left
%               empty ([]), the moving mean prediction will be used.
%   priorPoints:A set of prior points from pre-treatment imaging. This can
%               either be, e.g. 10 points from the pre-treatment 4D CBCT,
%               or 600 points that represent the entire 3D trajectory of
%               the CBCT session. N by 3
%   priorPhase: The respiratory phase of each frame in priorPoints.
%   R:          The 2 by 2 measurement uncertainty matrix.
%               Default is R = 0.388^2*eye(2)
%   NPast:      Number of past data points used for the prediction model.
%               Default is 100.
%
% OUTPUT
%   xs:         A N by 3 vector of the final calculated 3D trajectory.
%   Ps:         A 3 by 3 by N matrix of the calculated covariance of each
%               frame.
%   xps:        A N by 3 vector of the predicted 3D trajectory.
%   Pps:        A 3 by 3 by N matrix of the predicted covariance of each
%               frame.
%   numConds:   The numeric conditions for each frame.
%   pnRatios:   The ratio of which the measurement decreases the
%               uncertainty. Lower == More uncertain with measurement.
%   unstableTags:A vector of boolean specifying whether a frame was
%                numerically unstable.

%% Input check
MAXDEVFROMPREDICTION = 5;
MAXPNRATIO = 0.75;

NFrame = size(trj2D,1);

if size(trj2D,2) ~= 2
    error('ERROR: The size of trj2D must be N by 2.');
end;

if ~isequal(size(G),[3,4,NFrame])
    error('ERROR: The size of G should be 3 by 4 by N');
end

if ~isempty(phase) && length(phase) ~= NFrame
    error('ERROR: The length of phase does not match with the number of rows in trj2D');
end

if ~isempty(phase) && length(priorPhase) ~= size(priorPoints,1)
    error('ERROR: The length of priorPhase does not match with the number of rows in priorPoints');
end

if nargin < 6
    R = 0.388^2 * eye(2);
end

if nargin < 7
    NPast = 100;
end

%% Process prior data
% Is prior points the average 4D CBCT positions or the whole 3D prior trj?
if ~isequal(unique(priorPhase),priorPhase) % average 4D CBCT position
    priorPoints4DAvg = priorPoints;
else % whole trj
    for k = 1:length(priorPhase)
        priorPoints4DAvg(k,:) = ...
            mean(priorPoints(priorPhase == k,:));
    end
end

%% Initialization
pastPoints = priorPoints;
pastPhase = priorPhase;
recentPastPoints = pastPoints(max(1,size(pastPoints,1) - NPast + 1):end,:);
recentPastPhase = pastPhase(max(1,size(pastPoints,1) - NPast + 1):end);

x = pastPoints(end,:)';
if ~isempty(phase)
    phase_prev = pastPhase(end);
end
P = zeros(3,3);
Q = cov(priorPoints);

for k = 1:size(trj2D,1);
    if isempty(phase)
        xp = mean(recentPastPoints,1)';
    else
        %xp = x + priorPoints4DAvg(phase(k),:)' - priorPoints4DAvg(phase_prev,:)';
        %xp = x + mean(pastPoints(pastPhase==phase(k),:),1)' - ...
        %   mean(pastPoints(pastPhase==phase_prev,:),1)';
        if sum(recentPastPhase==phase(k)) == 0
            xp = pastPoints(pastPhase == phase(k),:);
            xp = xp(end,:)';
        else
            xp = mean(recentPastPoints(recentPastPhase==phase(k),:),1)';
        end
        phase_prev = phase(k);
    end
    
    % Jacobian of measurement matrix
    g = G(:,:,k);
    [u2D,l] = rtk3DTo2D(g,xp');
    u = u2D(1); v = u2D(2);
    H(1,:) = g(1,1:3)/l - u/l * g(3,1:3);
    H(2,:) = g(2,1:3)/l - v/l * g(3,1:3);
    
    % Predicted covariance
    Pp = P + Q;
    
    % Combine prediction and measurement
    K = Pp*H' / (H*Pp*H' + R);
    numConds(k) = cond(H*Pp*H' + R);
    x = xp + K * (trj2D(k,:)' - u2D(:));
    P = (eye(3) - K*H) * Pp;
    pnRatios(k) = norm(P) / norm(Pp);
    
    % Avoid instability
    % If deviation from the prediction is too large, use closest point
    % strategy
    unstableTags(k) = (norm(x([1,3])-xp([1,3])) > MAXDEVFROMPREDICTION) ...
        && (pnRatios(k) > MAXPNRATIO);
    if unstableTags(k)
        A = [ trj2D(k,1)*G(3,1,k) - G(1,1,k),   trj2D(k,1)*G(3,2,k) - G(1,2,k),   trj2D(k,1)*G(3,3,k) - G(1,3,k); ...
              trj2D(k,2)*G(3,1,k) - G(2,1,k),   trj2D(k,2)*G(3,2,k) - G(2,2,k),   trj2D(k,2)*G(3,3,k) - G(2,3,k)];
        beta = [G(1,4,k) - trj2D(k,1)*G(3,4,k); G(2,4,k) - trj2D(k,2)*G(3,4,k)];
        LP = [eye(3), A'; A, zeros(2,2)];
        d_aug = LP \ [zeros(3,1);beta - A*xp(:)];
        x = xp + d_aug(1:3);
    end
    
    % Store to output
    xs(k,:) = x(:)';
    xps(k,:) = xp(:)';
    Pps(:,:,k) = Pp;
    Ps(:,:,k) = P;
    
    % Update past history and prediction model
    pastPoints = [pastPoints; x(:)'];
    if ~isempty(phase)
        pastPhase = [pastPhase(:); phase(k)];
    end
    if size(pastPoints,1) > NPast;
        recentPastPoints = pastPoints(end - NPast + 1:end,:);
        if ~isempty(phase)
            recentPastPhase = pastPhase(end - NPast + 1:end,:);
        end
    else
        recentPastPoints = pastPoints;
        if ~isempty(phase)
            recentPastPhase = pastPhase;
        end
    end
    Q = cov(recentPastPoints);
end

end