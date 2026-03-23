function [f,time] = tvsd(f0,step,N,mask)
%% [f,time] = tvsd(f0,step,N,mask)
% ------------------------------------------
% FILE   : tvsd.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-01-03  Created.
% ------------------------------------------
% PURPOSE
%   TV-Steepest-Descent step.
% ------------------------------------------
% INPUT
%   f0:      The input 3D image.
%   step:   The step size.
%   N:      Number of iterations.
%   mask:   (Optional) if the user wants to put a cbct fov mask on the
%           step change.
% ------------------------------------------
% OUTPUT
%   f:    The resultant 3D image.
%   time: The computation time.
% ------------------------------------------

%% Calculate

if nargin < 4
    mask = 1;
end

f = f0;
% f = double(f0);

tic;

for k=1:N
    df = tvgrad3(f);
    df = df .* mask;
    df = df / norm(df(:));
    f = f - step * df;
end

time = toc;

return;