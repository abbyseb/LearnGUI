function [f,time] = aatvsd(f0,step,N,segImg,lambda,mask,priorImg,priorMask,priorFactor)
%% [f,time] = aatvsd(f0,step,N,segImg,lambda,mask)
% ------------------------------------------
% FILE   : aatvsd.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-12-16  Created.
%          2014-03-03  Preserve bony anatomy and pulmonary details.
%                      Use the squared of the histogram instead.
%          2014-05-21  Incorporate PICCS.
% ------------------------------------------
% PURPOSE
%   Anatomical-adaptive TV-steepest-descent step.
% ------------------------------------------
% INPUT
%   f0:         The input 3D image.
%   step:       The step size.
%   N:          Number of iterations.
%   segImg:     The segmentation image obtained from the first output of
%               the function "thorSeg".
%   lambda:      A weighting factor for substracting tvgrad(segImg) from
%               tvgrad(f). The higher lambda is, the more impact segImg has
%               on the TV minimization.
%   mask:       (Optional) if the user wants to put a cbct fov mask on the
%               step change.
%   priorImg:   A prior image if the prior image similarity constraint is
%               to be imposed.
%               (Optional) (default: [])
%   priorMask:  A priorMask to determine which part of the image is
%               affected by the prior image similarity.
%               (default: true(size(f0)))
%   priorFactor:The prior image similarity factor.
%               (default: 0.5)
% ------------------------------------------
% OUTPUT
%   f:          The resultant 3D image.
%   time:       The computation time.
% ------------------------------------------

%% Initialization

dim = size(f0);

if ~isscalar(lambda) || ~isnumeric(lambda)
    error('ERROR: "alpha" must be a scalar number.');
end

if ~isequal(dim,size(segImg))
    error('ERROR: "f0" and "segImg" must have the same dimension.');
end

if nargin < 6
    mask = 1;
else
    if ~isequal(dim,size(mask))
        error('ERROR: "f0" and "mask" must have the same dimension.');
    end
end

% Prior image realted parameters
if nargin < 9
    priorFactor = 0.5;
end;

if nargin < 8
    priorMask = 1;
else
    if ~isequal(dim,size(priorMask))
        error('ERROR: "f0" and "priorMask" must have the same dimension.');
    end
    if ~islogical(priorMask)
        error('ERROR: "priorMask" must be a logical map.');
    end
end;

if nargin < 7
    priorImg = [];
else
    if ~isequal(dim,size(priorImg))
        error('ERROR: "f0" and "priorImg" must have the same dimension.');
    end
end


%% Calculate

f = f0;

% TV_grad of the segmentation image
dfs = tvgrad3(segImg);
dfs = dfs .* mask;

tic;

for k=1:N
    
    % Conventional TV step change
    df = tvgrad3(f);
    df = df .* mask;
    
    % AATV step change, including the prior image constraint if used
    normFactor = norm(df(:));
    daatv = (df - lambda * dfs) / normFactor;
    if ~isempty(priorImg)
        dfi = tvgrad3(f-priorImg,priorMask & mask);
        daatv(priorMask & mask) = (1 - priorFactor) * daatv(priorMask & mask)...
            + priorFactor * dfi(priorMask & mask) / normFactor;
    end;
    
    % Apply GSD step
    f = f - step * daatv;

end

time = toc;

return;