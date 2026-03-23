function [f,time] = pitvsd(f0,step,N,f_prior,alpha,mask)
%% [f,time] = pitvsd(f0,step,N,f_prior,alpha,mask)
% ------------------------------------------
% FILE   : pitvsd.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-03-31  Created.
% ------------------------------------------
% PURPOSE
%   Prior image constrained TV-Steepest-Descent step.
% ------------------------------------------
% INPUT
%   f0:      The input 3D image.
%   step:    The step size.
%   N:       Number of iterations.
%   f_prior: The prior image.
%   alpha:   The prior weighting factor. (A larger alpha imposes stronger
%            similarity constraint between the solution and the prior
%            image.
%   mask:    (Optional) if the user wants to put a cbct fov mask on the
%            step change.
% ------------------------------------------
% OUTPUT
%   f:    The resultant 3D image.
%   time: The computation time.
% ------------------------------------------

%% Calculate

if ~isequal(size(f0),size(f_prior))
    error('ERROR: f0 and f_prior must have the same dimension.');
end

if nargin < 6
    mask = 1;
end

f = f0;

tic;

for k=1:N
    df = alpha* tvgrad3(f-f_prior) + (1-alpha)* tvgrad3(f);
    df = df .* mask;
    df = df / norm(df(:));
    f = f - step * df;
end

time = toc;

return;